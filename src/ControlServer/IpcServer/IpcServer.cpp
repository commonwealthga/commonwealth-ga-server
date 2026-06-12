#include "src/ControlServer/IpcServer/IpcServer.hpp"
#include "src/ControlServer/Logger.hpp"
#include "src/ControlServer/InstanceRegistry/InstanceRegistry.hpp"
#include "src/ControlServer/InstanceSpawner/InstanceSpawner.hpp"
#include "src/ControlServer/TcpSession/TcpSession.hpp"
#include "src/Shared/IpcProtocol.hpp"
#include "src/Shared/IpcFraming.hpp"
#include "lib/nlohmann/json.hpp"
#include <chrono>
#include <memory>
#include <array>
#include <vector>
#include <deque>
#include <string>
#include "src/ControlServer/MatchmakingService/MatchmakingService.hpp"
#include "src/ControlServer/MatchmakingService/RoleWeightedSplit.hpp"
#include "src/ControlServer/ChatSession/ChatCommand.hpp"
#include "src/ControlServer/Database/Database.hpp"

namespace {

bool IsPrivateOrLocalAddress(const asio::ip::address& address) {
    if (address.is_loopback()) return true;
    if (address.is_v4()) {
        const auto bytes = address.to_v4().to_bytes();
        const uint8_t a = bytes[0];
        const uint8_t b = bytes[1];
        if (a == 10) return true;
        if (a == 172 && b >= 16 && b <= 31) return true;
        if (a == 192 && b == 168) return true;
        if (a == 169 && b == 254) return true;
        return false;
    }
    if (address.is_v6()) {
        const auto bytes = address.to_v6().to_bytes();
        if ((bytes[0] & 0xfe) == 0xfc) return true;  // fc00::/7
        if (bytes[0] == 0xfe && (bytes[1] & 0xc0) == 0x80) return true;  // fe80::/10
    }
    return false;
}

bool StopNonHomeInstanceIfEmpty(int64_t instance_id, const char* reason) {
    if (instance_id == 0) {
        return false;
    }

    auto inst = InstanceRegistry::GetInstanceById(instance_id);
    if (!inst || inst->is_home_map || inst->state == "STOPPED") {
        return false;
    }

    const int active_players = InstanceRegistry::GetActivePlayerCount(instance_id);
    if (active_players != 0) {
        return false;
    }

    Logger::Log("ipc",
        "[IpcServer] Stopping empty non-home instance=%lld reason=%s\n",
        (long long)instance_id, reason ? reason : "unspecified");
    InstanceSpawner::StopInstanceProcess(*inst, reason ? reason : "empty non-home instance");
    InstanceRegistry::MarkStopped(instance_id);
    return true;
}

} // namespace

// ---------------------------------------------------------------------------
// IpcSession -- handles a single game instance connection (server side)
// ---------------------------------------------------------------------------

class IpcSession : public std::enable_shared_from_this<IpcSession> {
public:
    explicit IpcSession(asio::ip::tcp::socket socket)
        : socket_(std::move(socket)), write_in_progress_(false),
          instance_id_(0), validated_(false) {}

    void start() {
        do_read_header();
    }

private:
    // ── Read pipeline ───────────────────────────────────────────────────────

    void do_read_header() {
        auto self = shared_from_this();
        asio::async_read(
            socket_,
            asio::buffer(header_buf_, 4),
            asio::transfer_exactly(4),
            [this, self](const asio::error_code& ec, std::size_t /*bytes*/) {
                if (ec) {
                    // EOF on header read is the normal close for short-lived
                    // admin clients (e.g. dashboard /api/players/online poll)
                    // that send one ADMIN_ACTION and disconnect. Validated
                    // game-instance disconnects are still announced by
                    // on_disconnect(). Only log non-EOF errors here.
                    if (ec != asio::error::eof) {
                        Logger::Log("ipc", "[IpcServer] Read header error: %s\n",
                            ec.message().c_str());
                    }
                    on_disconnect();
                    return;
                }

                uint32_t len =
                      (static_cast<uint32_t>(header_buf_[0])      )
                    | (static_cast<uint32_t>(header_buf_[1]) <<  8)
                    | (static_cast<uint32_t>(header_buf_[2]) << 16)
                    | (static_cast<uint32_t>(header_buf_[3]) << 24);

                do_read_body(len);
            }
        );
    }

