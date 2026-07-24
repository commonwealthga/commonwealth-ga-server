#pragma once

#include <asio.hpp>
#include <vector>
#include <deque>
#include <string>
#include <memory>
#include <random>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <cstring>
#include <thread>
#include <mutex>
#include <map>

#include "src/ControlServer/Constants/GameTypes.h"
#include "src/ControlServer/Constants/TcpFunctions.h"
#include "src/ControlServer/Constants/TcpTypes.h"
#include "src/ControlServer/Constants/EquipSlot.hpp"
#include "src/ControlServer/TcpSession/PacketView.hpp"
#include "src/ControlServer/Logger.hpp"
#include "src/ControlServer/PlayerSessionStore/PlayerSessionStore.hpp"
#include "src/ControlServer/IpcServer/IpcServer.hpp"
#include "src/ControlServer/TeamService/TeamService.hpp"
#include "src/ControlServer/InstanceRegistry/InstanceRegistry.hpp"
#include "src/ControlServer/Database/Database.hpp"
#include "src/Shared/HexUtils.hpp"
#include "lib/nlohmann/json.hpp"
#include "src/ControlServer/MatchmakingService/MatchmakingService.hpp"

class TcpSession : public std::enable_shared_from_this<TcpSession> {
public:
    TcpSession(asio::ip::tcp::socket socket, asio::io_context& io)
        : socket_(std::move(socket)), io_ctx_(io) {
        data_.resize(1024);
        socket_.set_option(asio::ip::tcp::no_delay(true));
    }

    void start() {
        EnableKeepAlive();
        do_read();
    }

    // Enable SO_KEEPALIVE + tuned TCP_KEEPIDLE/INTVL/CNT on socket_. Without
    // this, a client that vanishes without an RST (BSOD, force-kill, sleep,
    // NAT drop) leaves do_read() blocked indefinitely -- so the matching
    // PlayerSessionStore entry persists and the next login of the same name
    // is rejected as "duplicate account." With keepalive, the OS times the
    // dead peer out in ~25s and do_read's error branch runs the normal
    // cleanup (Unregister, FinalizeSession, ClearPendingAck, etc.).
    void EnableKeepAlive();

    // ── TcpSession registry (session_guid -> weak_ptr<TcpSession>) ───────────

    static void RegisterSession(const std::string& guid, std::weak_ptr<TcpSession> session);
    static void UnregisterSession(const std::string& guid);

    // Dispatch a GAME_EVENT JSON payload to the TcpSession identified by session_guid.
    static void DeliverGameEvent(const std::string& session_guid, const nlohmann::json& j);

    // Deliver a MATCH_INVITATION to this session (called from matchmaking callback).
    static void DeliverMatchInvitation(const std::string& session_guid, int64_t instance_id, const std::string& game_mode, int task_force);

    // Deliver a PLAYER_ACTION IPC to the game-DLL instance the player is currently assigned to.
    // Returns true if dispatched, false if the session is unknown or the player is not assigned to an instance.
    static bool DeliverPlayerAction(const std::string& session_guid, const nlohmann::json& payload);

    // Admin dashboard move/team-change helper. Same-instance moves use the existing
    // in-instance change_team action; cross-instance moves reuse PLAYER_REGISTER +
    // GO_PLAY routing.
    static bool AdminMovePlayerToInstance(const std::string& session_guid,
                                          int64_t target_instance_id,
                                          int target_task_force,
                                          int64_t source_instance_id,
                                          int source_task_force,
                                          std::string& message);

    // Route a player out of an ended mission without forcing live
    // NetConnection cleanup, which is crash-prone inside UE3.
    static bool ForceMissionExit(const std::string& session_guid,
                                 int64_t parent_instance_id,
                                 const char* reason);

    // Clear the per-session queue + pending-match state on the named session.
    // Called when a matchmade instance dies before delivering MATCH_INVITATION
    // (spawn failure or pre-READY crash) — without this the session stays
    // wedged with current_match_queue_id_ != 0 and the next GET_TICKET_INFO
    // shows the player as still queued. Silent if guid is unknown.
    static void DeliverMatchCancelled(const std::string& session_guid, const char* reason);

