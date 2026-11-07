#include "src/ControlServer/BugReports/BugReports.hpp"
#include "src/ControlServer/ChatSession/ChatSession.hpp"
#include "src/ControlServer/Social/AgencyBank.hpp"
#include "src/ControlServer/ChatSession/ChatCommand.hpp"
#include "src/ControlServer/ChatSession/PlayerCommands.hpp"
#include "src/ControlServer/Database/Database.hpp"
#include "src/ControlServer/InstanceRegistry/InstanceRegistry.hpp"
#include "src/ControlServer/MatchmakingService/MatchmakingService.hpp"
#include "src/ControlServer/TcpSession/TcpSession.hpp"
#include "src/ControlServer/TeamService/TeamService.hpp"
#include "src/ControlServer/ServiceHooks/ServiceHooks.hpp"
#include <thread>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <optional>
#ifndef _WIN32
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <cerrno>
#include "src/ControlServer/Social/Friends.hpp"
#include "src/ControlServer/Social/ChatChannels.hpp"
#include "src/ControlServer/Social/Teams.hpp"
#endif

// All chat sessions — safe without mutex since io_context runs single-threaded.
// Pruned lazily (broadcast() drops expired weak_ptrs before iterating) and
// eagerly (TearDown removes its own entry).
static std::vector<std::weak_ptr<ChatSession>> g_chat_sessions;

namespace {

// Chat channel ids. These are the client's own enum — index into the name and
// colour tables built by FUN_10902990 (name at 0x1199ec28 + id*4, ARGB colour
// at 0x1199eca0 + id*4). See handoff.md §1 for the full table.
constexpr uint32_t kInstanceChannelId = 1;   // same instance, both sides
constexpr uint32_t kAgencyChannelId   = 2;   // agency membership
constexpr uint32_t kAllianceChannelId = 3;   // every agency in the alliance
constexpr uint32_t kLocalChannelId    = 4;   // same instance + same task force
constexpr uint32_t kTeamChannelId     = 5;   // party members
constexpr uint32_t kCityChannelId     = 7;   // every connected session
constexpr uint32_t kWhisperChannelId  = 8;   // /w, /tell
constexpr uint32_t kTradeChannelId    = 12;  // every connected session
constexpr uint32_t kLfgChannelId      = 13;  // every connected session

// Join/leave notices and error replies. Channel 11 (System) is present in all
// four default tab masks and renders yellow, so these no longer masquerade as
// Local chat.
constexpr uint32_t kSystemChannelId = 11;

// -announce goes out on channel 20. FUN_113be150 in the client remaps channel
// 20 to 11 (System) before testing the player's tab filter mask, and bit 11 is
// set in all four default tab masks — so a channel-20 line lands in every tab
// regardless of the player's Chat settings. It is the only id with that
// property, has no channel-name prefix of its own, and renders yellow.
constexpr uint32_t kAnnounceChannelId = 20;

// Server-pushed per-player feeds (login notice, Global News stand, Hall of
// Legends) go out on System (11) like the join line: channel-20 lines were
// never rendered in playtests while channel-11 lines were.
constexpr uint32_t kFeedChannelId = kSystemChannelId;

// Lowercased names permitted to use -announce. Empty = command disabled.
// Written once at startup before any session exists, read-only thereafter.
std::unordered_set<std::string> g_announcers;

// Config login_notice, sent only to the joining player.
std::string g_login_notice;
// Global News stand feed file (see SendNewsToGuid).
std::string g_news_file;

// Upstream's dev/cheat chat commands (-spawnenemy, -cheat, -fullheal,
// -possess, -topdown, -setspawntable, ...) have no permission check. Off unless the
// config has dev_controls = true (never on the live server). Written once at startup.
bool g_dev_chat_commands = false;

std::string ToLowerAscii(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

// Hard cap on the per-session write_queue_. Reached only if writes are
// failing or stalled long enough that the server has queued this many frames
// for one peer. At chat volumes that's an unrecoverable condition — the
// session is torn down instead of letting memory grow unbounded.
constexpr size_t kMaxWriteQueueDepth = 256;

constexpr int kBindRetryMaxAttempts = 50;
constexpr std::chrono::milliseconds kBindRetryDelay{100};

// Tune Linux TCP keepalive so a half-broken peer (NAT drop, dead wifi,
// powered-off router) is detected in ~50s instead of the default ~2 hours.
// Matches TcpSession::EnableKeepAlive so when a player hard-crashes, both
// their control and chat sockets get reaped in the same window — avoids a
// zombie chat session lingering after the gameplay socket is already gone.
// Combined sequence: 20s idle + 3 probes * 10s = 50s worst-case detection.
void EnableTcpKeepAlive(asio::ip::tcp::socket& sock) {
    std::error_code ec;
    sock.set_option(asio::socket_base::keep_alive(true), ec);
    if (ec) {
        Logger::Log("chat", "[Chat] set keep_alive failed: %s\n", ec.message().c_str());
        return;
    }
#ifdef _WIN32
    // Windows keepalive timing is configured through WSAIoctl(SIO_KEEPALIVE_VALS).
    // Keep the socket-level option here; timeout tuning belongs in a Win32-specific follow-up.
    return;
#else
    int fd = sock.native_handle();
    int idle = 20, intvl = 10, count = 3;
    if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE,  &idle,  sizeof(idle))  < 0 ||
        setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &intvl, sizeof(intvl)) < 0 ||
        setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT,   &count, sizeof(count)) < 0) {
        Logger::Log("chat", "[Chat] TCP_KEEPIDLE/INTVL/CNT tuning failed (errno=%d)\n", errno);
    }
#endif
}

// Drop expired weak_ptr entries from g_chat_sessions. Called lazily from
// broadcast() so the housekeeping cost is paid by chat traffic itself rather
// than as a periodic sweep.
void PruneExpiredSessions() {
    const size_t before = g_chat_sessions.size();
    g_chat_sessions.erase(
        std::remove_if(g_chat_sessions.begin(), g_chat_sessions.end(),
            [](const std::weak_ptr<ChatSession>& w) { return w.expired(); }),
        g_chat_sessions.end());
    if (g_chat_sessions.size() != before) {
        Logger::Log("chat", "[Chat] pruned %zu expired sessions (now %zu)\n",
            before - g_chat_sessions.size(), g_chat_sessions.size());
    }
}

// Build a CHAT_MESSAGE wire frame carrying a single (channel, message) pair.
// Layout matches the per-message frame produced by ChatSession::broadcast():
//   [2B chunk_size][2B packet_type=0x0073][2B item_count=2]
//   [2B CHAT_CH_TYPE][4B channel]
//   [2B MESSAGE][2B msg_len][msg_len bytes]
std::vector<uint8_t> BuildChatFrame(uint32_t channel, const std::string& message_text) {
    std::vector<uint8_t> body;
    const uint16_t packet_type = GA_U::CHAT_MESSAGE;
    const uint16_t item_count  = 2;
    body.push_back(packet_type & 0xFF); body.push_back(packet_type >> 8);
    body.push_back(item_count  & 0xFF); body.push_back(item_count  >> 8);

    // CHAT_CH_TYPE (4-byte int field)
    body.push_back(GA_T::CHAT_CH_TYPE & 0xFF);
    body.push_back(GA_T::CHAT_CH_TYPE >> 8);
    body.push_back(channel        & 0xFF);
    body.push_back((channel >>  8) & 0xFF);
    body.push_back((channel >> 16) & 0xFF);
    body.push_back((channel >> 24) & 0xFF);

    // MESSAGE (string field: type, len, bytes)
    body.push_back(GA_T::MESSAGE & 0xFF);
    body.push_back(GA_T::MESSAGE >> 8);
    const uint16_t msg_len = static_cast<uint16_t>(message_text.size());
    body.push_back(msg_len & 0xFF);
    body.push_back(msg_len >> 8);
    body.insert(body.end(), message_text.begin(), message_text.end());

    std::vector<uint8_t> chunk;
    const uint16_t chunk_size = static_cast<uint16_t>(body.size());
    chunk.push_back(chunk_size & 0xFF);
    chunk.push_back(chunk_size >> 8);
    chunk.insert(chunk.end(), body.begin(), body.end());
    return chunk;
}