    void do_read_body(uint32_t len) {
        body_buf_.resize(len);
        auto self = shared_from_this();
        asio::async_read(
            socket_,
            asio::buffer(body_buf_),
            asio::transfer_exactly(len),
            [this, self](const asio::error_code& ec, std::size_t /*bytes*/) {
                if (ec) {
                    // EOF mid-body is rare but harmless if a client tore the
                    // socket down between header and body; symmetric with the
                    // header-EOF suppression above.
                    if (ec != asio::error::eof) {
                        Logger::Log("ipc", "[IpcServer] Read body error: %s\n",
                            ec.message().c_str());
                    }
                    on_disconnect();
                    return;
                }

                dispatch(std::string(body_buf_.begin(), body_buf_.end()));
                do_read_header();
            }
        );
    }

    // ── Disconnect cleanup ──────────────────────────────────────────────────

    void on_disconnect() {
        if (instance_id_ != 0) {
            {
                std::lock_guard<std::mutex> lock(sessions_mutex_);
                g_sessions.erase(instance_id_);
            }
            // Orphan safety net: if this instance had a DRAFTING successor
            // that was waiting for MSG_MISSION_ENDED, the parent just died
            // without firing it. Promote the successor so the matchmaker
            // can use it as a normal queue match (instead of leaving it
            // stuck in DRAFTING forever).
            int64_t promoted = InstanceRegistry::PromoteSuccessor(instance_id_);
            if (promoted != 0) {
                Logger::Log("ipc", "[IpcServer] Instance %lld died — orphan-promoted successor %lld DRAFTING→READY\n",
                    (long long)instance_id_, (long long)promoted);
            }
            // If a pending match still exists for this instance, the instance
            // died before delivering INSTANCE_READY (crash during startup).
            // Cancel the match and unstick the players — without this they
            // get coalesced into the dead instance on every subsequent queue
            // join and never receive a MATCH_INVITATION.
            MatchmakingService::DiscardPendingMatchForDeadInstance(
                instance_id_, "instance disconnected before INSTANCE_READY");
            InstanceRegistry::MarkStopped(instance_id_);
            Logger::Log("ipc", "[IpcServer] Instance %lld disconnected, marked STOPPED\n",
                (long long)instance_id_);
        }
    }

    // ── Dispatch ────────────────────────────────────────────────────────────