    // Team wire pushes (TeamService). Resolve guid under sessions_mutex_.
    // 0x4A routes to the client's message-display handler (MSG_ID template +
    // @@token@@ fields), 0x4B queues the invitation popup, 0x4C dismisses it.
    static void DeliverTeamSystemMessage(const std::string& session_guid,
                                         uint32_t msg_id, const std::string& player_name);
    static void DeliverTeamInvitation(const std::string& session_guid,
                                      const std::string& leader_name);
    static void DeliverTeamInviteExpired(const std::string& session_guid);
    // TEAM_UPDATE (0x51) roster. Empty rows = "you are not in a team".
    static void DeliverTeamUpdate(const std::string& session_guid,
                                  const TeamRoster& roster);
    // Set a session's current queue + push the AgentInfo HUD "IN QUEUE" frame
    // (queue_id=0 clears it). Used to mirror team-queue state onto each member.
    static void DeliverMatchQueueStatus(const std::string& session_guid,
                                        uint32_t queue_id, uint32_t name_msg_id);

private:
    // ── Static registry members ──────────────────────────────────────────────
    static std::mutex sessions_mutex_;
    static std::map<std::string, std::weak_ptr<TcpSession>> g_sessions_;
    asio::ip::tcp::socket socket_;
    asio::io_context& io_ctx_;
    std::vector<uint8_t> data_;
    std::vector<uint8_t> rx_buffer_;

    // Outbound frames awaiting async_write. All writes MUST go through
    // enqueue_write: a blocking asio::write to a client that stops draining
    // its socket (e.g. wedged during map load) parks the single io_context
    // thread and takes chat/login/matchmaking down with it (2026-07-24
    // incident: one 121KB SEND_INVENTORY write blocked for 607s).
    std::deque<std::vector<uint8_t>> write_queue_;
    size_t write_queue_bytes_  = 0;
    bool   close_when_drained_ = false;
    // A client buffering this much unread data is not coming back — close it.
    static constexpr size_t kMaxWriteQueueBytes = 2 * 1024 * 1024;

    std::string player_name;
    std::string session_guid_;
    std::string ip_address_;
    int64_t  user_id_               = 0;
    int64_t  selected_character_id_ = 0;
    uint32_t selected_profile_id_   = 0;
    // ATgPawn_Character::r_nItemProfileId — required per-row key in the
    // skills response packet (FUN_1141f750 matches rows by this). Surfaced
    // from game server via spawn/skill_save IPC; default 0 until known.
    int32_t  item_profile_id_       = 0;
    uint64_t profile_refresh_token_ = 0;

	uint32_t current_match_queue_id_ = 0;

    // Set when a MATCH_INVITATION is delivered — the instance to join on MATCH_ACCEPT.
    int64_t pending_match_instance_id_ = 0;
    std::string pending_match_game_mode_;
    int pending_match_task_force_ = 1;
    uint64_t next_register_token_ = 1;
    uint64_t active_register_token_ = 0;

    // ACK-wait timer for PLAYER_REGISTER flow. Cancelled on ACK arrival or disconnect.
    std::shared_ptr<asio::steady_timer> pending_ack_timer_;

    // Static callback to spawn a home map instance on demand.
    static std::function<void()> on_need_home_map_;

    // Network config: external IP and chat port, set once from main.
    static std::string s_host_;
    static uint16_t    s_chat_port_;
    static bool        s_allow_duplicate_account_logins_;
    static bool        s_require_password_verification_;

    // Moderation knobs — pushed in from main.cpp at startup. Same static-setter
    // pattern as s_allow_duplicate_account_logins_ / SetLoginPolicy above.
    static std::string s_ban_spoof_mode_;                  // "silent" | "garbage"
    static int         s_ban_spoof_fallback_close_sec_;    // 0 = never
    static int         s_kick_fallback_close_sec_;         // 0 = never
public:
    static void SetHomeMapSpawner(std::function<void()> cb);
    static void SetNetworkConfig(const std::string& host, uint16_t chat_port);
    static void SetLoginPolicy(bool allow_duplicate_account_logins,
                               bool require_password_verification = true);
    static void SetModerationConfig(const std::string& ban_spoof_mode,
                                    int ban_spoof_fallback_close_sec,
                                    int kick_fallback_close_sec);

    // Live-kick every TcpSession whose user_id / IP matches the predicate.
    // Sends a bogus GSC_GO_PLAY pointing at a non-existent map; client tears
    // down its current session trying to switch, fails, and disconnects on
    // its own — the existing do_read error branch sweeps up.
    static int KickSessionsForUser(int64_t user_id);
    static int KickSessionsForIp  (const std::string& ip);