// Build a CHAT_CONFIG (0x0072) wire frame enabling the given chat channels.
// The client keeps a 16-slot enabled-channel table (slot = CHAT_CH_TYPE,
// value = CHAT_CH_ID, nonzero = enabled) and refuses to switch its current
// send channel to a slot that is 0 — so without this packet a client that
// entered whisper mode (/r or reply keybind) can never switch back to Local.
// Layout:
//   [2B chunk_size][2B packet_type=0x0072][2B item_count=1]
//   [2B DATA_SET][2B row_count]
//     per row: [2B field_count=2][CHAT_CH_TYPE 4B][CHAT_CH_ID 4B]
std::vector<uint8_t> BuildChatConfigFrame(const std::vector<uint32_t>& channels) {
    std::vector<uint8_t> body;
    const uint16_t packet_type = GA_U::CHAT_CONFIG;
    const uint16_t item_count  = 1;
    body.push_back(packet_type & 0xFF); body.push_back(packet_type >> 8);
    body.push_back(item_count  & 0xFF); body.push_back(item_count  >> 8);

    body.push_back(GA_T::DATA_SET & 0xFF);
    body.push_back(GA_T::DATA_SET >> 8);
    const uint16_t row_count = static_cast<uint16_t>(channels.size());
    body.push_back(row_count & 0xFF);
    body.push_back(row_count >> 8);
    for (uint32_t ch : channels) {
        body.push_back(0x02); body.push_back(0x00);  // 2 fields per row
        for (uint16_t type : { (uint16_t)GA_T::CHAT_CH_TYPE, (uint16_t)GA_T::CHAT_CH_ID }) {
            body.push_back(type & 0xFF);
            body.push_back(type >> 8);
            body.push_back(ch        & 0xFF);
            body.push_back((ch >>  8) & 0xFF);
            body.push_back((ch >> 16) & 0xFF);
            body.push_back((ch >> 24) & 0xFF);
        }
    }

    std::vector<uint8_t> chunk;
    const uint16_t chunk_size = static_cast<uint16_t>(body.size());
    chunk.push_back(chunk_size & 0xFF);
    chunk.push_back(chunk_size >> 8);
    chunk.insert(chunk.end(), body.begin(), body.end());
    return chunk;
}

// VOIP_CONFIG (0x0184) frame from a voip_config service decision:
//   {"host":"..","user":"..","password":"..",
//    "rows":[{"type":<chat channel 0..15>,"id":<uint, 0 = leave>,"uri":"..","access":<uint>}]}
// The client starts voice on the first VOIP_CONFIG whose host/user/password are
// all non-empty, then per row leaves the voice channel of that type if it has a
// different id and joins `uri` when id != 0.
// Layout: [2B chunk][2B 0x0184][2B items=4][HOST_NAME s][USER_NAME s][PASSWORD s]
//         [DATA_SET][2B rows] per row: [2B fields=4][CHAT_CH_TYPE 4B][CHAT_CH_ID 4B][URI_NAME s][VOIP_ACCESS 4B]
std::vector<uint8_t> BuildVoipConfigFrame(const nlohmann::json& d) {
    const std::string host = ServiceHooks::ClampString(d, "host", 255);
    const std::string user = ServiceHooks::ClampString(d, "user", 255);
    const std::string pass = ServiceHooks::ClampString(d, "password", 255);
    if (host.empty() || user.empty() || pass.empty()) return {};
    std::vector<uint8_t> body;
    auto u16 = [&](uint16_t v) { body.push_back(v & 0xFF); body.push_back(v >> 8); };
    auto u32 = [&](uint16_t type, uint32_t v) {
        u16(type);
        for (int i = 0; i < 4; ++i) body.push_back((v >> (8 * i)) & 0xFF);
    };
    auto str = [&](uint16_t type, const std::string& v) {
        u16(type); u16(static_cast<uint16_t>(v.size()));
        body.insert(body.end(), v.begin(), v.end());
    };
    u16(GA_U::VOIP_CONFIG);
    u16(4);
    str(GA_T::HOST_NAME, host);
    str(GA_T::USER_NAME, user);
    str(GA_T::PASSWORD, pass);
    std::vector<nlohmann::json> rows;
    if (d.contains("rows") && d["rows"].is_array())
        for (const auto& r : d["rows"]) if (r.is_object() && rows.size() < 16) rows.push_back(r);
    u16(GA_T::DATA_SET);
    u16(static_cast<uint16_t>(rows.size()));
    for (const auto& r : rows) {
        u16(4);
        u32(GA_T::CHAT_CH_TYPE, static_cast<uint32_t>(ServiceHooks::ClampInt(r, "type", 0, 0, 15)));
        u32(GA_T::CHAT_CH_ID, static_cast<uint32_t>(ServiceHooks::ClampInt(r, "id", 0, 0, 0xFFFFFFFFLL)));
        str(GA_T::URI_NAME, ServiceHooks::ClampString(r, "uri", 511));
        u32(GA_T::VOIP_ACCESS, static_cast<uint32_t>(ServiceHooks::ClampInt(r, "access", 0, 0, 0xFFFFFFFFLL)));
    }
    std::vector<uint8_t> chunk;
    chunk.push_back(body.size() & 0xFF);
    chunk.push_back((body.size() >> 8) & 0xFF);
    chunk.insert(chunk.end(), body.begin(), body.end());
    return chunk;
}

// Broadcast a system message (e.g. join/leave announcement) to every active
// chat session. Doesn't read or modify any per-session state besides
// write_queue_ via deliver(). `exclude` skips one specific session (used for
// "left the chat" — the leaving session's socket is already closed).
void BroadcastSystemMessage(const std::string& message_text,
                            uint32_t channel,
                            ChatSession* exclude) {
    auto frame = BuildChatFrame(channel, message_text);
    PruneExpiredSessions();
    size_t delivered = 0;
    for (auto& weak : g_chat_sessions) {
        if (auto peer = weak.lock()) {
            if (peer.get() == exclude) continue;
            peer->deliver(frame);
            delivered++;
        }
    }
    Logger::Log("chat", "[Chat] system msg ch=%u text='%s' delivered to %zu session(s)\n",
        channel, message_text.c_str(), delivered);
}

// Which sessions may receive a message on `channel`.
//
// nullopt      = every connected session (City, and any channel we don't scope).
// empty set    = the sender can't use this channel right now (not in an
//                instance / not in a team) — caller replies with a system line
//                instead of broadcasting.
// non-empty    = deliver only to sessions whose guid is in the set. The sender's
//                own guid is included, so their line still echoes back to them.
//
// One roster lookup per message, not per peer.
std::optional<std::unordered_set<std::string>> RecipientsFor(
        uint32_t channel, const std::string& sender_guid) {
    if (auto r = Social::Teams::ChatRecipients(channel, sender_guid)) return r;   // raid chat 6 + team chat inside a raid
    if (channel == kTeamChannelId) {
        const auto guids = TeamService::GetTeamMemberGuids(sender_guid);
        return std::unordered_set<std::string>(guids.begin(), guids.end());
    }

    if (channel == kAgencyChannelId || channel == kAllianceChannelId) {
        // Agency membership is keyed by account, not character or session, so
        // resolve guid -> user_id through the session store on both ends.
        auto sender = PlayerSessionStore::GetByGuid(sender_guid);
        if (!sender || sender->user_id == 0) return std::unordered_set<std::string>{};
        const int64_t agency_id = Database::GetAgencyIdForUser(sender->user_id);
        if (agency_id == 0) return std::unordered_set<std::string>{};

        std::vector<int64_t> member_ids;
        if (channel == kAgencyChannelId) {
            member_ids = Database::GetAgencyMemberUserIds(agency_id);
        } else {
            const int64_t alliance_id = Database::GetAllianceIdForAgency(agency_id);
            if (alliance_id == 0) return std::unordered_set<std::string>{};
            member_ids = Database::GetAllianceMemberUserIds(alliance_id);
        }
        const std::unordered_set<int64_t> members(member_ids.begin(), member_ids.end());

        // Walk the connected sessions rather than the member list: offline
        // members can't receive anything, and this keeps it to one store
        // lookup per online player instead of a DB query per member.
        std::unordered_set<std::string> out;
        PruneExpiredSessions();
        for (auto& weak : g_chat_sessions) {
            if (auto peer = weak.lock()) {
                const std::string& guid = peer->get_session_guid();
                if (guid.empty()) continue;
                auto info = PlayerSessionStore::GetByGuid(guid);
                if (info && members.count(info->user_id)) out.insert(guid);
            }
        }
        Logger::Log("chat", "[Scope] ch=%u agency=%lld members=%zu recipients=%zu\n",
            channel, (long long)agency_id, members.size(), out.size());
        return out;
    }

    if (channel == kInstanceChannelId || channel == kLocalChannelId) {
        auto where = InstanceRegistry::GetInstancePlayerTaskForce(sender_guid);
        if (!where) return std::unordered_set<std::string>{};  // not in an instance
        const int64_t instance_id = where->first;
        const int sender_tf       = where->second;

        // Local is side-scoped ("Attacker/Defender Channel" per the client's
        // own /help text). On the home map / VR there are no sides — task
        // force comes back unset, so fall back to instance-wide.
        const bool side_scoped = (channel == kLocalChannelId) && sender_tf > 0;

        std::unordered_set<std::string> out;
        const auto roster = InstanceRegistry::GetActivePlayersForInstance(instance_id);
        for (const auto& row : roster) {
            if (side_scoped && row.task_force != sender_tf) continue;
            if (!row.guid.empty()) out.insert(row.guid);
        }
        // Recipient count is the one number worth having when someone reports
        // "my message didn't arrive" — autobalance can leave a player alone on
        // a side, where recipients=1 is correct rather than broken.
        Logger::Log("chat", "[Scope] ch=%u inst=%lld sender_tf=%d roster=%zu recipients=%zu\n",
            channel, (long long)instance_id, sender_tf, roster.size(), out.size());
        return out;
    }

    if (channel == kCityChannelId) return Social::ChatChannels::ZoneRecipients(sender_guid);   // zone chat
    return std::nullopt;  // Trade/LFG + anything unscoped: everyone.
}