    void dispatch(const std::string& json_msg) {
        auto j = nlohmann::json::parse(json_msg, nullptr, /*allow_exceptions=*/false, /*ignore_comments=*/true);
        if (j.is_discarded()) {
            Logger::Log("ipc", "[IpcServer] Malformed JSON from game instance\n");
            return;
        }

        std::string type = j.value("type", "");

        if (type == IpcProtocol::MSG_INSTANCE_HELLO) {
            int64_t inst_id = j.value("instance_id", (int64_t)0);
            Logger::Log("ipc", "[IpcServer] INSTANCE_HELLO: instance_id=%lld\n",
                (long long)inst_id);

            // Reject if instance_id is 0 (unassigned).
            if (inst_id == 0) {
                Logger::Log("ipc", "[IpcServer] INSTANCE_HELLO rejected: instance_id=0\n");
                socket_.close();
                return;
            }

            instance_id_ = inst_id;
            validated_ = true;

            // Register in g_sessions.
            {
                std::lock_guard<std::mutex> lock(sessions_mutex_);
                g_sessions[inst_id] = shared_from_this();
            }

            // Send ACK. stats_enabled=false on home maps — the DLL skips
            // all match-stats tracking/emission there (design 2026-06-12).
            auto info = InstanceRegistry::GetInstanceById(inst_id);
            is_home_map_ = info && info->is_home_map;
            nlohmann::json ack;
            ack["type"]          = IpcProtocol::MSG_INSTANCE_HELLO_ACK;
            ack["accepted"]      = true;
            ack["stats_enabled"] = !is_home_map_;
            send(ack.dump());
            Logger::Log("ipc", "[IpcServer] INSTANCE_HELLO_ACK sent to instance_id=%lld\n",
                (long long)inst_id);
        }
        else if (type == IpcProtocol::MSG_ADMIN_ACTION) {
            handle_admin_action(j);
        }
        else if (type == IpcProtocol::MSG_INSTANCE_READY) {
            int64_t inst_id    = j.value("instance_id", (int64_t)0);
            int max_players    = j.value("max_players", 0);
            Logger::Log("ipc", "[IpcServer] INSTANCE_READY: instance_id=%lld max_players=%d\n",
                (long long)inst_id, max_players);
            InstanceRegistry::MarkReady(inst_id, max_players);

            // Check if this instance was spawned for a pending match
            auto pending = MatchmakingService::ConsumePendingMatch(inst_id);
            if (pending) {
                Logger::Log("ipc", "[IpcServer] Instance %lld ready — sending MATCH_INVITATION to %zu players\n",
                    (long long)inst_id, pending->session_guids.size());
                MatchmakingService::TrackReadyMatchReservations(
                    inst_id, pending->queue_id, pending->game_mode,
                    pending->session_guids, pending->task_force_assignments,
                    pending->profile_ids, pending->cap);
                for (const auto& guid : pending->session_guids) {
                    int tf = 1;
                    auto tfit = pending->task_force_assignments.find(guid);
                    if (tfit != pending->task_force_assignments.end()) tf = tfit->second;
                    TcpSession::DeliverMatchInvitation(guid, inst_id, pending->game_mode, tf);
                }
            }
        }
        else if (type == IpcProtocol::MSG_PING) {
            Logger::Log("ipc", "[IpcServer] PING received, sending PONG\n");
            nlohmann::json pong;
            pong["type"] = IpcProtocol::MSG_PONG;
            send(pong.dump());
        }
        else if (type == IpcProtocol::MSG_PLAYER_REGISTER_ACK) {
            std::string guid = j.value("session_guid", "");
            bool success     = j.value("success", false);
            int pawn_id      = j.value("pawn_id", 0);
            Logger::Log("ipc", "[IpcServer] PLAYER_REGISTER_ACK: guid=%s success=%d pawn_id=%d\n",
                guid.c_str(), (int)success, pawn_id);
            IpcServer::DeliverAck(guid, success, pawn_id);
        }
        else if (type == IpcProtocol::MSG_GAME_EVENT) {
            std::string subtype = j.value("subtype", "");
            std::string guid    = j.value("session_guid", "");
            if (subtype == "chat_command_audit") {
                const std::string player  = j.value("player_name", "");
                const std::string command = j.value("command", "");
                const std::string outcome = j.value("outcome", "");
                const std::string details = j.value("details", "");
                const int64_t inst_id     = j.value("instance_id", instance_id_);
                Logger::Log("chat-command",
                    "[ChatCmd] %s player='%s' guid=%s instance=%lld outcome=%s details=%s\n",
                    command.c_str(), player.c_str(), guid.c_str(),
                    (long long)inst_id, outcome.c_str(), details.c_str());
                return;
            }
            Logger::Log("ipc", "[IpcServer] GAME_EVENT subtype=%s guid=%s\n",
                subtype.c_str(), guid.c_str());
            if (guid.empty()) {
                Logger::Log("ipc", "[IpcServer] GAME_EVENT: empty guid -- dropped\n");
                return;
            }
            TcpSession::DeliverGameEvent(guid, j);
        }
        else if (type == IpcProtocol::MSG_PLAYER_JOINED) {
            int64_t inst_id    = j.value("instance_id", (int64_t)0);
            std::string guid   = j.value("session_guid", "");
            int task_force     = j.value("task_force", 1);
            Logger::Log("ipc", "[IpcServer] PLAYER_JOINED: instance=%lld guid=%s tf=%d\n",
                (long long)inst_id, guid.c_str(), task_force);
            if (inst_id != 0 && !guid.empty()) {
                InstanceRegistry::UpdateInstancePlayerTaskForce(inst_id, guid, task_force);
            }
        }
        else if (type == IpcProtocol::MSG_PLAYER_LEFT) {
            int64_t inst_id    = j.value("instance_id", (int64_t)0);
            std::string guid   = j.value("session_guid", "");
            Logger::Log("ipc", "[IpcServer] PLAYER_LEFT: instance=%lld guid=%s\n",
                (long long)inst_id, guid.c_str());
            InstanceRegistry::MarkInstancePlayerLeft(inst_id, guid);
            StopNonHomeInstanceIfEmpty(inst_id, "last player left");
        }
        else if (type == IpcProtocol::MSG_INSTANCE_EMPTY) {
            int64_t inst_id = j.value("instance_id", (int64_t)0);
            Logger::Log("ipc", "[IpcServer] INSTANCE_EMPTY: instance=%lld\n", (long long)inst_id);
            // Home stays warm for mission returns; non-home instances have no
            // useful idle role once every player is gone.
            if (!StopNonHomeInstanceIfEmpty(inst_id, "instance empty")) {
                InstanceRegistry::SetLastEmptyAtIfEmpty(inst_id);
            }
        }
        else if (type == IpcProtocol::MSG_NEED_HOME_MAP) {
            // Mission instance ended a round and is warning us players are about
            // to click "End Mission" → they'll send GSC_CHANGE_INSTANCE{0,0}
            // expecting an immediate route back to home. Pre-spawn now so the
            // home instance is READY by the time that TCP packet lands.
            Logger::Log("ipc", "[IpcServer] NEED_HOME_MAP from instance=%lld\n",
                (long long)instance_id_);
            TcpSession::EnsureHomeMapWarm("MSG_NEED_HOME_MAP");
        }
        else if (type == IpcProtocol::MSG_MATCH_EVENT) {
            if (is_home_map_) return;  // home instances are not matches
            int64_t inst_id = j.value("instance_id", instance_id_);
            Database::MatchEventRow row;
            row.instance_id         = inst_id;
            row.game_time           = j.value("game_time", 0.0);
            row.event_type          = j.value("event_type", "");
            row.actor_user_id       = j.value("actor_user_id", (int64_t)0);
            row.actor_character_id  = j.value("actor_character_id", (int64_t)0);
            row.actor_bot_id        = j.value("actor_bot_id", 0);
            row.actor_task_force    = j.value("actor_task_force", 0);
            row.target_user_id      = j.value("target_user_id", (int64_t)0);
            row.target_character_id = j.value("target_character_id", (int64_t)0);
            row.target_bot_id       = j.value("target_bot_id", 0);
            row.target_task_force   = j.value("target_task_force", 0);
            row.owner_user_id       = j.value("owner_user_id", (int64_t)0);
            row.owner_character_id  = j.value("owner_character_id", (int64_t)0);
            row.device_id           = j.value("device_id", 0);
            row.detail              = j.value("detail", (int64_t)0);
            row.flags               = j.value("flags", 0);
            if (row.event_type.empty()) return;
            const int64_t event_id = Database::InsertMatchEvent(row);
            // KILL piggybacks its assists; ASSIST.detail = kill rowid.
            if (event_id != 0 && j.contains("assists") && j["assists"].is_array()) {
                for (const auto& a : j["assists"]) {
                    Database::MatchEventRow ar;
                    ar.instance_id         = inst_id;
                    ar.game_time           = row.game_time;
                    ar.event_type          = "ASSIST";
                    ar.actor_user_id       = a.value("user_id", (int64_t)0);
                    ar.actor_character_id  = a.value("character_id", (int64_t)0);
                    ar.actor_task_force    = a.value("task_force", 0);
                    ar.target_user_id      = row.target_user_id;
                    ar.target_character_id = row.target_character_id;
                    ar.target_bot_id       = row.target_bot_id;
                    ar.target_task_force   = row.target_task_force;
                    ar.detail              = event_id;
                    Database::InsertMatchEvent(ar);
                }
            }
        }
        else if (type == IpcProtocol::MSG_MATCH_STATS) {
            if (is_home_map_) return;
            Database::MatchPlayerStatsRow row;
            row.instance_id  = j.value("instance_id", instance_id_);
            row.user_id      = j.value("user_id", (int64_t)0);
            row.character_id = j.value("character_id", (int64_t)0);
            row.task_force   = j.value("task_force", 0);
            if (row.character_id == 0) return;
            if (j.contains("scores") && j["scores"].is_array()) {
                int i = 0;
                for (const auto& s : j["scores"]) {
                    if (i >= 11) break;
                    row.scores[i++] = s.is_number() ? s.get<int>() : 0;
                }
            }
            row.capture_seconds        = j.value("capture_seconds", 0.0);
            row.contest_seconds        = j.value("contest_seconds", 0.0);
            row.objective_captures     = j.value("objective_captures", 0);
            row.beacon_spawns_provided = j.value("beacon_spawns_provided", 0);
            row.beacon_spawns_used     = j.value("beacon_spawns_used", 0);
            row.beacons_destroyed      = j.value("beacons_destroyed", 0);
            row.time_played_seconds    = j.value("time_played_seconds", 0.0);
            Database::UpsertMatchPlayerStats(row);
        }
        else if (type == IpcProtocol::MSG_MISSION_ENDED) {
            // BeginEndMission fired on the mission instance. Stamp end_mission_at
            // on the parent row AND promote any DRAFTING successor to READY in
            // one DB transaction. After this, GSC_CHANGE_INSTANCE{0,0} from any
            // player still in this mission consults the parent's end_mission_at
            // + queue's continue_in_queue config to decide between home vs the
            // successor instance.
            int64_t inst_id = j.value("instance_id", (int64_t)0);
            if (inst_id == 0) inst_id = instance_id_;  // fallback to validated id
            int64_t successor = InstanceRegistry::MarkMissionEnded(inst_id);
            Logger::Log("ipc", "[IpcServer] MISSION_ENDED: parent=%lld successor=%lld\n",
                (long long)inst_id, (long long)successor);

            // Win/stalemate outcome from the game's own win-state
            // computation (design 2026-06-12). Write-once.
            const std::string outcome = j.value("outcome", "");
            if (!outcome.empty()) {
                Database::SetInstanceOutcomeIfNull(
                    inst_id, outcome, j.value("winning_task_force", 0));
            }

            // Populate pre-assigned teams for the successor so each
            // survivor's GSC_CHANGE_INSTANCE arrival lands them on a
            // policy-chosen team rather than the legacy hardcoded TF1.
            // Only meaningful for balanced policies (pinned queues route
            // 100% to one side anyway).
            if (successor != 0) {
                auto parent_info = InstanceRegistry::GetInstanceById(inst_id);
                if (parent_info && parent_info->queue_id != 0) {
                    auto queue_cfg = MatchmakingService::GetQueueConfig(parent_info->queue_id);
                    const bool is_balanced =
                        queue_cfg && (queue_cfg->taskforce_policy == TaskforcePolicy::Balanced
                                   || queue_cfg->taskforce_policy == TaskforcePolicy::BalancedPvp);
                    if (is_balanced) {
                        auto roster = InstanceRegistry::GetActivePlayersForInstance(inst_id);
                        std::vector<RoleWeightedSplit::PlayerSlot> slots;
                        slots.reserve(roster.size());
                        for (const auto& r : roster) {
                            slots.push_back({r.guid, r.profile_id});
                        }
                        auto assignment = RoleWeightedSplit::ComputeBatchAssignment(
                            slots, RoleWeightedSplit::TeamState{},
                            RoleWeightedSplit::TeamState{});
                        for (const auto& [guid, tf] : assignment) {
                            MatchmakingService::SetPreAssignedTeam(successor, guid, tf);
                        }
                        Logger::Log("matchmaking",
                            "[IpcServer] MISSION_ENDED pre-assigned %zu player(s) to successor=%lld policy=%s\n",
                            assignment.size(), (long long)successor,
                            queue_cfg->taskforce_policy == TaskforcePolicy::BalancedPvp
                                ? "balanced_pvp" : "balanced");
                    } else {
                        Logger::Log("matchmaking",
                            "[IpcServer] MISSION_ENDED parent=%lld policy not balanced — skipping pre-assignment\n",
                            (long long)inst_id);
                    }
                } else {
                    Logger::Log("matchmaking",
                        "[IpcServer] MISSION_ENDED parent=%lld has no queue_id — skipping pre-assignment\n",
                        (long long)inst_id);
                }
            }
            schedule_mission_end_force_exit(inst_id);
        }
        else if (type == IpcProtocol::MSG_REQUEST_SUCCESSOR) {
            // Pre-warm trigger: spawn a successor instance bound to this parent
            // via predecessor_instance_id. Dedupe against any existing non-
            // STOPPED successor (DRAFTING, READY, or STARTING — covers in-
            // flight spawns from a previous duplicate request).
            int64_t parent_id = j.value("instance_id", (int64_t)0);
            if (parent_id == 0) parent_id = instance_id_;
            auto existing = InstanceRegistry::GetSuccessor(parent_id);
            if (existing) {
                Logger::Log("ipc", "[IpcServer] REQUEST_SUCCESSOR parent=%lld → already has successor %lld state=%s (dedupe, no spawn)\n",
                    (long long)parent_id, (long long)existing->instance_id, existing->state.c_str());
            } else {
                IpcServer::TriggerSuccessor(parent_id);
            }
        }
        else if (type == IpcProtocol::MSG_REQUEST_REBALANCE) {
            // ~5s before setup ends. Rebalance the live roster using the
            // queue's taskforce policy, dispatching only the required moves.
            int64_t inst_id = j.value("instance_id", (int64_t)0);
            if (inst_id == 0) inst_id = instance_id_;

            auto inst = InstanceRegistry::GetInstanceById(inst_id);
            if (!inst || inst->queue_id == 0) {
                Logger::Log("team-balance",
                    "[IpcServer] REQUEST_REBALANCE inst=%lld has no queue — skipping\n",
                    (long long)inst_id);
            } else {
                auto queue_cfg = MatchmakingService::GetQueueConfig(inst->queue_id);
                const bool is_balanced =
                    queue_cfg && (queue_cfg->taskforce_policy == TaskforcePolicy::Balanced
                               || queue_cfg->taskforce_policy == TaskforcePolicy::BalancedPvp);
                if (!is_balanced) {
                    Logger::Log("team-balance",
                        "[IpcServer] REQUEST_REBALANCE inst=%lld policy not balanced — skipping\n",
                        (long long)inst_id);
                } else {
                    auto rows = InstanceRegistry::GetActivePlayersForInstance(inst_id);
                    std::vector<RoleWeightedSplit::RosterEntry> roster;
                    roster.reserve(rows.size());
                    for (const auto& r : rows) {
                        roster.push_back({r.guid, r.profile_id, r.task_force});
                    }
                    auto delta = RoleWeightedSplit::ComputeRebalanceDelta(roster);
                    if (!RoleWeightedSplit::ShouldRebalance(roster, delta)) {
                        Logger::Log("team-balance",
                            "[IpcServer] REQUEST_REBALANCE inst=%lld balanced (players=%zu) — no moves\n",
                            (long long)inst_id, roster.size());
                    } else {
                        Logger::Log("team-balance",
                            "[IpcServer] REQUEST_REBALANCE inst=%lld moving %zu of %zu player(s)\n",
                            (long long)inst_id, delta.size(), roster.size());
                        for (const auto& m : delta) {
                            ChatCommand::DispatchTeamMove(inst_id, m.first, m.second);
                        }
                    }
                }
            }
        }
        else {
            Logger::Log("ipc", "[IpcServer] Unknown message type: %s\n", type.c_str());
        }
    }