    // Tear down the session registered under guid: reap its instance-side
    // NetConnection (MSG_PLAYER_CLOSE), unregister it everywhere and close its
    // socket. Used by the re-login takeover (crashed client whose dead socket
    // hasn't been reaped yet). Returns 1 if a live TcpSession was closed, 0 if
    // only store entries were swept.
    static int EvictStaleSession(const std::string& guid, const char* reason);

    // True when a live TcpSession object is registered under guid. Used by the
    // periodic ghost-player reconciliation in main.cpp.
    static bool HasLiveSession(const std::string& guid);

    // Snapshot for the dashboard "Online now" panel.
    struct OnlineSnapshot {
        std::string session_guid;
        int64_t     user_id     = 0;
        std::string username;
        std::string ip;           // host portion only ("1.2.3.4", no port)
        int64_t     instance_id  = 0;
        int64_t     login_at     = 0;  // unix seconds
    };
    static void ForEachLiveSession(const std::function<void(const OnlineSnapshot&)>& visit);

    // Fire the home-map spawner callback IF no home instance currently exists.
    // Safe to call multiple times — `GetHomeInstance()` returns any non-STOPPED
    // home instance (STARTING or READY), so back-to-back calls won't double-
    // spawn. Used by login, end-of-mission warm-up (MSG_NEED_HOME_MAP IPC),
    // and the GSC_CHANGE_INSTANCE return-to-home handler.
    static void EnsureHomeMapWarm(const char* reason);
private:

    // Set when a READY home map instance is found for this player.
    int64_t assigned_instance_id_ = 0;
    int home_task_force_ = 1;

    // Row id in ga_user_sessions for this connection. 0 until set by the
    // GSC_USER_LOGIN handler; backfilled with logout_at when the socket
    // drops in do_read's error branch.
    int64_t session_row_id_   = 0;
    // Unix seconds — set alongside the session-row insert. Reported in the
    // ForEachLiveSession snapshot.
    int64_t session_login_at_ = 0;

    // The SESSION_GUID nonce we issued in the credential-less pre-login frame
    // (32-char hex, as GenerateSessionGuid emits). The client encrypts the
    // credential blob against this; the credentialed frame consumes it to
    // verify. Cleared after one use so a stale nonce can't be replayed.
    std::string pending_login_challenge_guid_;

    // Login-bug spoof: apply s_ban_spoof_mode_ to this session. Called from
    // the GSC_USER_LOGIN handler after a ban row is matched.
    void ApplyBanSpoof();

    // Bogus GSC_GO_PLAY for the live-kick path. Frame is valid, map name
    // is one no client will resolve — client falls into its local
    // disconnect path and closes the socket.
    void send_kick_to_bogus_map();

    // Safety backstop for the live-kick: if the client hasn't closed the
    // socket within s_kick_fallback_close_sec_ after the bogus GO_PLAY,
    // force-close it from our side.
    void arm_kick_fallback_timer();

    template<typename... Bytes>
    void append(std::vector<uint8_t>& buffer, Bytes&&... bytes) {
        (buffer.push_back(static_cast<uint8_t>(bytes)), ...);
    }

    void WriteTLV(std::vector<uint8_t>& buffer, uint16_t type, const uint8_t* value, size_t len) {
        append(buffer, type & 0xFF, type >> 8);
        buffer.insert(buffer.end(), value, value + len);
    }

    void Write1B(std::vector<uint8_t>& buffer, uint16_t type, uint8_t val) {
        uint8_t buf[] = {
            static_cast<uint8_t>(type & 0xFF),
            static_cast<uint8_t>(type >> 8),
            val
        };
        buffer.insert(buffer.end(), buf, buf + 3);
    }

    void Write2B(std::vector<uint8_t>& buffer, uint16_t type, uint16_t val) {
        uint8_t buf[] = {
            static_cast<uint8_t>(type & 0xFF), static_cast<uint8_t>(type >> 8),
            static_cast<uint8_t>(val & 0xFF), static_cast<uint8_t>(val >> 8)
        };
        buffer.insert(buffer.end(), buf, buf + 4);
    }

    void Write4B(std::vector<uint8_t>& buffer, uint16_t type, uint32_t val) {
        append(buffer, type & 0xFF, type >> 8);
        append(buffer, val & 0xFF, (val >> 8) & 0xFF, (val >> 16) & 0xFF, (val >> 24) & 0xFF);
    }

