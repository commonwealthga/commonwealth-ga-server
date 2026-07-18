#include "src/ControlServer/ChatSession/ChatSession.hpp"
#include "src/ControlServer/ChatSession/ChatCommand.hpp"
#include "src/ControlServer/Database/Database.hpp"
#include "src/ControlServer/MatchmakingService/MatchmakingService.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#ifndef _WIN32
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <cerrno>
#endif

// All chat sessions — safe without mutex since io_context runs single-threaded.
// Pruned lazily (broadcast() drops expired weak_ptrs before iterating) and
// eagerly (TearDown removes its own entry).
static std::vector<std::weak_ptr<ChatSession>> g_chat_sessions;

namespace {

// Wire-level channel id used for join/leave system announcements. Chosen to
// be distinct from the gameplay-chat channels the client receives (channel 4
// = global, channel 8 = DM). Real clients render unknown channels as a
// default-tinted chat line so the message still appears even if the client
// has no special styling for this id; the "*** ... ***" decoration in the
// message body guarantees visibility regardless. If a future client patch
// adds a dedicated system-tint channel, change here and rebuild.
constexpr uint32_t kSystemChannelId = 4;

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
        || parsed.spawn_target.has_value()
        || parsed.deploy_target.has_value()
        || parsed.topdown.has_value()
        || parsed.possess
        || parsed.unpossess
        || parsed.coords
        || parsed.fullheal
        || parsed.reload_queues;
}

} // anonymous namespace

ChatSession::ChatSession(asio::ip::tcp::socket socket)
    : socket_(std::move(socket)),
      bind_retry_timer_(socket_.get_executor()),
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
    BroadcastSystemMessage(
        "*** " + player_name_ + " has joined the chat ***",
        kSystemChannelId,
        /*exclude=*/nullptr);
}

bool ChatSession::BindPlayerFromRegistry(const char* reason) {
    if (closed_ || session_guid_.empty()) return false;

    auto info = PlayerSessionStore::GetByGuid(session_guid_);
    if (!info) return false;

    player_name_ = info->player_name;
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
        if (parsed.recognized && parsed.topdown) {
            ChatCommand::DispatchTopDown(*parsed.topdown, session_guid_);
        }
        if (parsed.recognized && parsed.reload_queues) {
            Logger::Log("chat-command",
                "[ChatCmd] -reload-queues player='%s' guid=%s outcome=activated details=MatchmakingService::ReloadQueues\n",
                player_name_.c_str(), session_guid_.c_str());
            MatchmakingService::ReloadQueues();
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

void ChatSession::broadcast(const uint8_t* data, size_t length) {
    std::vector<uint8_t> response;

    PacketView pkt(data + 6, length - 6);

    uint32_t channel = pkt.Read4B(GA_T::CHAT_CH_TYPE).value_or(4);

    std::string message = player_name_ + ": " + pkt.ReadString(GA_T::MESSAGE).value_or("");

    bool isDM = channel == 8;

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
        // Per-side frames. Client renders NAME (0x370) as "<name> says:" (or
        // MESSAGE verbatim when NAME is empty — used for the sender echo) and
        // caches PLAYER_NAME (0x3C5) as the reply-keybind target.
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
        if (target.get() != this)
            deliver(makeWhisperFrame("", "To " + targetName + ": " + text, targetName));
        return;
    }

    Logger::Log("chat", "Broadcasting message '%s' to channel %u\n", message.c_str(), channel);

    uint16_t packet_type = GA_U::CHAT_MESSAGE;
    uint16_t item_count = 2;

    append(response, packet_type & 0xFF, packet_type >> 8);
    append(response, item_count & 0xFF, item_count >> 8);

    Write4B(response, GA_T::CHAT_CH_TYPE, channel);
    WriteString(response, GA_T::MESSAGE, message);

    std::vector<uint8_t> chunk;
    uint16_t chunk_size = static_cast<uint16_t>(response.size());

    chunk.push_back(chunk_size & 0xFF);
    chunk.push_back(chunk_size >> 8);

    chunk.insert(chunk.end(), response.begin(), response.end());

    // Sweep expired entries on every broadcast — keeps the loop tight and
    // ensures stale weak_ptrs from prior tear-downs don't accumulate.
    PruneExpiredSessions();

    for (auto& weak : g_chat_sessions) {
        if (auto peer = weak.lock()) {
            if (isDM) {
                if (peer.get() == this) {
                    peer->deliver(chunk);
                }
            } else {
                // Ignore gate: don't show this sender's lines to players who
                // have them on the F3 ignore list.
                if (peer.get() != this &&
                    peer->is_ignoring(player_name_)) {
                    continue;
                }
                peer->deliver(chunk);
            }
        }
    }
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

void ChatSession::TearDown(const char* reason) {
    if (closed_) return;
    closed_ = true;
    bind_retry_timer_.cancel();

    Logger::Log("chat",
        "[Chat] tearing down player='%s' guid=%s reason=%s queue=%zu sessions_before=%zu\n",
        player_name_.c_str(), session_guid_.c_str(), reason,
        write_queue_.size(), g_chat_sessions.size());

    // Announce departure to remaining peers if this player completed the
    // handshake (no point announcing an unidentified connection). Skip the
    // departing session itself — its socket is about to close, deliver()
    // would short-circuit anyway because closed_=true. Done BEFORE the
    // socket close so the broadcast loop still sees a clean state.
    if (announced_join_ && !player_name_.empty()) {
        BroadcastSystemMessage(
            "*** " + player_name_ + " has left the chat ***",
            kSystemChannelId,
            /*exclude=*/this);
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
