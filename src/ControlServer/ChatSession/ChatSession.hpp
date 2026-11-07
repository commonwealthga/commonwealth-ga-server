#pragma once

#include <asio.hpp>
#include <vector>
#include <deque>
#include <string>
#include <memory>
#include <sstream>
#include <iomanip>
#include <unordered_set>
#include <optional>
#include <cstring>

#include "src/ControlServer/Constants/TcpFunctions.h"
#include "src/ControlServer/Constants/TcpTypes.h"
#include "src/ControlServer/TcpSession/PacketView.hpp"
#include "src/ControlServer/Logger.hpp"
#include "src/ControlServer/PlayerSessionStore/PlayerSessionStore.hpp"
#include "src/ControlServer/ChatSession/ChatCommand.hpp"
#include "lib/nlohmann/json.hpp"

class ChatSession : public std::enable_shared_from_this<ChatSession> {
public:
    ChatSession(asio::ip::tcp::socket socket);
    void start();
    void deliver(const std::vector<uint8_t>& msg);
    const std::string& get_player_name() const { return player_name_; }
    const std::string& get_session_guid() const { return session_guid_; }
    // True when this session's player ignores `sender_name`. Backed by a
    // per-session cached ignore set, reloaded only when
    // Database::FriendsEpoch() has changed since the last check.
    bool is_ignoring(const std::string& sender_name);

    // Deliver a system chat line to one player's chat session, if they have
    // one. Used by TcpSession (e.g. friends "player not found" feedback).
    static void SystemMessageTo(const std::string& player_name, const std::string& text);
    // Deliver a prebuilt chat frame to this player's chat socket (false = not connected).
    static bool DeliverToPlayer(const std::string& player_name, const std::vector<uint8_t>& frame);

    // Same, addressed by session guid — the main TCP socket knows guids, not
    // player names.
    static void SystemMessageToGuid(const std::string& session_guid,
                                    const std::string& text);

    // A localised line from the client's message table, by id, on the system channel.
    // Fields carry the values its text asks for; the client formats and colours it,
    // plays the sound linked to the id and hands item messages to its ACQUIRED banner
    // (TgUIReceivedItemScreen, filter 0x11498a80: id in its list + ITEM_ID > 0).
    struct MsgField {
        uint16_t type = 0;
        char kind = 'u';  // 'u' 4-byte int, 'f' 4-byte float, 's' string
        uint32_t n = 0;
        std::string s;
        static MsgField U(uint16_t t, int64_t v) { MsgField f; f.type = t; f.n = (uint32_t)v; return f; }
        static MsgField F(uint16_t t, int64_t v) { MsgField f; f.type = t; f.kind = 'f'; float x = (float)v; std::memcpy(&f.n, &x, 4); return f; }
        static MsgField S(uint16_t t, std::string v) { MsgField f; f.type = t; f.kind = 's'; f.s = std::move(v); return f; }
    };
    static void SystemMsgIdToGuid(const std::string& session_guid, uint32_t msg_id,
                                  const std::vector<MsgField>& fields);

    // Push a line onto the green Agency channel for every online member of
    // `agency_id`. Used by TcpSession for the join/leave/kick notices; the
    // membership row of the player being announced must already be written
    // (join) or deleted (leave) so the audience matches the new roster.
    static void AgencyMessage(int64_t agency_id, const std::string& text);
    // Deliver a prebuilt chat frame to every chat session of these accounts.
    static void DeliverFrameToUsers(const std::vector<int64_t>& user_ids, const std::vector<uint8_t>& frame);

    // Send `text` as a chat line on `channel` from the player owning
    // `session_guid`, with the same recipient scoping an ordinary chat message
    // gets. Used by the PLAYER_COMMAND (0x019F) slash-command handler, which
    // arrives on the main TCP socket rather than the chat socket. Returns false
    // when that player has no bound chat session.
    static bool SendChannelMessageFrom(const std::string& session_guid,
                                       uint32_t channel,
                                       const std::string& text);