    void WriteString(std::vector<uint8_t>& buffer, uint16_t type, const std::string& str) {
        append(buffer, type & 0xFF, type >> 8);
        append(buffer, static_cast<uint8_t>(str.size()), 0x00);
        buffer.insert(buffer.end(), str.begin(), str.end());
    }

    void WriteWideString(std::vector<uint8_t>& buffer, uint16_t type, const std::wstring& str)
    {
        append(buffer, type & 0xFF, type >> 8);

        uint16_t len = static_cast<uint16_t>(str.size());
        append(buffer, len & 0xFF, len >> 8);

        for (wchar_t ch : str) {
            append(buffer, ch & 0xFF, (ch >> 8) & 0xFF);
        }
    }

    void WriteNBytes(std::vector<uint8_t>& buffer, uint16_t type, const std::vector<uint8_t>& bytes) {
        append(buffer, type & 0xFF, type >> 8);
        buffer.insert(buffer.end(), bytes.begin(), bytes.end());
    }

    void WriteNBytesRaw(std::vector<uint8_t>& buffer, const std::vector<uint8_t>& bytes) {
        buffer.insert(buffer.end(), bytes.begin(), bytes.end());
    }

    // void WriteFloat(std::vector<uint8_t>& buffer, uint16_t type, float val) {
    //     append(buffer, type & 0xFF, type >> 8);
    //     buffer.insert(buffer.end(), reinterpret_cast<uint8_t*>(&val), reinterpret_cast<uint8_t*>(&val) + sizeof(float));
    // }

    void WriteFloat(std::vector<uint8_t>& buffer, uint16_t type, float val)
    {
        append(buffer, type & 0xFF, type >> 8);

        // static_assert(sizeof(float) == 4, "Unexpected float size");

        uint32_t raw;
        std::memcpy(&raw, &val, sizeof(raw));

        append(buffer,
         raw & 0xFF,
         (raw >> 8) & 0xFF,
         (raw >> 16) & 0xFF,
         (raw >> 24) & 0xFF
         );
    }

    void WriteDouble(std::vector<uint8_t>& buffer, uint16_t type, double val)
    {
        append(buffer, type & 0xFF, type >> 8);

        // static_assert(sizeof(double) == 8, "Unexpected double size");

        uint64_t raw;
        std::memcpy(&raw, &val, sizeof(raw));

        append(buffer,
         raw & 0xFF,
         (raw >> 8) & 0xFF,
         (raw >> 16) & 0xFF,
         (raw >> 24) & 0xFF,
         (raw >> 32) & 0xFF,
         (raw >> 40) & 0xFF,
         (raw >> 48) & 0xFF,
         (raw >> 56) & 0xFF
         );
    }

    void WriteIP(std::vector<uint8_t>& response, uint16_t type, const std::string& ip, uint16_t port) {
        response.push_back(type & 0xFF);       // low byte
        response.push_back(type >> 8);         // high byte

        response.push_back(0x02);              // Type ID
        response.push_back(0x00);              // Subtype / reserved

        // Write port in big-endian
        response.push_back(port >> 8);         // High byte
        response.push_back(port & 0xFF);       // Low byte

        // Write IP as 4 bytes
        std::istringstream iss(ip);
        std::string token;
        while (std::getline(iss, token, '.')) {
            response.push_back(static_cast<uint8_t>(std::stoi(token)));
        }
    }

    static std::string GenerateSessionGuid() {
        std::mt19937 gen(std::random_device{}());
        std::uniform_int_distribution<unsigned int> dist(0, 255);
        uint8_t bytes[16];
        for (auto& b : bytes) b = static_cast<uint8_t>(dist(gen));
        bytes[6] = (bytes[6] & 0x0F) | 0x40;
        bytes[8] = (bytes[8] & 0x3F) | 0x80;
        std::ostringstream oss;
        for (auto b : bytes)
            oss << std::hex << std::setw(2) << std::setfill('0') << (int)b;
        return oss.str();
    }

    static std::vector<uint8_t> GuidHexToBytes(const std::string& hex) {
        std::vector<uint8_t> bytes;
        for (size_t i = 0; i + 1 < hex.size(); i += 2)
            bytes.push_back(static_cast<uint8_t>(std::stoul(hex.substr(i, 2), nullptr, 16)));
        return bytes;
    }