// System line shown when a player sends on a channel they can't currently use.
const char* UnavailableChannelText(uint32_t channel) {
    if (channel == kTeamChannelId)     return "*** You are not in a team ***";
    if (channel == 6)                  return "*** You must be in a raid to do this ***";
    if (channel == kAgencyChannelId)   return "*** You are not in an agency ***";
    if (channel == kAllianceChannelId) return "*** Your agency is not in an alliance ***";
    return "*** You are not in an instance ***";
}

std::string SanitizeCommandForLog(std::string s) {
    for (char& c : s) {
        if (c == '\r' || c == '\n' || c == '\t') c = ' ';
    }
    constexpr size_t kMaxCommandLogBytes = 160;
    if (s.size() > kMaxCommandLogBytes) {
        s.resize(kMaxCommandLogBytes);
        s += "...";
    }
    return s;
}

bool HasParsedCommandAction(const ChatCommand::ParseResult& parsed) {
    return parsed.change_team.has_value()
        || parsed.report.has_value()
        || parsed.player_cmd.has_value()
        || parsed.spawn_target.has_value()
        || parsed.deploy_target.has_value()
        || parsed.topdown.has_value()
        || parsed.markers.has_value()
        || parsed.fx_browse.has_value()
        || parsed.set_spawn_table.has_value()
        || parsed.spectate.has_value()
        || parsed.toggle_broken_suits.has_value()
        || parsed.toggle_solo_mode.has_value()
        || parsed.cheat.has_value()
        || parsed.override_diff.has_value()
        || parsed.set_dlc.has_value()
        || parsed.possess
        || parsed.unpossess
        || parsed.unspectate
        || parsed.coords
        || parsed.fullheal
        || parsed.class_counts
        || parsed.reload_queues
        || parsed.components.has_value()
        || parsed.announce.has_value();
}

} // anonymous namespace

ChatSession::ChatSession(asio::ip::tcp::socket socket)
    : socket_(std::move(socket)),
      bind_retry_timer_(socket_.get_executor()),
      voice_timer_(socket_.get_executor()),
      read_buf_(4096) {
    socket_.set_option(asio::ip::tcp::no_delay(true));
    EnableTcpKeepAlive(socket_);
}

void ChatSession::start() {
    g_chat_sessions.push_back(shared_from_this());
    Logger::Log("chat", "[Chat] Client connected, sessions=%zu\n", g_chat_sessions.size());
    do_read();
}

void ChatSession::AnnounceJoinIfNeeded() {
    if (announced_join_ || player_name_.empty()) return;
    announced_join_ = true;
    // Populate the client's send-channel table (combo box) — see
    // BuildChatConfigFrame. Sent once at join and never revised: a player who
    // picks a channel they can't currently use (Team with no party, Local
    // outside an instance) gets a system reply instead. Far less state to keep
    // in sync than re-sending CHAT_CONFIG on every instance or party change.
    //
    // The combo box can only ever show channels from a hardcoded 10-entry
    // candidate table in the client (`DAT_1199c770`, read by FUN_113bdd20):
    // 4 Local, 7 City, 2 Agency, 3 Alliance, 5 Team, 6 Raid, 12 Trade, 13 LFG,
    // 9 Private1, 10 GM. This packet only decides which of *those* are shown.
    // Instance (1) and Personal (8) are not on it and can never appear —
    // Instance is reachable via the client's own /i, Personal via the tell
    // flow. Both are still enabled here because the client's channel-switch
    // paths check this table. Private1 (9) and GM (10) are deliberately
    // omitted: they're selectable but sit in no default tab, so anything sent
    // on them is invisible to every recipient.
    deliver(BuildChatConfigFrame({ kCityChannelId, kLocalChannelId,
                                   kInstanceChannelId, kTeamChannelId,
                                   kWhisperChannelId, kTradeChannelId,
                                   kLfgChannelId, kAgencyChannelId,
                                   kAllianceChannelId, 6 /* Raid */ }));
    for (size_t a = 0, b; !g_login_notice.empty() && a < g_login_notice.size(); a = b + 1) {   // "\n" = one feed line each
        b = g_login_notice.find('\n', a); if (b == std::string::npos) b = g_login_notice.size();
        if (b > a) deliver(BuildChatFrame(kFeedChannelId, g_login_notice.substr(a, b - a)));
    }
    Social::Friends::OnOnline(player_name_, socket_.get_executor());   // friend notice + F3 refresh
    AgencyChat::OnChatJoin(session_guid_);   // agency MOTD + member online line
    BroadcastSystemMessage(
        "*** " + player_name_ + " has joined the chat ***",
        kSystemChannelId,
        /*exclude=*/nullptr);
    if (ServiceHooks::Enabled("voip_config")) VoiceTick();
}

bool ChatSession::BindPlayerFromRegistry(const char* reason) {
    if (closed_ || session_guid_.empty()) return false;

    auto info = PlayerSessionStore::GetByGuid(session_guid_);
    if (!info) return false;

    player_name_ = info->player_name;
    if (!bound_at_) bound_at_ = static_cast<int64_t>(std::time(nullptr));
    bind_retry_timer_.cancel();
    Logger::Log("chat", "[Chat] %s bound player='%s' guid=%s\n",
        reason ? reason : "bind", player_name_.c_str(), session_guid_.c_str());
    AnnounceJoinIfNeeded();
    return true;
}

void ChatSession::ScheduleBindRetry() {
    if (closed_ || session_guid_.empty() || !player_name_.empty()) return;
    if (bind_retry_attempts_ >= kBindRetryMaxAttempts) {
        Logger::Log("chat", "[Chat] bind retry exhausted guid=%s attempts=%d\n",
            session_guid_.c_str(), bind_retry_attempts_);
        return;
    }

    ++bind_retry_attempts_;
    auto self(shared_from_this());
    bind_retry_timer_.expires_after(kBindRetryDelay);
    bind_retry_timer_.async_wait([this, self](std::error_code ec) {
        if (closed_ || ec) return;
        if (BindPlayerFromRegistry("retry")) return;
        ScheduleBindRetry();
    });
}

void ChatSession::deliver(const std::vector<uint8_t>& msg) {
    if (closed_) return;
    if (write_queue_.size() >= kMaxWriteQueueDepth) {
        Logger::Log("chat",
            "[Chat] write_queue overflow (>=%zu) for player='%s' guid=%s — tearing down\n",
            kMaxWriteQueueDepth, player_name_.c_str(), session_guid_.c_str());
        TearDown("write_queue_overflow");
        return;
    }
    const bool idle = write_queue_.empty();
    write_queue_.push_back(msg);
    if (idle) do_write();
}

void ChatSession::do_write() {
    if (closed_) return;
    auto self(shared_from_this());
    asio::async_write(socket_, asio::buffer(write_queue_.front()),
        [this, self](std::error_code ec, std::size_t bytes) {
            if (closed_) return;
            if (ec) {
                // Previously: silently returned, leaving write_queue_ non-empty
                // forever, so future deliver()s short-circuited on idle=false
                // and the session was stuck receiving nothing. Now we log,
                // tear down, and let the read side notice the closed socket.
                Logger::Log("chat",
                    "[Chat] write error player='%s' guid=%s ec='%s' (%d) queue=%zu — tearing down\n",
                    player_name_.c_str(), session_guid_.c_str(),
                    ec.message().c_str(), ec.value(), write_queue_.size());
                TearDown("write_error");
                return;
            }
            write_queue_.pop_front();
            if (!write_queue_.empty()) do_write();
            (void)bytes;
        });
}