    void schedule_mission_end_force_exit(int64_t instance_id) {
        if (instance_id == 0) {
            return;
        }

        auto timer = std::make_shared<asio::steady_timer>(socket_.get_executor());
        timer->expires_after(std::chrono::seconds(30));
        Logger::Log("ipc",
            "[IpcServer] MISSION_ENDED: force-exit timer armed parent=%lld delay=30s\n",
            (long long)instance_id);
        timer->async_wait([timer, instance_id](const asio::error_code& ec) {
            if (ec) {
                return;
            }

            auto inst = InstanceRegistry::GetInstanceById(instance_id);
            if (!inst || inst->state == "STOPPED" || inst->is_home_map
                || inst->end_mission_at == 0) {
                return;
            }

            auto roster = InstanceRegistry::GetActivePlayersForInstance(instance_id);
            Logger::Log("ipc",
                "[IpcServer] MISSION_ENDED: force-exit firing parent=%lld players=%zu\n",
                (long long)instance_id, roster.size());
            for (const auto& row : roster) {
                TcpSession::ForceMissionExit(row.guid, instance_id,
                    "mission ended timeout");
            }
            StopNonHomeInstanceIfEmpty(instance_id, "mission ended force-exit");
        });
    }

    void handle_admin_action(const nlohmann::json& j) {
        nlohmann::json ack;
        ack["type"] = IpcProtocol::MSG_ADMIN_ACTION_ACK;
        ack["subtype"] = j.value("subtype", "");
        ack["success"] = false;

        asio::error_code ep_ec;
        auto remote = socket_.remote_endpoint(ep_ec);
        if (ep_ec || !IsPrivateOrLocalAddress(remote.address())) {
            Logger::Log("ipc", "[IpcServer] ADMIN_ACTION rejected: non-local/private source\n");
            ack["message"] = "admin actions are only accepted from local/private networks";
            send(ack.dump());
            return;
        }

        if (IpcServer::admin_token_.empty()) {
            Logger::Log("ipc", "[IpcServer] ADMIN_ACTION rejected: admin_token not configured\n");
            ack["message"] = "admin actions are disabled";
            send(ack.dump());
            return;
        }

        std::string token = j.value("token", "");
        if (token != IpcServer::admin_token_) {
            Logger::Log("ipc", "[IpcServer] ADMIN_ACTION rejected: bad token from %s\n",
                ep_ec ? "unknown" : remote.address().to_string().c_str());
            ack["message"] = "invalid admin token";
            send(ack.dump());
            return;
        }

        if (!IpcServer::admin_action_handler_) {
            ack["message"] = "admin action handler is not configured";
            send(ack.dump());
            return;
        }

        std::string message;
        const std::string subtype = j.value("subtype", "");
        const bool ok = IpcServer::admin_action_handler_(subtype, j, message);
        ack["success"] = ok;
        ack["message"] = message;
        send(ack.dump());
    }