    void PrependLengthPrefix(std::vector<uint8_t>& buffer) {
        uint16_t len = static_cast<uint16_t>(buffer.size());
        buffer.insert(buffer.begin(), {static_cast<uint8_t>(len & 0xFF), static_cast<uint8_t>(len >> 8)});
    }

    void send_response(std::vector<uint8_t>& buffer) {
        // Wire framing:
        //   payload <= 1456 bytes:  [2B length][payload]  (single frame)
        //   payload >  1456 bytes:  N-1 chunks of [2B 0x0000][1456 bytes]
        //                           followed by 1 final chunk of
        //                           [2B last_chunk_size][last_chunk_bytes]
        //
        // The 0x0000 prefix tells the client "this is a continuation chunk,
        // accumulate". A non-zero length prefix tells the client "this is
        // the final chunk of size N, marshal is complete after these bytes".
        constexpr size_t kChunkPayload = 1456;

        std::vector<uint8_t> response;

        if (buffer.size() <= kChunkPayload) {
            response.reserve(buffer.size() + 2);
            const uint16_t length_prefix = static_cast<uint16_t>(buffer.size());
            response.push_back(static_cast<uint8_t>(length_prefix & 0xFF));
            response.push_back(static_cast<uint8_t>(length_prefix >> 8));
            response.insert(response.end(), buffer.begin(), buffer.end());
        } else {
            const size_t total = buffer.size();
            // Continuation chunks: each [2B 0x0000][kChunkPayload bytes].
            // The final chunk is whatever remains (1..kChunkPayload bytes),
            // prefixed with its byte count instead of 0x0000.
            const size_t n_full_chunks   = total / kChunkPayload;
            const size_t final_chunk_len = total - n_full_chunks * kChunkPayload;

            // If `total` is an exact multiple of kChunkPayload, the last full
            // chunk becomes the "final" one carrying its length instead of
            // 0x0000. Otherwise the leftover bytes are the final chunk.
            size_t continuation_chunks = n_full_chunks;
            size_t last_chunk_size     = final_chunk_len;
            if (final_chunk_len == 0) {
                continuation_chunks -= 1;
                last_chunk_size = kChunkPayload;
            }

            response.reserve(total + 2 * (continuation_chunks + 1));
            size_t pos = 0;
            for (size_t i = 0; i < continuation_chunks; ++i) {
                response.push_back(0x00);
                response.push_back(0x00);
                response.insert(response.end(),
                                buffer.begin() + pos,
                                buffer.begin() + pos + kChunkPayload);
                pos += kChunkPayload;
            }
            // Final chunk with length prefix.
            const uint16_t last_len = static_cast<uint16_t>(last_chunk_size);
            response.push_back(static_cast<uint8_t>(last_len & 0xFF));
            response.push_back(static_cast<uint8_t>(last_len >> 8));
            response.insert(response.end(),
                            buffer.begin() + pos,
                            buffer.end());

            const uint16_t op = (buffer.size() >= 2)
                ? static_cast<uint16_t>(buffer[0] | (buffer[1] << 8))
                : 0;
            Logger::Log("tcp",
                "[TCP] send_response chunked: opcode=0x%04X payload=%zu wire=%zu (%zu cont chunks + final=%zu)\n",
                op, buffer.size(), response.size(),
                continuation_chunks, last_chunk_size);
        }

        enqueue_write(std::move(response));
    }

    // Queue a wire-ready frame and start the async write chain if idle.
    // Single io_context thread — no locking needed.
    void enqueue_write(std::vector<uint8_t>&& frame) {
        if (!socket_.is_open() || close_when_drained_) return;
        if (write_queue_bytes_ + frame.size() > kMaxWriteQueueBytes) {
            Logger::Log("tcp",
                "[TCP] write queue overflow (%zu + %zu bytes) player='%s' — closing stalled socket\n",
                write_queue_bytes_, frame.size(), player_name.c_str());
            // Close only — no queue clear here: the in-flight async_write still
            // references the front buffer; its aborted completion clears the
            // queue, and do_read's error branch runs handle_socket_disconnect.
            std::error_code ignored;
            socket_.shutdown(asio::ip::tcp::socket::shutdown_both, ignored);
            socket_.close(ignored);
            return;
        }
        write_queue_bytes_ += frame.size();
        const bool idle = write_queue_.empty();
        write_queue_.push_back(std::move(frame));
        if (idle) do_write();
    }