void ChatSession::do_read() {
    if (closed_) return;
    auto self(shared_from_this());
    socket_.async_read_some(asio::buffer(read_buf_),
        [this, self](std::error_code ec, std::size_t len) {
            if (closed_) return;
            if (ec) {
                Logger::Log("chat",
                    "[Chat] read disconnect player='%s' guid=%s ec='%s' (%d)\n",
                    player_name_.c_str(), session_guid_.c_str(),
                    ec.message().c_str(), ec.value());
                TearDown("read_eof_or_error");
                return;
            }

            // Frame the byte stream into discrete packets using the wire's
            // own 2-byte length prefix. Layout per packet:
            //   [2B payload_len][2B packet_type][2B item_count][TLV...]
            // where payload_len is the byte count of everything AFTER the
            // prefix itself (matches how broadcast() builds the chunk).
            // Without this loop, coalesced packets (two messages arriving in
            // one read) had only the first processed, and split packets
            // (one message split across reads) were forwarded as garbage —
            // which is why same-host testing worked but remote clients
            // silently lost each other's messages.
            accumulator_.insert(accumulator_.end(),
                                read_buf_.data(), read_buf_.data() + len);

            while (accumulator_.size() >= 2) {
                const uint16_t payload_len =
                    static_cast<uint16_t>(accumulator_[0]) |
                    (static_cast<uint16_t>(accumulator_[1]) << 8);
                const size_t total = static_cast<size_t>(2) + payload_len;
                if (accumulator_.size() < total) break;  // partial packet, await more

                handle_packet(accumulator_.data(), total);
                if (closed_) return;  // handler may have torn us down

                accumulator_.erase(accumulator_.begin(),
                                   accumulator_.begin() + total);
            }

            do_read();
        });
}

void ChatSession::handle_packet(const uint8_t* data, size_t length) {
    // Wire format: [2B length][2B packet_type=0x0073][2B item_count][TLV fields...]
    if (length < 6) return;

    uint16_t packet_type = data[2] | (data[3] << 8);
    if (packet_type == Social::ChatChannels::kListChannelMembers) { Social::ChatChannels::OnListMembers(session_guid_, data, length); return; }   // /chatwho
    if (packet_type != GA_U::CHAT_MESSAGE) {
        Logger::Log("chat", "[Chat] Unknown packet type 0x%04X\n", packet_type);
        return;
    }

    // Distinguish initial handshake (first field = SESSION_GUID 0x0473)
    // from a real chat message (first field = CHAT_CH_TYPE 0x00BE).
    if (length >= 8) {
        uint16_t first_field = data[6] | (data[7] << 8);
        if (first_field == GA_T::SESSION_GUID) {
            // Extract 16-byte GUID and look up the player
            if (length >= 8 + 16) {
                std::ostringstream oss;
                for (int i = 0; i < 16; i++)
                    oss << std::hex << std::setw(2) << std::setfill('0') << (int)data[8 + i];
                std::string guid_hex = oss.str();
                session_guid_ = guid_hex;
                bind_retry_attempts_ = 0;
                if (!BindPlayerFromRegistry("handshake")) {
                    Logger::Log("chat", "[Chat] Handshake received, guid=%s not found in registry\n", guid_hex.c_str());
                    ScheduleBindRetry();
                }
            } else {
                Logger::Log("chat", "[Chat] Handshake received (guid too short)\n");
            }
            return;
        }
    }

    // Defensive: if the handshake landed before PlayerSessionStore::Register
    // for any reason, retry the lookup so the broadcaster shows a real name
    // instead of "" on the next message. Cheap, single map lookup.
    if (player_name_.empty() && !session_guid_.empty()) {
        BindPlayerFromRegistry("message");
    }
    if (player_name_.empty()) {
        Logger::Log("chat", "[Chat] dropping unbound message guid=%s\n",
            session_guid_.c_str());
        return;
    }

    // Peek at MESSAGE before broadcasting so we can intercept slash commands.
    {
        PacketView pkt(data + 6, length - 6);
        std::string message_text = pkt.ReadString(GA_T::MESSAGE).value_or("");
        auto parsed = ChatCommand::TryParseChatCommand(message_text);
        if (parsed.recognized) {
            const std::string command_for_log = SanitizeCommandForLog(message_text);
            if (!HasParsedCommandAction(parsed)) {
                Logger::Log("chat-command",
                    "[ChatCmd] player='%s' guid=%s command='%s' outcome=ignored details=invalid_args\n",
                    player_name_.c_str(), session_guid_.c_str(), command_for_log.c_str());
            }
        }
        if (parsed.recognized && !g_dev_chat_commands) {
            // Players keep -changeteam, -classcounts, -announce (own list) and
            // -components (own compile-time switch); everything else is dev-only.
            const bool dev_only = parsed.spawn_target || parsed.deploy_target || parsed.topdown
                || parsed.spectate || parsed.toggle_broken_suits || parsed.toggle_solo_mode
                || parsed.set_dlc || parsed.markers || parsed.fx_browse || parsed.set_spawn_table
                || parsed.cheat || parsed.possess || parsed.unpossess || parsed.unspectate
                || parsed.coords || parsed.fullheal || parsed.reload_queues;
            if (dev_only) {
                parsed.spawn_target.reset(); parsed.deploy_target.reset(); parsed.topdown.reset();
                parsed.spectate.reset(); parsed.toggle_broken_suits.reset(); parsed.toggle_solo_mode.reset();
                parsed.set_dlc.reset(); parsed.markers.reset(); parsed.fx_browse.reset();
                parsed.set_spawn_table.reset(); parsed.cheat.reset();
                parsed.possess = parsed.unpossess = parsed.unspectate = false;
                parsed.coords = parsed.fullheal = parsed.reload_queues = false;
                Logger::Log("chat-command",
                    "[ChatCmd] player='%s' guid=%s outcome=denied details=dev_controls_off\n",
                    player_name_.c_str(), session_guid_.c_str());
                deliver(BuildChatFrame(kSystemChannelId,
                    "*** That command is not available on this server ***"));
            }
        }
        if (parsed.recognized && parsed.change_team) {
            ChatCommand::DispatchChangeTeam(*parsed.change_team, session_guid_);
        }
        if (parsed.recognized && parsed.spawn_target) {
            ChatCommand::DispatchSpawnTarget(*parsed.spawn_target, session_guid_);
        }
        if (parsed.recognized && parsed.deploy_target) {
            ChatCommand::DispatchDeployTarget(*parsed.deploy_target, session_guid_);
        }
        if (parsed.recognized && parsed.possess) {
            ChatCommand::DispatchPossess(session_guid_);
        }
        if (parsed.recognized && parsed.unpossess) {
            ChatCommand::DispatchUnpossess(session_guid_);
        }
        if (parsed.recognized && parsed.coords) {
            ChatCommand::DispatchCoords(session_guid_);
        }
        if (parsed.recognized && parsed.fullheal) {
            ChatCommand::DispatchFullHeal(session_guid_);
        }
        if (parsed.recognized && parsed.class_counts) {
            ChatCommand::ExecuteClassCounts(session_guid_);
        }
        if (parsed.recognized && parsed.topdown) {
            ChatCommand::DispatchTopDown(*parsed.topdown, session_guid_);
        }
        if (parsed.recognized && parsed.markers) {
            ChatCommand::DispatchMarkers(*parsed.markers, session_guid_);
        }
        if (parsed.recognized && parsed.fx_browse) {
            ChatCommand::DispatchFxBrowse(*parsed.fx_browse, session_guid_);
        }
        if (parsed.recognized && parsed.set_spawn_table) {
            ChatCommand::DispatchSetSpawnTable(*parsed.set_spawn_table, session_guid_);
        }
        if (parsed.recognized && parsed.spectate) {
            HandleSpectateCommand(parsed.spectate->instance_id, parsed.spectate->team);
        }
        if (parsed.recognized && parsed.unspectate) {
            HandleUnspectateCommand();
        }
        if (parsed.recognized && parsed.toggle_broken_suits) {
            ChatCommand::DispatchToggleBrokenSuits(*parsed.toggle_broken_suits, session_guid_);
        }
        if (parsed.recognized && parsed.toggle_solo_mode) {
            ChatCommand::ExecuteToggleSoloMode(*parsed.toggle_solo_mode, session_guid_);
        }
        if (parsed.recognized && parsed.set_dlc) {
            ChatCommand::ExecuteSetDlc(*parsed.set_dlc, session_guid_);
        }
        if (parsed.recognized && parsed.announce) {
            if (!IsAnnouncer()) {
                Logger::Log("chat-command",
                    "[ChatCmd] -announce player='%s' guid=%s outcome=denied details=not_in_announcer_list\n",
                    player_name_.c_str(), session_guid_.c_str());
                deliver(BuildChatFrame(kSystemChannelId,
                    "*** You are not permitted to use -announce ***"));
            } else {
                // No ignore gate and no self-skip: an announcement goes to
                // every connected session, sender included.
                const auto frame = BuildChatFrame(kAnnounceChannelId, *parsed.announce);
                PruneExpiredSessions();
                size_t sent = 0;
                for (auto& weak : g_chat_sessions) {
                    if (auto peer = weak.lock()) { peer->deliver(frame); sent++; }
                }
                Logger::Log("chat-command",
                    "[ChatCmd] -announce player='%s' guid=%s outcome=sent details=%zu_session(s) text='%s'\n",
                    player_name_.c_str(), session_guid_.c_str(), sent,
                    parsed.announce->c_str());
            }
        }
        if (parsed.recognized && parsed.report) {
            const auto res = BugReports::Submit(parsed.report->kind, session_guid_, player_name_,
                parsed.report->target, parsed.report->text);
            deliver(BuildChatFrame(kSystemChannelId, res.reply));
        }
        if (parsed.recognized && parsed.player_cmd) {
            PlayerCommands::Run(session_guid_, parsed.player_cmd->name, parsed.player_cmd->rest);
        }
        if (parsed.recognized && parsed.components.has_value()) {
            HandleComponentsCommand(*parsed.components);
        }
        if (parsed.recognized && parsed.reload_queues) {
            Logger::Log("chat-command",
                "[ChatCmd] -reload-queues player='%s' guid=%s outcome=activated details=MatchmakingService::ReloadQueues\n",
                player_name_.c_str(), session_guid_.c_str());
            MatchmakingService::ReloadQueues();
        }
        if (parsed.recognized && parsed.cheat) {
            Logger::Log("chat-command",
                "[ChatCmd] -cheat player='%s' mode='%d'\n",
                player_name_.c_str(), *parsed.cheat);
            ChatCommand::DispatchToggleCheatMode (*parsed.cheat, session_guid_);
        }
        if (parsed.recognized && parsed.override_diff) {
            ChatCommand::ExecuteOverrideDiff(*parsed.override_diff, session_guid_);
        }
        if (parsed.suppress_broadcast) {
            // Recognized command (valid or invalid arg) — do not broadcast.
            return;
        }
    }

    Logger::Log("chat", "[Chat] Message received from player='%s' broadcasting to %zu peer(s)\n",
        player_name_.c_str(),
        g_chat_sessions.size() > 0 ? g_chat_sessions.size() - 1 : 0);
    broadcast(data, length);
}

