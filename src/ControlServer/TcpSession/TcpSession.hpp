#pragma once

#include <asio.hpp>
#include <vector>
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
#include "src/ControlServer/InstanceRegistry/InstanceRegistry.hpp"
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
        do_read();
    }

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

    // Clear the per-session queue + pending-match state on the named session.
    // Called when a matchmade instance dies before delivering MATCH_INVITATION
    // (spawn failure or pre-READY crash) — without this the session stays
    // wedged with current_match_queue_id_ != 0 and the next GET_TICKET_INFO
    // shows the player as still queued. Silent if guid is unknown.
    static void DeliverMatchCancelled(const std::string& session_guid, const char* reason);

private:
    // ── Static registry members ──────────────────────────────────────────────
    static std::mutex sessions_mutex_;
    static std::map<std::string, std::weak_ptr<TcpSession>> g_sessions_;
    asio::ip::tcp::socket socket_;
    asio::io_context& io_ctx_;
    std::vector<uint8_t> data_;
    std::vector<uint8_t> rx_buffer_;

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

	uint32_t current_match_queue_id_ = 0;

    // Set when a MATCH_INVITATION is delivered — the instance to join on MATCH_ACCEPT.
    int64_t pending_match_instance_id_ = 0;
    std::string pending_match_game_mode_;
    int pending_match_task_force_ = 1;

    // ACK-wait timer for PLAYER_REGISTER flow. Cancelled on ACK arrival or disconnect.
    std::shared_ptr<asio::steady_timer> pending_ack_timer_;

    // Static callback to spawn a home map instance on demand.
    static std::function<void()> on_need_home_map_;

    // Network config: external IP and chat port, set once from main.
    static std::string s_host_;
    static uint16_t    s_chat_port_;
    static bool        s_allow_duplicate_account_logins_;
public:
    static void SetHomeMapSpawner(std::function<void()> cb);
    static void SetNetworkConfig(const std::string& host, uint16_t chat_port);
    static void SetLoginPolicy(bool allow_duplicate_account_logins);

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

        std::error_code ec;
        asio::write(socket_, asio::buffer(response), ec);

        if (ec) {
            Logger::Log("tcp", "[TCP] Write failed: %s\n", ec.message().c_str());
        }
    }

    void close_after_login_rejection() {
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
                    // Cancel any pending ACK wait timer.
                    if (pending_ack_timer_) {
                        pending_ack_timer_->cancel();
                        pending_ack_timer_.reset();
                    }
                    if (!session_guid_.empty()) {
                        MatchmakingService::RemovePlayer(session_guid_);
                        UnregisterSession(session_guid_);
                        IpcServer::ClearPendingAck(session_guid_);
                        PlayerSessionStore::Unregister(session_guid_);
                    }
                    Logger::Log("tcp", "[TCP] Client disconnected: %s\n", player_name.c_str());
                }
            });
    }

    void handle_packet(const uint8_t* data, size_t length);

    // Sends PLAYER_REGISTER over IPC and arms 60-second ACK-wait timer.
    // On ACK success: calls send_go_play_response(). On timeout/failure: silent.
    void initiate_player_register_and_go_play();

    // Polls for a READY home map instance every 2 seconds up to remaining_seconds,
    // then calls initiate_player_register_and_go_play().
    void wait_for_home_map_then_register(int remaining_seconds);

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
                                 const char* error_text = nullptr);

    void send_login_rejected_response(const char* error_text = nullptr);

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