    void do_write() {
        auto self(shared_from_this());
        asio::async_write(socket_, asio::buffer(write_queue_.front()),
            [this, self](std::error_code ec, std::size_t /*bytes*/) {
                if (ec) {
                    Logger::Log("tcp", "[TCP] Write failed: %s (player='%s')\n",
                        ec.message().c_str(), player_name.c_str());
                    write_queue_.clear();
                    write_queue_bytes_ = 0;
                    std::error_code ignored;
                    socket_.close(ignored);  // do_read's error branch cleans up
                    return;
                }
                write_queue_bytes_ -= write_queue_.front().size();
                write_queue_.pop_front();
                if (!write_queue_.empty()) {
                    do_write();
                } else if (close_when_drained_) {
                    std::error_code ignored;
                    socket_.shutdown(asio::ip::tcp::socket::shutdown_both, ignored);
                    socket_.close(ignored);
                }
            });
    }

    void close_after_login_rejection() {
        // Writes are queued now — closing immediately would cancel an
        // in-flight rejection frame. Drain first if anything is pending.
        if (!write_queue_.empty()) {
            close_when_drained_ = true;
            return;
        }
        std::error_code ignored;
        socket_.shutdown(asio::ip::tcp::socket::shutdown_both, ignored);
        socket_.close(ignored);
    }

    void do_read() {
        auto self(shared_from_this());
        socket_.async_read_some(
            asio::buffer(data_),
            [this, self](std::error_code ec, std::size_t length) {
                if (!ec) {
                    rx_buffer_.insert(rx_buffer_.end(), data_.begin(), data_.begin() + length);
                    while (rx_buffer_.size() >= 2) {
                        const uint16_t frame_len = static_cast<uint16_t>(
                            rx_buffer_[0] | (rx_buffer_[1] << 8));
                        if (frame_len == 0) {
                            rx_buffer_.erase(rx_buffer_.begin(), rx_buffer_.begin() + 2);
                            continue;
                        }
                        const size_t total_len = static_cast<size_t>(frame_len) + 2;
                        if (rx_buffer_.size() < total_len) {
                            break;
                        }
                        handle_packet(rx_buffer_.data(), total_len);
                        rx_buffer_.erase(rx_buffer_.begin(), rx_buffer_.begin() + total_len);
                    }
                    do_read();
                } else {
                    handle_socket_disconnect();
                    Logger::Log("tcp", "[TCP] Client disconnected: %s\n", player_name.c_str());
                }
            });
    }

    // Full session teardown after the control socket is gone (read error) or
    // the session is evicted by a re-login takeover. Reaps the instance-side
    // NetConnection via MSG_PLAYER_CLOSE, finalizes the DB session row and
    // unregisters everywhere. Idempotent — clears session_guid_ so a later
    // do_read error on the same object is a no-op.
    void handle_socket_disconnect();

    void handle_packet(const uint8_t* data, size_t length);

    // Sends PLAYER_REGISTER over IPC and arms 60-second ACK-wait timer.
    // On ACK success: calls send_go_play_response(). On timeout/failure: silent.
    void initiate_player_register_and_go_play();

    // Polls for a READY home map instance every 2 seconds up to remaining_seconds,
    // then calls initiate_player_register_and_go_play().
    void wait_for_home_map_then_register(int remaining_seconds);

    void route_from_mission_instance(int64_t parent_instance_id, const char* reason);

    // Continue-in-queue routing. Sends PLAYER_REGISTER to the given target
    // mission instance with task_force assignment; on ACK success replies
    // with GSC_GO_PLAY pointing at that instance. On ACK failure (e.g. the
    // successor was already full), falls back to the standard home-routing
    // path so the player still ends up somewhere sensible.
    void initiate_player_register_for_target(const struct InstanceInfo& target, int task_force);

    void send_get_loot_table_items_by_id_filtered_response();

    void send_inventory_response(int nPawnId, int64_t character_id);

    // Bulk STATE=2 delete pass for every inventory row the user owns. The
    // device-bar widget binds to per-profile slot data inside the client's
    // m_InventoryMap entries (DATA_SET_CHARACTER_PROFILES rows). On profile
    // switch, sending updated rows isn't enough — the client's record-merge
    // logic appears to accumulate rather than replace the per-profile entries,
    // leaving stale "equipped in profile N" data that the bar still renders.
    // Wiping each entry before send_inventory_response re-adds it forces a
    // fresh rebuild — same shape as the disconnect/reconnect flow that the
    // user already confirmed fixes the symptom.
    void send_inventory_clear(int nPawnId, int64_t character_id);