    // Names permitted to use -announce. Set once at startup from
    // ControlServerConfig::announcers; empty means the command is disabled.
    static void SetAnnouncers(const std::vector<std::string>& names);
    // System line shown to each player on join ("" = off).
    static void SetLoginNotice(const std::string& text);
    // Global News stand. SendNewsToGuid shows the news file
    // (title + newest posts) as yellow lines to one player; false when the
    // file is missing/empty/invalid or the player has no chat session.
    static void SetNewsFile(const std::string& path);
    static bool SendNewsToGuid(const std::string& session_guid, std::string* headline = nullptr);
    // AvA Hall of Legends POI. Top agents from our DB (level,
    // missions won, enemies killed, PvP kills; zz test accounts skipped) as
    // yellow lines to one player; empty boards say so.
    static bool SendHallOfLegendsToGuid(const std::string& session_guid);
    // Dev/cheat chat commands only when config dev_controls is on.
    static void SetDevChatCommands(bool enabled);
    // Player commands (-who / -played): bound player names of all
    // chat sessions, and seconds since this guid's chat session bound (-1 = none).
    static std::vector<std::string> OnlinePlayerNames();
    static std::optional<std::unordered_set<std::string>> ChannelScope(uint32_t channel, const std::string& guid);   // (RecipientsFor)
    static int64_t SessionSecondsForGuid(const std::string& session_guid);

private:
    asio::ip::tcp::socket socket_;
    asio::steady_timer bind_retry_timer_;
    // Voice: membership re-check timer + last membership the voip_config
    // service answered for (see VoiceTick in ChatSession.cpp).
    asio::steady_timer voice_timer_;
    std::string voice_key_;
    bool voice_busy_ = false;
    bool voice_sent_ = false;
    unsigned voice_ticks_ = 0;
    int64_t voice_agency_ = 0;
    int64_t voice_retry_at_ = 0;
    // Scratch buffer for one async_read_some call. TCP is a byte stream, so a
    // single read can return a partial packet, exactly one packet, or several
    // packets concatenated — appended into `accumulator_` and drained from
    // there in do_read(). On loopback small payloads happen to arrive 1:1, so
    // the unframed code worked locally but broke between remote clients.
    std::vector<uint8_t> read_buf_;
    std::vector<uint8_t> accumulator_;
    std::deque<std::vector<uint8_t>> write_queue_;
    std::string player_name_;
    std::string session_guid_;
    // Cached ignore list (lowercased names) + the friends-epoch it was built
    // at. 0 = never loaded (Database epoch starts at 1).
    std::unordered_set<std::string> ignored_lower_;
    uint64_t ignore_epoch_ = 0;
    int bind_retry_attempts_ = 0;
    bool announced_join_ = false;
    int64_t bound_at_ = 0;
    // Single-shot lifecycle guard. Set by TearDown(); checked in every async
    // callback so a session that's already been torn down (e.g. by a write
    // error) is inert if a stale callback fires later. Without this the read
    // and write error paths could double-remove from g_chat_sessions or
    // double-broadcast the leave announcement.
    bool closed_ = false;

    template<typename... Bytes>
    void append(std::vector<uint8_t>& buffer, Bytes&&... bytes) {
        (buffer.push_back(static_cast<uint8_t>(bytes)), ...);
    }

    void WriteString(std::vector<uint8_t>& buffer, uint16_t type, const std::string& str) {
        append(buffer, type & 0xFF, type >> 8);
        uint16_t len = static_cast<uint16_t>(str.size());
        append(buffer, len & 0xFF, len >> 8);
        buffer.insert(buffer.end(), str.begin(), str.end());
    }

    void Write4B(std::vector<uint8_t>& buffer, uint16_t type, uint32_t val) {
        append(buffer, type & 0xFF, type >> 8);
        append(buffer, val & 0xFF, (val >> 8) & 0xFF, (val >> 16) & 0xFF, (val >> 24) & 0xFF);
    }

    // True when this session's player is in the configured announcer list.
    bool IsAnnouncer() const;

    void do_read();
    void do_write();
    void handle_packet(const uint8_t* data, size_t length);
    void broadcast(const uint8_t* data, size_t length);
    // Scope + build + deliver one formatted chat line. `message` is the final
    // MESSAGE field ("<name>: <text>"), already prefixed by the caller.
    void SendOnChannel(uint32_t channel, const std::string& message);
    bool BindPlayerFromRegistry(const char* reason);
    void ScheduleBindRetry();
    void AnnounceJoinIfNeeded();
    void VoiceTick();
    void OnVoiceConfig(const std::optional<nlohmann::json>& decision, const std::string& key);

    // -spectate handling. instance_id==0 -> list running (non-home-map)
    // instances the caller may join; instance_id!=0 -> request to join that
    // instance as a spectator, optionally cosmetically assigned to `team`
    // (None = today's teamless behavior) so that team's health bars render
    // client-side. Permission-gated on ga_user_roles "spectator" (checked
    // here, control-server-side — never trusts anything client-supplied).
    // Replies privately to this session only via deliver().
    void HandleSpectateCommand(int64_t instance_id, ChatCommand::SpectateTeam team);

    // -unspectate handling. No permission check needed here (unlike
    // -spectate) — TcpSession::DeliverSpectateExit itself rejects any session
    // that isn't currently spectating, which is the only gate that matters.
    // Replies privately to this session only via deliver().
    void HandleUnspectateCommand();

    // -components — inventory-desync test harness. Report prints the three
    // numbers that must agree; grant/take drive the real SEND_INVENTORY paths.
    void HandleComponentsCommand(const ChatCommand::ParseResult::ComponentsArgs& args);

    // Single tear-down path. Idempotent via `closed_`. On entry, optionally
    // broadcasts a "<player> has left the chat" system message if the session
    // was handshaken, then clears the write queue, closes the socket, and
    // removes self from g_chat_sessions so further broadcasts skip it.
    void TearDown(const char* reason);
};