// -components — the inventory-desync test harness.
//
// Bare form prints the three numbers that must agree, so a desync can be read
// off one chat line instead of correlated across two log files:
//   pool   = ga_players_inventory rows send_inventory_response ships
//   comps  = live ga_user_components stacks it also ships
//   total  = what ATgInventoryManager::r_ItemCount must equal, and therefore
//            what the client's m_InventoryMap must contain.
//
// grant/take push a stack through the production wire paths — a take that
// empties a stack fires the STATE=2 removal, which is the map-shrink case the
// desync tracks.
void ChatSession::HandleComponentsCommand(
        const ChatCommand::ParseResult::ComponentsArgs& args) {
    auto info = PlayerSessionStore::GetByGuid(session_guid_);
    const int64_t user_id      = info ? info->user_id : 0;
    const int64_t character_id = info ? info->selected_character_id : 0;

    if (user_id <= 0 || character_id == 0) {
        deliver(BuildChatFrame(kSystemChannelId, "components: no character selected."));
        return;
    }

    if (args.op == ChatCommand::ParseResult::ComponentsArgs::Op::Report) {
        const auto charInfo = PlayerSessionStore::GetCharacterById(character_id);
        const int  profile  = charInfo ? (int)charInfo->profile_id : 0;
        const int  total    = Database::GetExpectedItemCount(user_id, profile);
        const auto comps    = Database::GetAllComponents(user_id);

        char line[256];
        snprintf(line, sizeof(line),
            "components: char=%lld profile=%d  pool+comps = r_ItemCount must be %d "
            "(%zu live component stack(s))",
            (long long)character_id, profile, total, comps.size());
        deliver(BuildChatFrame(kSystemChannelId, line));

        for (const auto& c : comps) {
            snprintf(line, sizeof(line), "   item %d x%d", c.item_id, c.quantity);
            deliver(BuildChatFrame(kSystemChannelId, line));
        }

        Logger::Log("chat-command",
            "[ChatCmd] -components report player='%s' guid=%s outcome=sent details=total_%d\n",
            player_name_.c_str(), session_guid_.c_str(), total);
        return;
    }

    const bool take = (args.op == ChatCommand::ParseResult::ComponentsArgs::Op::Take);
    const std::string result = TcpSession::DeliverComponentDebug(
        session_guid_, take ? 1 : 0, args.item_id, args.count);

    deliver(BuildChatFrame(kSystemChannelId, result));
    Logger::Log("chat-command",
        "[ChatCmd] -components %s player='%s' guid=%s item=%d n=%d outcome=%s\n",
        take ? "take" : "grant", player_name_.c_str(), session_guid_.c_str(),
        args.item_id, args.count, result.c_str());
}

void ChatSession::HandleSpectateCommand(int64_t instance_id, ChatCommand::SpectateTeam team) {
    auto info = PlayerSessionStore::GetByGuid(session_guid_);
    const int64_t user_id = info ? info->user_id : 0;

    if (user_id <= 0 || !Database::UserHasRole(user_id, "spectator")) {
        deliver(BuildChatFrame(kSystemChannelId,
            "You don't have permission to use spectator mode."));
        Logger::Log("chat-command",
            "[ChatCmd] -spectate player='%s' guid=%s outcome=rejected details=no_spectator_role\n",
            player_name_.c_str(), session_guid_.c_str());
        return;
    }

    if (instance_id == 0) {
        // List request: running, READY, non-home-map instances only — a
        // spectator has nowhere useful to go in a still-starting instance,
        // and the always-on home/lobby map is excluded per design (nothing
        // there to spectate).
        auto running = InstanceRegistry::GetAllRunningInstances();
        std::vector<InstanceInfo> spectatable;
        for (auto& inst : running) {
            if (!inst.is_home_map && inst.state == "READY") spectatable.push_back(inst);
        }

        if (spectatable.empty()) {
            deliver(BuildChatFrame(kSystemChannelId,
                "No running instances available to spectate right now."));
        } else {
            deliver(BuildChatFrame(kSystemChannelId,
                "Instances available to spectate (join with -spectate <id>):"));
            for (auto& inst : spectatable) {
                std::ostringstream line;
                line << "  [" << inst.instance_id << "] " << inst.map_name
                     << " (" << inst.game_mode << ") players="
                     << InstanceRegistry::GetActivePlayerCount(inst.instance_id)
                     << "/" << inst.max_players;
                deliver(BuildChatFrame(kSystemChannelId, line.str()));
            }
        }
        Logger::Log("chat-command",
            "[ChatCmd] -spectate player='%s' guid=%s outcome=listed count=%zu\n",
            player_name_.c_str(), session_guid_.c_str(), spectatable.size());
        return;
    }

    // Purely cosmetic — 0 = teamless (unchanged behavior), 1/2 assign
    // PRI.r_TaskForce/Team so the client's own same-team HUD checks resolve.
    // Never affects whether a pawn spawns (still gated on is_spectator).
    const int team_task_force = (team == ChatCommand::SpectateTeam::Attackers) ? 1
                               : (team == ChatCommand::SpectateTeam::Defenders) ? 2
                               : 0;

    std::string message;
    const bool dispatched =
        TcpSession::DeliverSpectateJoin(session_guid_, instance_id, team_task_force, message);
    deliver(BuildChatFrame(kSystemChannelId, message));
    Logger::Log("chat-command",
        "[ChatCmd] -spectate player='%s' guid=%s instance=%lld team_tf=%d outcome=%s details=%s\n",
        player_name_.c_str(), session_guid_.c_str(), (long long)instance_id,
        team_task_force, dispatched ? "dispatched" : "rejected", message.c_str());
}