    // Identical wire format to send_inventory_response but pulls the records
    // from an in-memory list (no DB lookup). Used by the -possess chat command
    // to display the bot's loadout in the player's inventory UI.
    struct LoadoutItem {
        int device_id     = 0;
        int inventory_id  = 0;
        int quality       = 1162;
        int slot_value_id = 0;
    };
    void send_loadout_inventory_response(int nPawnId, const std::vector<LoadoutItem>& items);

    void send_beacon_pickup_response(int nPawnId, int nDeviceId, int nInventoryId, int nEquipSlotValueId, int nItemProfileId);
    void send_beacon_remove_response(int nPawnId, int nInventoryId);
    void send_quest_accept_response(int nQuestId);
    void send_quest_complete_response(int nQuestId);
    void send_quest_abandon_response(int nQuestId);

    void send_character_inventory_response(int nPawnId);

    void send_player_skills_response();

    void send_agency_get_roster_response();
    void handle_agency_create(const PacketView& pkt);
    // Leader disbands the agency (delete it + push the "no agency" reset).
    void handle_agency_disband();
    // Append the caller's agency (meta + DATA_SET_RANKS + DATA_SET_MEMBERS) to
    // `response` as top-level fields. Returns the number of fields appended (0
    // if the caller is not in an agency). Shared by roster + create responses.
    int append_agency_payload(std::vector<uint8_t>& response);

    void handle_agency_invite(const PacketView& pkt);
    // AGENCY_PROMOTE / AGENCY_DEMOTE — target member by PLAYER_ID (= character_id).
    void handle_agency_rank_change(const PacketView& pkt, bool promote);
    // Kick a member (AGENCY_PLAYER_REMOVE, or AGENCY_REMOVE with someone else's id).
    void handle_agency_kick(int64_t target_char);
    // AGENCY_SET_NOTE — MOTD / description / per-member public+officer notes.
    void handle_agency_set_note(const PacketView& pkt);
    // AGENCY_UPDATE_RANKS — full rank-table replacement from the rank editor.
    void handle_agency_update_ranks(const PacketView& pkt);
    // AGENCY_UPDATE_RECRUITING — Recruiting tab listing text + the two flags.
    void handle_agency_update_recruiting(const PacketView& pkt);
    // AGENCY_SET_OWNER — Management tab "Transfer Leader".
    void handle_agency_set_owner(const PacketView& pkt);
    // Push "no agency" + "no alliance" to a live session that just lost its
    // agency without asking (kick / someone else's disband).
    static void DeliverAgencyRefresh(int64_t target_user_id);
    void handle_agency_accept();
    // Push an AGENCY_INVITATION popup to this session.
    // Status/error line for agency actions (MSG_ID template + @@player_name@@).
    void send_agency_message(uint32_t msg_id, const std::string& player_name_token);
    void send_agency_invitation(const std::string& agency_name,
                                const std::string& inviter_leader_name);
    // Pending agency invites: invited user_id -> agency_id it was invited to.
    static std::mutex pending_agency_mutex_;
    static std::map<int64_t, int64_t> pending_agency_invites_;

    void send_alliance_member_list_response();
    void handle_alliance_create(const PacketView& pkt);
    // Append the caller's alliance (meta + DATA_SET_AGENCIES) as top-level
    // fields. Returns field count (0 if the caller's agency has no alliance).
    int append_alliance_payload(std::vector<uint8_t>& response);
    void handle_alliance_disband();
    void handle_alliance_invite(const PacketView& pkt);
    void handle_alliance_accept();
    void handle_alliance_remove(const PacketView& pkt);
    // Push an ALLIANCE_INVITATION popup to this session.
    void send_alliance_invitation(const std::string& alliance_name,
                                  const std::string& inviter_leader_name,
                                  int64_t alliance_id);
    // Deliver an alliance invitation to whichever live session owns target_user_id.
    static bool DeliverAllianceInvitation(int64_t target_user_id,
                                          const std::string& alliance_name,
                                          const std::string& inviter_leader_name,
                                          int64_t alliance_id);
    // Pending alliance invites: target agency_id -> alliance_id it was invited to.
    static std::mutex pending_alliance_mutex_;
    static std::map<int64_t, int64_t> pending_alliance_invites_;