    // ── Write pipeline (write_in_progress chain) ─────────────────────────────

    void send(const std::string& json_msg) {
        // Encode and enqueue. Kick write chain if not already running.
        write_queue_.push_back(IpcFraming::Encode(json_msg));
        if (!write_in_progress_) {
            write_in_progress_ = true;
            do_write();
        }
    }

    void do_write() {
        if (write_queue_.empty()) {
            write_in_progress_ = false;
            return;
        }

        // Keep a shared reference to the frame buffer alive across the async op.
        auto frame = std::make_shared<std::string>(std::move(write_queue_.front()));
        write_queue_.pop_front();

        auto self = shared_from_this();
        asio::async_write(
            socket_,
            asio::buffer(*frame),
            [this, self, frame](const asio::error_code& ec, std::size_t /*bytes*/) {
                if (ec) {
                    Logger::Log("ipc", "[IpcServer] Write error: %s\n",
                        ec.message().c_str());
                    write_in_progress_ = false;
                    return;
                }
                // Chain next write.
                do_write();
            }
        );
    }

    // ── Members ─────────────────────────────────────────────────────────────

    asio::ip::tcp::socket    socket_;
    uint8_t                  header_buf_[4];
    std::vector<uint8_t>     body_buf_;
    std::deque<std::string>  write_queue_;
    bool                     write_in_progress_;
    int64_t                  instance_id_;   // 0 until INSTANCE_HELLO validated
    bool                     validated_;     // true after INSTANCE_HELLO accepted
    bool                     is_home_map_ = false;  // cached at HELLO; gates match-stats writes