void ChatSession::HandleUnspectateCommand() {
    std::string message;
    const bool dispatched = TcpSession::DeliverSpectateExit(session_guid_, message);
    deliver(BuildChatFrame(kSystemChannelId, message));
    Logger::Log("chat-command",
        "[ChatCmd] -unspectate player='%s' guid=%s outcome=%s details=%s\n",
        player_name_.c_str(), session_guid_.c_str(),
        dispatched ? "dispatched" : "rejected", message.c_str());
}

void ChatSession::broadcast(const uint8_t* data, size_t length) {
    std::vector<uint8_t> response;

    PacketView pkt(data + 6, length - 6);

    // Missing channel field falls back to City — the unscoped "everyone"
    // channel, which is what this code did before channels were scoped.
    uint32_t channel = pkt.Read4B(GA_T::CHAT_CH_TYPE).value_or(kCityChannelId);
    if (channel == Social::ChatChannels::kGlobalChannelId && Social::ChatChannels::OnGlobal(session_guid_, player_name_, IsAnnouncer(), data, length)) return;   // /gl /alert

    std::string message = player_name_ + ": " + pkt.ReadString(GA_T::MESSAGE).value_or("");
    if (std::string gag; Social::Friends::IsGagged(player_name_, &gag)) { deliver(BuildChatFrame(kSystemChannelId, gag)); return; }
    Social::Friends::Activity(player_name_);   // chatting clears /afk
    if (Social::Teams::HandleRaidAlert(session_guid_, player_name_, data, length)) return;   // /rgw /raidw

    bool isDM = channel == kWhisperChannelId;

    if (isDM) {
        // Whisper: recipient arrives in PLAYER_NAME (0x03C5), verified via live capture.
        const std::string recipient = pkt.ReadString(GA_T::PLAYER_NAME).value_or("");
        auto ieq = [](const std::string& a, const std::string& b) {
            if (a.size() != b.size()) return false;
            for (size_t i = 0; i < a.size(); i++)
                if (std::tolower((unsigned char)a[i]) != std::tolower((unsigned char)b[i]))
                    return false;
            return true;
        };
        if (ieq(recipient, player_name_)) {
            deliver(BuildChatFrame(kSystemChannelId,
                "*** You cannot whisper yourself ***"));
            return;
        }
        PruneExpiredSessions();
        std::shared_ptr<ChatSession> target;
        for (auto& weak : g_chat_sessions) {
            if (auto peer = weak.lock()) {
                if (ieq(peer->get_player_name(), recipient)) { target = peer; break; }
            }
        }
        if (!target) {
            deliver(BuildChatFrame(kSystemChannelId,
                "*** " + recipient + " is not online ***"));
            return;
        }
        // Per-side frames. On channel 8 the client composes the line itself:
        // NAME (0x370) == own player name + non-empty PLAYER_NAME (0x3C5)
        // renders "To <PLAYER_NAME>: msg" (sender echo), otherwise
        // "<NAME> says: msg". The reply cache is NAME, and it is overwritten on
        // every ch8 line whose NAME != own name — so the echo MUST carry the
        // sender's own name, else it blanks the cache and /r dies after one use.
        const std::string text = pkt.ReadString(GA_T::MESSAGE).value_or("");
        auto makeWhisperFrame = [this, channel](const std::string& displayName,
                                                const std::string& msgText,
                                                const std::string& replyToName) {
            std::vector<uint8_t> body;
            uint16_t pt = GA_U::CHAT_MESSAGE, ic = 4;
            append(body, pt & 0xFF, pt >> 8, ic & 0xFF, ic >> 8);
            Write4B(body, GA_T::CHAT_CH_TYPE, channel);
            WriteString(body, GA_T::NAME, displayName);
            WriteString(body, GA_T::MESSAGE, msgText);
            WriteString(body, GA_T::PLAYER_NAME, replyToName);
            std::vector<uint8_t> frame;
            uint16_t sz = static_cast<uint16_t>(body.size());
            frame.push_back(sz & 0xFF);
            frame.push_back(sz >> 8);
            frame.insert(frame.end(), body.begin(), body.end());
            return frame;
        };

        const std::string& targetName = target->get_player_name();
        // Ignore gate: recipient has the sender on their F3 ignore list.
        if (target->is_ignoring(player_name_)) {
            Logger::Log("friends", "[Friends] whisper blocked: '%s' ignores '%s'\n",
                targetName.c_str(), player_name_.c_str());
            deliver(BuildChatFrame(kSystemChannelId,
                "*** " + targetName + " is ignoring you ***"));
            return;
        }
        target->deliver(makeWhisperFrame(player_name_, text, player_name_));
        deliver(makeWhisperFrame(player_name_, text, targetName));
        if (auto afk = Social::Friends::AfkAutoReply(targetName); !afk.empty())   // "%s - AFK"
            deliver(BuildChatFrame(kSystemChannelId, afk));
        return;
    }

    SendOnChannel(channel, message);
}

// Scope, build and deliver one already-formatted chat line. Shared by the chat
// socket (ChatSession::broadcast) and by slash commands arriving as
// PLAYER_COMMAND on the main TCP socket (SendChannelMessageFrom).
void ChatSession::SendOnChannel(uint32_t channel, const std::string& message) {
    // Scope the recipients before building the frame — an unusable channel
    // costs the sender a system line and nobody else anything.
    const auto recipients = RecipientsFor(channel, session_guid_);
    if (recipients && recipients->empty()) {
        Logger::Log("chat", "[Chat] ch=%u unavailable for player='%s'\n",
            channel, player_name_.c_str());
        deliver(BuildChatFrame(kSystemChannelId, UnavailableChannelText(channel)));
        return;
    }

    Logger::Log("chat", "Broadcasting message '%s' to channel %u (%s)\n",
        message.c_str(), channel,
        recipients ? "scoped" : "all sessions");

    std::vector<uint8_t> response;
    const uint16_t packet_type = GA_U::CHAT_MESSAGE;
    const uint16_t item_count = 2;

    append(response, packet_type & 0xFF, packet_type >> 8);
    append(response, item_count & 0xFF, item_count >> 8);

    Write4B(response, GA_T::CHAT_CH_TYPE, channel);
    WriteString(response, GA_T::MESSAGE, message);

    std::vector<uint8_t> chunk;
    const uint16_t chunk_size = static_cast<uint16_t>(response.size());

    chunk.push_back(chunk_size & 0xFF);
    chunk.push_back(chunk_size >> 8);

    chunk.insert(chunk.end(), response.begin(), response.end());

    // Sweep expired entries on every broadcast — keeps the loop tight and
    // ensures stale weak_ptrs from prior tear-downs don't accumulate.
    PruneExpiredSessions();

    for (auto& weak : g_chat_sessions) {
        if (auto peer = weak.lock()) {
            // Channel scope: instance / side / party membership.
            if (recipients && !recipients->count(peer->get_session_guid())) {
                continue;
            }
            // Ignore gate: don't show this sender's lines to players who
            // have them on the F3 ignore list. Logged because a one-way
            // ignore is indistinguishable from a broken channel when you're
            // staring at two clients — the sender still sees their own line.
            if (peer.get() != this && peer->is_ignoring(player_name_)) {
                Logger::Log("chat", "[Chat] ch=%u '%s' ignores '%s' — line dropped\n",
                    channel, peer->get_player_name().c_str(), player_name_.c_str());
                continue;
            }
            peer->deliver(chunk);
        }
    }
}

bool ChatSession::SendChannelMessageFrom(const std::string& session_guid,
                                         uint32_t channel,
                                         const std::string& text) {
    if (session_guid.empty() || text.empty()) return false;
    PruneExpiredSessions();
    for (auto& weak : g_chat_sessions) {
        if (auto peer = weak.lock()) {
            if (peer->get_session_guid() != session_guid) continue;
            if (peer->get_player_name().empty()) return false;
            if (std::string gag; Social::Friends::IsGagged(peer->get_player_name(), &gag)) { peer->deliver(BuildChatFrame(kSystemChannelId, gag)); return true; }
            peer->SendOnChannel(channel, peer->get_player_name() + ": " + text);
            return true;
        }
    }
    return false;
}