    // QUERY_PLAYERS (0x0152) — player search from the Team menu / player
    // search scene. Echoes CALLER_ID (the client TgDataSet's GObjObjects
    // index) + a DATA_SET of matching online players. See
    // player-search-tcp-protocol.md.
    void send_query_players_response(const PacketView& pkt);
// F3 friends menu (FRIEND_GET_LIST / FRIEND_UPDATE arrive on this socket).
void handle_friend_update(const PacketView& pkt);
void send_friends_list();

    // PLAYER_COMMAND (0x019F) — slash commands the client doesn't recognise
    // itself are forwarded here verbatim in TEXT_VALUE (0x04FF). Channel
    // commands (/t, /l, /c, ...) route into the player's chat session;
    // anything else gets an "unknown command" reply.
    void handle_player_command(const PacketView& pkt);

    // Team wire helpers. Opcodes 0x4A/0x4D/0x4E/0x4F all route to the client's
    // message-display handler (vt[0x54]: MSG_ID 0x36E template + @@token@@
    // fields); 0x4B queues the invitation popup; 0x4C dismisses it (no fields).
    void send_team_system_message(uint32_t msg_id, const std::string& player_name);
    void send_team_invitation_popup(const std::string& leader_name);
    void send_team_invite_expired();
    void send_team_update(const TeamRoster& roster);


    void send_start_listen_response();

    void send_player_update_client_response();

    void send_update_new_mail_count_response();

    void send_add_player_character_response();

    void send_select_character_response();

    void send_change_map_prep_response();

    void send_change_map_response();

    void send_match_launch_response();

	void send_match_join_response(uint32_t matchQueueId, uint32_t matchFilters);

	void send_match_leave_response();

	// S→C MATCH_JOIN (0xE1): drives the AgentInfo HUD queue panel
	// ("IN QUEUE: <name>" + leave-queue keybind / DESERTER countdown).
	// queue_id=0 hides the panel. See queue-hud-protocol notes in
	// team-tcp-protocol.md.
	void send_match_queue_status(uint32_t queue_id, uint32_t name_msg_id,
	                             uint32_t penalty_seconds = 0);

    void send_match_invitation(int64_t instance_id, const std::string& game_mode, int task_force);

    void send_match_accept_response();

    void send_go_play_tutorial_response();

    void send_marshal_channel_response();

    void send_go_play_response();

    // Variant that targets an arbitrary mission instance (used by the
    // continue-in-queue successor flow). task_force selects the team
    // assignment baked into the GSC_GO_PLAY payload.
    void send_go_play_to_instance(const struct InstanceInfo& target, int task_force);

    void send_instance_ready_response();

    void send_get_ticket_info_response();

    void send_character_list_response();
    void send_character_list_response_mock();

    void send_character_list_queue_response();

    void send_login_response_for(const std::string& response_player_name,
                                 const std::string& response_session_guid,
                                 bool success = true,
                                 const char* error_text = nullptr,
                                 uint32_t error_msg_id = 0);

    // msg_id is the client localization id the login menu shows on failure.
    // CGameClient::FinishLogin's error path reads MSG_ID (0x036E) and displays
    // it; WITHOUT it the client shows nothing and the login form hangs on
    // "logging in" until its own client-side timeout.
    //
    // Default 0 keeps that hang ON PURPOSE — banned players (and the other
    // reject callers) should NOT get a clear error; we want them to think the
    // server is just broken rather than learn they're banned. Only the
    // incorrect-password path passes a real id (18760 = "incorrect password").
    void send_login_rejected_response(const char* error_text = nullptr,
                                      uint32_t msg_id = 0);

    void send_login_response();

    static void LogData(const uint8_t* data, size_t length) {
        std::ostringstream oss;
        for (size_t i = 0; i < length; i++) {
            oss << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]);
        }
        Logger::Log("tcp", "%s\n", oss.str().c_str());
    }

    static std::string get_current_datetime() {
        auto now = std::chrono::system_clock::now();
        std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);

        std::tm tm;
#if defined(_WIN32) || defined(_WIN64)
        localtime_s(&tm, &now_time_t);  // Windows
#else
        localtime_r(&now_time_t, &tm);  // POSIX
#endif

        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }
};