    // IpcServer::SendToInstance calls IpcSession::send() -- grant access via friend.
    friend class IpcServer;

    // g_sessions and sessions_mutex_ accessed from dispatch() and on_disconnect().
    static std::mutex sessions_mutex_;
    static std::map<int64_t, std::weak_ptr<IpcSession>> g_sessions;
};

// ---------------------------------------------------------------------------
// IpcSession static members (defined here, in IpcServer.cpp TU)
// ---------------------------------------------------------------------------

std::mutex IpcSession::sessions_mutex_;
std::map<int64_t, std::weak_ptr<IpcSession>> IpcSession::g_sessions;

// ---------------------------------------------------------------------------
// IpcServer static members
// ---------------------------------------------------------------------------

std::mutex IpcServer::ack_mutex_;
std::map<std::string, std::function<void(bool, int)>> IpcServer::pending_acks_;
IpcServer::SuccessorSpawner IpcServer::successor_spawner_;
std::string IpcServer::admin_token_;
IpcServer::AdminActionHandler IpcServer::admin_action_handler_;

void IpcServer::SetSuccessorSpawner(SuccessorSpawner cb) {
    successor_spawner_ = std::move(cb);
}

void IpcServer::TriggerSuccessor(int64_t parent_instance_id) {
    if (successor_spawner_) {
        Logger::Log("ipc", "[IpcServer] REQUEST_SUCCESSOR parent=%lld → invoking spawner\n",
            (long long)parent_instance_id);
        successor_spawner_(parent_instance_id);
    } else {
        Logger::Log("ipc", "[IpcServer] REQUEST_SUCCESSOR parent=%lld but no spawner registered — dropped\n",
            (long long)parent_instance_id);
    }
}