void ChatSession::AgencyMessage(int64_t agency_id, const std::string& text) {
    if (agency_id == 0 || text.empty()) return;
    const auto ids = Database::GetAgencyMemberUserIds(agency_id);
    const std::unordered_set<int64_t> members(ids.begin(), ids.end());
    auto frame = BuildChatFrame(kAgencyChannelId, text);
    PruneExpiredSessions();
    size_t delivered = 0;
    for (auto& weak : g_chat_sessions) {
        auto peer = weak.lock();
        if (!peer) continue;
        const std::string& guid = peer->get_session_guid();
        if (guid.empty()) continue;
        auto info = PlayerSessionStore::GetByGuid(guid);
        if (info && members.count(info->user_id)) { peer->deliver(frame); delivered++; }
    }
    Logger::Log("chat", "[Chat] agency msg agency=%lld text='%s' delivered to %zu session(s)\n",
        (long long)agency_id, text.c_str(), delivered);
}

void ChatSession::DeliverFrameToUsers(const std::vector<int64_t>& user_ids, const std::vector<uint8_t>& frame) {
    const std::unordered_set<int64_t> wanted(user_ids.begin(), user_ids.end());
    PruneExpiredSessions();
    for (auto& weak : g_chat_sessions) {
        auto peer = weak.lock();
        if (!peer || peer->get_session_guid().empty()) continue;
        auto info = PlayerSessionStore::GetByGuid(peer->get_session_guid());
        if (info && wanted.count(info->user_id)) peer->deliver(frame);
    }
}

void ChatSession::SystemMessageToGuid(const std::string& session_guid,
                                      const std::string& text) {
    if (session_guid.empty()) return;
    auto frame = BuildChatFrame(kSystemChannelId, text);
    for (auto& weak : g_chat_sessions) {
        if (auto peer = weak.lock()) {
            if (peer->get_session_guid() == session_guid) {
                peer->deliver(frame);
                return;
            }
        }
    }
}

void ChatSession::SystemMsgIdToGuid(const std::string& session_guid, uint32_t msg_id,
                                    const std::vector<MsgField>& fields) {
    if (session_guid.empty()) return;
    std::vector<uint8_t> b;
    const auto put2 = [&](uint32_t v) { b.push_back(v & 0xFF); b.push_back((v >> 8) & 0xFF); };
    const auto put4 = [&](uint32_t v) { put2(v & 0xFFFF); put2(v >> 16); };
    put2(GA_U::CHAT_MESSAGE);
    put2((uint32_t)fields.size() + 3);
    put2(GA_T::CHAT_CH_TYPE); put4(kSystemChannelId);
    put2(GA_T::CHAT_ANNOUNCEMENT_FLAG); b.push_back(1);
    put2(GA_T::MSG_ID); put4(msg_id);
    for (const auto& f : fields) {
        put2(f.type);
        if (f.kind == 's') { put2((uint32_t)f.s.size()); b.insert(b.end(), f.s.begin(), f.s.end()); }
        else put4(f.n);
    }
    std::vector<uint8_t> frame;
    frame.push_back(b.size() & 0xFF); frame.push_back((b.size() >> 8) & 0xFF);
    frame.insert(frame.end(), b.begin(), b.end());
    for (auto& weak : g_chat_sessions) {
        if (auto peer = weak.lock()) {
            if (peer->get_session_guid() == session_guid) {
                peer->deliver(frame);
                return;
            }
        }
    }
}

std::vector<std::string> ChatSession::OnlinePlayerNames() {
    std::vector<std::string> names;
    for (auto& weak : g_chat_sessions) {
        if (auto peer = weak.lock()) {
            if (!peer->player_name_.empty()) names.push_back(peer->player_name_);
        }
    }
    std::sort(names.begin(), names.end());
    names.erase(std::unique(names.begin(), names.end()), names.end());
    return names;
}

int64_t ChatSession::SessionSecondsForGuid(const std::string& session_guid) {
    for (auto& weak : g_chat_sessions) {
        if (auto peer = weak.lock()) {
            if (peer->session_guid_ == session_guid && peer->bound_at_)
                return static_cast<int64_t>(std::time(nullptr)) - peer->bound_at_;
        }
    }
    return -1;
}

bool ChatSession::is_ignoring(const std::string& sender_name) {
    const uint64_t epoch = Database::FriendsEpoch();
    if (epoch != ignore_epoch_) {
        ignored_lower_.clear();
        for (auto& name : Database::GetIgnoredNames(player_name_))
            ignored_lower_.insert(std::move(name));
        ignore_epoch_ = epoch;
    }
    if (ignored_lower_.empty()) return false;
    std::string lower = sender_name;
    std::transform(lower.begin(), lower.end(), lower.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ignored_lower_.count(lower) != 0;
}

void ChatSession::SetDevChatCommands(bool enabled) {
    g_dev_chat_commands = enabled;
    Logger::Log("chat", "[Chat] dev chat commands %s\n", enabled ? "ENABLED (dev_controls)" : "disabled");
}

void ChatSession::SetLoginNotice(const std::string& text) {
    g_login_notice = text;
}

void ChatSession::SetNewsFile(const std::string& path) {
    g_news_file = path;
}

bool ChatSession::SendNewsToGuid(const std::string& session_guid, std::string* headline) {
    constexpr size_t kMaxPosts = 5;
    if (session_guid.empty() || g_news_file.empty()) return false;
    std::ifstream in(g_news_file);
    if (!in) return false;
    nlohmann::json j = nlohmann::json::parse(in, nullptr, /*allow_exceptions=*/false);
    if (!j.is_object() || !j.contains("posts") || !j["posts"].is_array() || j["posts"].empty()) return false;

    std::vector<std::vector<uint8_t>> frames;
    frames.push_back(BuildChatFrame(kFeedChannelId,
        "=== " + j.value("title", std::string("Global News")) + " ==="));
    for (const auto& post : j["posts"]) {
        if (frames.size() > kMaxPosts) break;
        if (!post.is_object()) continue;
        std::string text = post.value("text", std::string());
        if (text.empty()) continue;
        if (headline && headline->empty()) *headline = text;
        std::string date = post.value("date", std::string());
        frames.push_back(BuildChatFrame(kFeedChannelId, date.empty() ? text : "[" + date + "] " + text));
    }
    for (auto& weak : g_chat_sessions) {
        if (auto peer = weak.lock()) {
            if (peer->get_session_guid() == session_guid) {
                for (auto& f : frames) peer->deliver(f);
                return true;
            }
        }
    }
    return false;
}

namespace {
// One leaderboard line: "<label>: 1. name 12  2. name 9 ..." from a query that
// returns (name, value) rows, best first.
std::string HallOfLegendsLine(sqlite3* db, const char* label, const char* sql) {
    std::string line = std::string(label) + ":";
    sqlite3_stmt* s = nullptr;
    int n = 0;
    if (sqlite3_prepare_v2(db, sql, -1, &s, nullptr) == SQLITE_OK) {
        while (sqlite3_step(s) == SQLITE_ROW) {
            const unsigned char* nm = sqlite3_column_text(s, 0);
            if (!nm) continue;
            ++n;
            line += "  " + std::to_string(n) + ". " + reinterpret_cast<const char*>(nm) +
                    " " + std::to_string(sqlite3_column_int64(s, 1));
        }
    }
    sqlite3_finalize(s);
    if (n == 0) line += " no records yet";
    return line;
}
}

bool ChatSession::SendHallOfLegendsToGuid(const std::string& session_guid) {
    if (session_guid.empty()) return false;
    sqlite3* db = Database::GetConnection();
    if (!db) return false;
    // real accounts only (zz* = test accounts, service_account = server-run AvA bot agents); live characters only
    #define HOL_USERS "u.username NOT LIKE 'zz%' AND COALESCE(u.service_account, 0) = 0 AND c.deleted_at = 0"
    std::vector<std::vector<uint8_t>> frames;
    frames.push_back(BuildChatFrame(kFeedChannelId, "=== AvA Hall of Legends ==="));
    frames.push_back(BuildChatFrame(kFeedChannelId, HallOfLegendsLine(db, "AvA season winners",  /* 1.39 hall shows past season winners */
        "SELECT 'Season ' || s.season || ' ' || COALESCE(s.owner_name, '?') || ', territories', s.territories FROM ga_ava_seasons s "  /* winner per season */
        "WHERE s.territories > 0 AND s.owner_agency_id = (SELECT x.owner_agency_id FROM ga_ava_seasons x WHERE x.season = s.season "
        "AND x.territories > 0 ORDER BY x.networth DESC, x.territories DESC, x.owner_agency_id LIMIT 1) ORDER BY s.season DESC LIMIT 5")));
    frames.push_back(BuildChatFrame(kFeedChannelId, HallOfLegendsLine(db, "Highest level",
        "SELECT u.username, MAX(COALESCE((SELECT MAX(l.level) FROM asm_data_set_xp_levels l "
        "WHERE l.xp_value <= c.xp), 1)) AS v FROM ga_characters c JOIN ga_users u ON u.id = c.user_id "
        "WHERE " HOL_USERS " GROUP BY u.id ORDER BY v DESC, MAX(c.xp) DESC LIMIT 5")));
    frames.push_back(BuildChatFrame(kFeedChannelId, HallOfLegendsLine(db, "Missions won",
        "SELECT u.username, COUNT(DISTINCT i.id) AS v FROM ga_match_player_stats m "
        "JOIN ga_instances i ON i.instance_id = m.instance_id JOIN ga_characters c ON c.id = m.character_id "
        "JOIN ga_users u ON u.id = c.user_id WHERE i.is_home_map = 0 "
        "AND i.winning_task_force = m.task_force AND " HOL_USERS
        " GROUP BY u.id ORDER BY v DESC LIMIT 5")));
    frames.push_back(BuildChatFrame(kFeedChannelId, HallOfLegendsLine(db, "Enemies defeated",
        "SELECT u.username, SUM(m.bot_kills) AS v FROM ga_match_player_stats m "
        "JOIN ga_characters c ON c.id = m.character_id JOIN ga_users u ON u.id = c.user_id "
        "WHERE " HOL_USERS " GROUP BY u.id HAVING v > 0 ORDER BY v DESC LIMIT 5")));
    frames.push_back(BuildChatFrame(kFeedChannelId, HallOfLegendsLine(db, "PvP kills",
        "SELECT u.username, SUM(m.kills) AS v FROM ga_match_player_stats m "
        "JOIN ga_characters c ON c.id = m.character_id JOIN ga_users u ON u.id = c.user_id "
        "WHERE " HOL_USERS " GROUP BY u.id HAVING v > 0 ORDER BY v DESC LIMIT 5")));
    frames.push_back(BuildChatFrame(kFeedChannelId, HallOfLegendsLine(db, "AvA wins",
        "SELECT u.username, SUM(r.wins) AS v FROM ga_ava_records r "
        "JOIN ga_characters c ON c.id = r.character_id JOIN ga_users u ON u.id = c.user_id "
        "WHERE " HOL_USERS " GROUP BY u.id HAVING v > 0 ORDER BY v DESC, SUM(r.captures) DESC LIMIT 5")));
    #undef HOL_USERS
    for (auto& weak : g_chat_sessions) {
        if (auto peer = weak.lock()) {
            if (peer->get_session_guid() == session_guid) {
                for (auto& f : frames) peer->deliver(f);
                return true;
            }
        }
    }
    return false;
}

void ChatSession::SetAnnouncers(const std::vector<std::string>& names) {
    g_announcers.clear();
    for (const auto& n : names) {
        if (!n.empty()) g_announcers.insert(ToLowerAscii(n));
    }
    Logger::Log("chat", "[Chat] announcer list loaded: %zu name(s)\n", g_announcers.size());
}

std::optional<std::unordered_set<std::string>> ChatSession::ChannelScope(uint32_t channel, const std::string& guid) { return RecipientsFor(channel, guid); }

bool ChatSession::IsAnnouncer() const {
    if (g_announcers.empty() || player_name_.empty()) return false;
    return g_announcers.count(ToLowerAscii(player_name_)) != 0;
}

bool ChatSession::DeliverToPlayer(const std::string& player_name, const std::vector<uint8_t>& frame) {
    if (player_name.empty()) return false;
    for (auto& weak : g_chat_sessions)
        if (auto peer = weak.lock())
            if (peer->get_player_name() == player_name) { peer->deliver(frame); return true; }
    return false;
}

void ChatSession::SystemMessageTo(const std::string& player_name, const std::string& text) {
    if (player_name.empty()) return;
    auto frame = BuildChatFrame(kSystemChannelId, text);
    for (auto& weak : g_chat_sessions) {
        if (auto peer = weak.lock()) {
            if (peer->get_player_name() == player_name) {
                peer->deliver(frame);
                return;
            }
        }
    }
}

// Voice channel membership. Every 2 s the session works out where the
// player is (party, instance + side, the instance's map, agency) and, when
// that changed, asks the "voip_config" ServiceHooks service what to send;
// its answer goes to the client as VOIP_CONFIG. The call runs on a worker
// thread and the reply is handled back on this session's executor. No
// service, "pass" or an error -> nothing is sent (retried after 10 s).
void ChatSession::VoiceTick() {
    if (closed_ || session_guid_.empty()) return;
    auto info = PlayerSessionStore::GetByGuid(session_guid_);
    if (info) {
        nlohmann::json m;
        m["player"] = player_name_;
        m["ip"] = info->ip_address;
        m["user_id"] = info->user_id;
        m["character_id"] = info->selected_character_id;
        int64_t inst = 0; int tf = 0; std::string map;
        if (auto where = InstanceRegistry::GetInstancePlayerTaskForce(session_guid_)) {
            inst = where->first; tf = where->second;
            if (auto ii = InstanceRegistry::GetInstanceById(inst)) map = ii->map_name;
        }
        m["instance_id"] = inst;
        m["task_force"] = tf;
        m["map"] = map;
        m["team_id"] = TeamService::GetTeamId(session_guid_);
        if (voice_ticks_++ % 5 == 0)
            voice_agency_ = info->user_id ? Database::GetAgencyIdForUser(info->user_id) : 0;
        m["agency_id"] = voice_agency_;
        const std::string key = m.dump();
        const int64_t now = static_cast<int64_t>(std::time(nullptr));
        if (!voice_busy_ && key != voice_key_ && now >= voice_retry_at_) {
            voice_busy_ = true;
            nlohmann::json data = m;
            data["guid"] = session_guid_;
            data["first"] = !voice_sent_;
            std::weak_ptr<ChatSession> weak = shared_from_this();
            auto ex = socket_.get_executor();
            std::thread([weak, ex, data, key]() {
                auto decision = ServiceHooks::Call("voip_config", data);
                asio::post(ex, [weak, decision, key]() {
                    if (auto self = weak.lock()) self->OnVoiceConfig(decision, key);
                });
            }).detach();
        }
    }
    auto self(shared_from_this());
    voice_timer_.expires_after(std::chrono::seconds(2));
    voice_timer_.async_wait([this, self](std::error_code ec) {
        if (!ec) VoiceTick();
    });
}

void ChatSession::OnVoiceConfig(const std::optional<nlohmann::json>& decision, const std::string& key) {
    voice_busy_ = false;
    if (closed_) return;
    if (!decision) {
        voice_retry_at_ = static_cast<int64_t>(std::time(nullptr)) + 10;
        return;
    }
    voice_key_ = key;
    auto frame = BuildVoipConfigFrame(*decision);
    if (frame.empty()) return;
    deliver(frame);
    voice_sent_ = true;
    Logger::Log("chat", "[Voice] VOIP_CONFIG -> player='%s' bytes=%zu\n", player_name_.c_str(), frame.size());
}

void ChatSession::TearDown(const char* reason) {
    if (closed_) return;
    closed_ = true;
    bind_retry_timer_.cancel();
    voice_timer_.cancel();

    Logger::Log("chat",
        "[Chat] tearing down player='%s' guid=%s reason=%s queue=%zu sessions_before=%zu\n",
        player_name_.c_str(), session_guid_.c_str(), reason,
        write_queue_.size(), g_chat_sessions.size());

    // Announce departure to remaining peers if this player completed the
    // handshake (no point announcing an unidentified connection). Skip the
    // departing session itself — its socket is about to close, deliver()
    // would short-circuit anyway because closed_=true. Done BEFORE the
    // socket close so the broadcast loop still sees a clean state.
    if (announced_join_) AgencyChat::OnChatLeave(session_guid_, socket_.get_executor());
    if (announced_join_ && !player_name_.empty()) {
        BroadcastSystemMessage(
            "*** " + player_name_ + " has left the chat ***",
            kSystemChannelId,
            /*exclude=*/this);
        Social::Friends::OnOffline(player_name_, socket_.get_executor());
    }

    write_queue_.clear();

    std::error_code ignored;
    socket_.shutdown(asio::ip::tcp::socket::shutdown_both, ignored);
    socket_.close(ignored);

    // Drop this session's entry and any other expired weak_ptrs in one pass.
    g_chat_sessions.erase(
        std::remove_if(g_chat_sessions.begin(), g_chat_sessions.end(),
            [this](const std::weak_ptr<ChatSession>& w) {
                auto sp = w.lock();
                return !sp || sp.get() == this;
            }),
        g_chat_sessions.end());

    Logger::Log("chat", "[Chat] teardown complete, sessions_after=%zu\n",
        g_chat_sessions.size());
}