void IpcServer::SetAdminToken(std::string token) {
    admin_token_ = std::move(token);
}

void IpcServer::SetAdminActionHandler(AdminActionHandler cb) {
    admin_action_handler_ = std::move(cb);
}

// ---------------------------------------------------------------------------
// IpcServer -- accepts game instance connections on the IPC port
// ---------------------------------------------------------------------------

IpcServer::IpcServer(asio::io_context& io, uint16_t port)
    : io_(io) {
    asio::error_code ec;
    acceptor_ = std::make_unique<asio::ip::tcp::acceptor>(io.get_executor());
    asio::ip::tcp::endpoint endpoint(asio::ip::tcp::v4(), port);
    acceptor_->open(endpoint.protocol(), ec);
    if (ec) {
        Logger::Log("ipc", "[IpcServer] Open failed on port %d: %s\n", port, ec.message().c_str());
        return;
    }
    // Restore the implicit SO_REUSEADDR that asio's single-arg-endpoint
    // acceptor ctor used to set. Without it, restarting while a spawned
    // game instance's IPC socket lingers in TIME_WAIT makes bind() fail
    // with EADDRINUSE for up to 60s.
    acceptor_->set_option(asio::socket_base::reuse_address(true), ec);
    if (ec) {
        Logger::Log("ipc", "[IpcServer] set_option(reuse_address) failed on port %d: %s\n", port, ec.message().c_str());
        acceptor_->close();
        return;
    }
    acceptor_->bind(endpoint, ec);
    if (ec) {
        Logger::Log("ipc", "[IpcServer] Bind failed on port %d: %s\n", port, ec.message().c_str());
        acceptor_->close();
        return;
    }
    acceptor_->listen(asio::socket_base::max_listen_connections, ec);
    if (ec) {
        Logger::Log("ipc", "[IpcServer] Listen failed on port %d: %s\n", port, ec.message().c_str());
        acceptor_->close();
        return;
    }
    listening_ = true;
    Logger::Log("ipc", "[IpcServer] Listening on port %d\n", port);
    do_accept();
}

void IpcServer::do_accept() {
    if (!acceptor_) return;
    acceptor_->async_accept([this](std::error_code ec, asio::ip::tcp::socket sock) {
        if (!ec) {
            // Don't log the raw TCP accept. It fires for every dashboard
            // admin poll (5s cadence from /api/players/online) and floods the
            // log. Genuine game instances log themselves via INSTANCE_HELLO;
            // admin actions log only on validation failure.
            auto session = std::make_shared<IpcSession>(std::move(sock));
            // Sessions are not tracked in g_sessions until INSTANCE_HELLO validates them.
            session->start();
        }
        if (listening_) do_accept();
    });
}

bool IpcServer::SendToInstance(int64_t instance_id, const std::string& json_msg) {
    std::lock_guard<std::mutex> lock(IpcSession::sessions_mutex_);
    auto it = IpcSession::g_sessions.find(instance_id);
    if (it == IpcSession::g_sessions.end()) {
        Logger::Log("ipc", "[IpcServer] SendToInstance: no session for instance_id=%lld\n",
            (long long)instance_id);
        return false;
    }
    auto session = it->second.lock();
    if (!session) {
        IpcSession::g_sessions.erase(it);
        Logger::Log("ipc", "[IpcServer] SendToInstance: expired session for instance_id=%lld\n",
            (long long)instance_id);
        return false;
    }
    session->send(json_msg);
    return true;
}

void IpcServer::RegisterPendingAck(const std::string& session_guid,
                                   std::function<void(bool, int)> callback) {
    std::lock_guard<std::mutex> lock(ack_mutex_);
    pending_acks_[session_guid] = std::move(callback);
}

void IpcServer::ClearPendingAck(const std::string& session_guid) {
    std::lock_guard<std::mutex> lock(ack_mutex_);
    pending_acks_.erase(session_guid);
}

void IpcServer::DeliverAck(const std::string& session_guid, bool success, int pawn_id) {
    std::function<void(bool, int)> cb;
    {
        std::lock_guard<std::mutex> lock(ack_mutex_);
        auto it = pending_acks_.find(session_guid);
        if (it == pending_acks_.end()) {
            Logger::Log("ipc", "[IpcServer] DeliverAck: no pending ACK for guid %s (already timed out?)\n",
                session_guid.c_str());
            return;
        }
        cb = std::move(it->second);
        pending_acks_.erase(it);
    }
    cb(success, pawn_id);
}
