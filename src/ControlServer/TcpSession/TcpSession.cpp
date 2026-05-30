#include "src/ControlServer/TcpSession/TcpSession.hpp"
#include "src/ControlServer/Database/Database.hpp"
#include "src/ControlServer/Loadouts/ArmorLoadouts.hpp"
#include "src/ControlServer/MapGameInfo/MapGameInfo.hpp"
#include "src/ControlServer/MatchmakingService/TicketInfoEncoder.hpp"
#include "src/ControlServer/MatchmakingService/RuntimeStats.hpp"
#include "src/Shared/IpcProtocol.hpp"
#include <set>
#include <map>

// ---------------------------------------------------------------------------
// TcpSession static member definitions
// ---------------------------------------------------------------------------

std::mutex TcpSession::sessions_mutex_;
std::map<std::string, std::weak_ptr<TcpSession>> TcpSession::g_sessions_;
std::function<void()> TcpSession::on_need_home_map_;
std::string TcpSession::s_host_ = "127.0.0.1";
uint16_t    TcpSession::s_chat_port_ = 9001;

void TcpSession::SetHomeMapSpawner(std::function<void()> cb) {
    on_need_home_map_ = std::move(cb);
}

void TcpSession::EnsureHomeMapWarm(const char* reason) {
    if (!on_need_home_map_) return;
    auto home = InstanceRegistry::GetHomeInstance();
    if (home) return;  // STARTING or READY already exists; spawner would no-op.
    Logger::Log("tcp", "[TcpSession] EnsureHomeMapWarm: spawning on demand (%s)\n",
        reason ? reason : "unspecified");
    on_need_home_map_();
}

void TcpSession::SetNetworkConfig(const std::string& host, uint16_t chat_port) {
    s_host_ = host;
    s_chat_port_ = chat_port;
}

// ---------------------------------------------------------------------------
// TcpSession registry
// ---------------------------------------------------------------------------

void TcpSession::RegisterSession(const std::string& guid, std::weak_ptr<TcpSession> session) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    g_sessions_[guid] = session;
}

void TcpSession::UnregisterSession(const std::string& guid) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    g_sessions_.erase(guid);
}

// ---------------------------------------------------------------------------
// DeliverGameEvent -- route IPC GAME_EVENT to the correct TcpSession
// ---------------------------------------------------------------------------

void TcpSession::DeliverGameEvent(const std::string& session_guid, const nlohmann::json& j) {
    std::shared_ptr<TcpSession> session;
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        auto it = g_sessions_.find(session_guid);
        if (it == g_sessions_.end()) {
            Logger::Log("ipc", "[TcpSession] DeliverGameEvent: no session for guid=%s -- dropped\n",
                session_guid.c_str());
            return;
        }
        session = it->second.lock();
        if (!session) {
            g_sessions_.erase(it);
            Logger::Log("ipc", "[TcpSession] DeliverGameEvent: expired session guid=%s -- dropped\n",
                session_guid.c_str());
            return;
        }
    }

    std::string subtype = j.value("subtype", "");

    if (subtype == "spawn") {
        int pawn_id = j.value("pawn_id", 0);
        session->item_profile_id_ = j.value("item_profile_id", 0);
        Logger::Log("ipc", "[TcpSession] DeliverGameEvent: spawn pawn_id=%d item_profile_id=%d guid=%s\n",
            pawn_id, (int)session->item_profile_id_, session_guid.c_str());
        session->send_inventory_response(pawn_id, session->selected_character_id_);
        // Push the character's skill-tree allocation so the in-game skill UI
        // renders the saved state. Client caches the response; opening the
        // skill screen then doesn't require a server round-trip. A later
        // SendCharacterSkillMarshal RPC (when supported) can trigger another
        // push if the data becomes stale.
        session->send_player_skills_response();
    }
    else if (subtype == "beacon_pickup") {
        int pawn_id            = j.value("pawn_id", 0);
        int device_id          = j.value("device_id", 0);
        int inventory_id       = j.value("inventory_id", 0);
        int equip_slot_value_id = j.value("equip_slot_value_id", 0);
        Logger::Log("ipc", "[TcpSession] DeliverGameEvent: beacon_pickup pawn=%d dev=%d inv=%d slot=%d\n",
            pawn_id, device_id, inventory_id, equip_slot_value_id);
        session->send_beacon_pickup_response(pawn_id, device_id, inventory_id, equip_slot_value_id);
    }
    else if (subtype == "beacon_remove") {
        int pawn_id      = j.value("pawn_id", 0);
        int inventory_id = j.value("inventory_id", 0);
        Logger::Log("ipc", "[TcpSession] DeliverGameEvent: beacon_remove pawn=%d inv=%d\n",
            pawn_id, inventory_id);
        session->send_beacon_remove_response(pawn_id, inventory_id);
    }
    else if (subtype == "possess_show_loadout") {
        int pawn_id = j.value("pawn_id", 0);
        std::vector<TcpSession::LoadoutItem> items;
        if (j.contains("devices") && j["devices"].is_array()) {
            for (const auto& d : j["devices"]) {
                TcpSession::LoadoutItem it{};
                it.device_id     = d.value("device_id", 0);
                it.inventory_id  = d.value("inventory_id", 0);
                it.quality       = d.value("quality", 1162);
                it.slot_value_id = d.value("slot_value_id", 0);
                if (it.device_id != 0 && it.inventory_id != 0 && it.slot_value_id != 0) {
                    items.push_back(it);
                }
            }
        }
        Logger::Log("ipc", "[TcpSession] DeliverGameEvent: possess_show_loadout pawn=%d items=%d\n",
            pawn_id, (int)items.size());
        session->send_loadout_inventory_response(pawn_id, items);
    }
    else if (subtype == "possess_restore_inventory") {
        int     pawn_id      = j.value("pawn_id", 0);
        int64_t character_id = j.value("character_id", (int64_t)0);
        Logger::Log("ipc", "[TcpSession] DeliverGameEvent: possess_restore_inventory pawn=%d char=%lld\n",
            pawn_id, (long long)character_id);
        if (pawn_id != 0 && character_id != 0) {
            session->send_inventory_response(pawn_id, character_id);
        }
    }
    else if (subtype == "skill_request") {
        Logger::Log("ipc", "[TcpSession] DeliverGameEvent: skill_request guid=%s\n",
            session_guid.c_str());
        session->send_player_skills_response();
    }
    else if (subtype == "skill_save") {
        int64_t character_id = j.value("character_id", (int64_t)0);
        int     pawn_id      = j.value("pawn_id", 0);
        if (j.contains("item_profile_id")) {
            int32_t ipid = j.value("item_profile_id", 0);
            if (ipid >= 0) session->item_profile_id_ = ipid;
        }
        std::vector<SkillRow> skills;
        if (j.contains("skills") && j["skills"].is_array()) {
            for (const auto& s : j["skills"]) {
                SkillRow r{};
                r.skill_group_id = s.value("skill_group_id", 0);
                r.skill_id       = s.value("skill_id", 0);
                r.points         = s.value("points", 0);
                if (r.skill_group_id && r.skill_id && r.points > 0)
                    skills.push_back(r);
            }
        }
        // Opcode 0x00A0 is used bidirectionally: the client also sends it
        // with an empty DATA_SET_CHARACTER_SKILLS array to REQUEST the
        // server's current state (observed on skill-UI reopen). Treat an
        // empty inbound as a request — don't wipe the DB. Real respec goes
        // through the separate skill_respec IPC subtype.
        Logger::Log("ipc",
            "[TcpSession] DeliverGameEvent: skill_save char=%lld pawnId=%d rows=%d guid=%s%s\n",
            (long long)character_id, pawn_id, (int)skills.size(), session_guid.c_str(),
            skills.empty() ? " (empty -> treated as REQUEST, not persisting)" : "");
        if (character_id != 0 && !skills.empty()) {
            PlayerSessionStore::SetSkillsForCharacter(character_id, skills);
        }
        // Echo current DB state so the UI's inbound 0x00A0 handler has
        // authoritative data whether this was a save or a request.
        session->send_player_skills_response();
        // After a skill change, also push SEND_INVENTORY so the client's
        // per-device stat panels re-render against the new buff state.
        // Only when pawn_id is provided (game-server-forwarded path) AND
        // this was a real save (skills non-empty) — a request-shaped empty
        // marshal shouldn't trigger a redundant inventory blast.
        if (pawn_id != 0 && !skills.empty() && character_id != 0) {
            session->send_inventory_response(pawn_id, character_id);
        }
    }
    else if (subtype == "skill_respec") {
        int64_t character_id = j.value("character_id", (int64_t)0);
        int     pawn_id      = j.value("pawn_id", 0);
        Logger::Log("ipc", "[TcpSession] DeliverGameEvent: skill_respec char=%lld pawnId=%d\n",
            (long long)character_id, pawn_id);
        if (character_id != 0) {
            PlayerSessionStore::ClearSkillsForCharacter(character_id);
        }
        // Echo the zeroed allocation to the client so the UI matches.
        session->send_player_skills_response();
        // Trainer respec wipes everything → also re-send inventory so
        // per-device stat displays drop the previously-buffed numbers.
        if (pawn_id != 0 && character_id != 0) {
            session->send_inventory_response(pawn_id, character_id);
        }
    }
    else if (subtype == "equip_save") {
        // Client's equip-screen save (RPC ServerAcceptNewProfileFromEquipScreen).
        // Payload shape:
        //   character_id, pawn_id, loadout_profile (1 today — loadout slots
        //   1..5 are a future-scope feature, we ignore the value for now),
        //   slot_to_inventory: { "equip_point": inventory_id, ... }  -- the
        //                        client's FTGEQUIP_SLOTS_STRUCT.SlotIndices[]
        //                        for engine equip-points 1..24 (weapons +
        //                        cosmetics).
        //   misc_items:         { "index": inventory_id, ... }  -- the
        //                        client's FTGEQUIP_SLOTS_STRUCT.MiscItems[],
        //                        which (hypothesis) carries armor equip
        //                        changes keyed by group-126 sort_order.
        //                        Indexing scheme is the open question for
        //                        Phase 4 swap-armor; this log line + the
        //                        per-entry breakdown above are what we use
        //                        to decode it from real packet data.
        // Validation + persistence lives in PlayerSessionStore.
        const int64_t character_id    = j.value("character_id", (int64_t)0);
        const int     pawn_id         = j.value("pawn_id", 0);
        const int     loadout_profile = j.value("loadout_profile", 1);
        std::map<int, int> slot_to_inventory;
        if (j.contains("slot_to_inventory") && j["slot_to_inventory"].is_object()) {
            for (auto it = j["slot_to_inventory"].begin(); it != j["slot_to_inventory"].end(); ++it) {
                const int equip_point  = std::atoi(it.key().c_str());
                const int inventory_id = it.value().get<int>();
                if (equip_point > 0 && inventory_id > 0) {
                    slot_to_inventory[equip_point] = inventory_id;
                }
            }
        }
        std::map<int, int> misc_items;
        if (j.contains("misc_items") && j["misc_items"].is_object()) {
            for (auto it = j["misc_items"].begin(); it != j["misc_items"].end(); ++it) {
                const int idx          = std::atoi(it.key().c_str());
                const int inventory_id = it.value().get<int>();
                if (inventory_id > 0) {
                    misc_items[idx] = inventory_id;
                }
            }
        }
        Logger::Log("armor",
            "[equip_save] char=%lld pawnId=%d loadout=%d slots=%zu misc=%zu guid=%s\n",
            (long long)character_id, pawn_id, loadout_profile,
            slot_to_inventory.size(), misc_items.size(), session_guid.c_str());
        for (const auto& kv : slot_to_inventory) {
            Logger::Log("armor",
                "[equip_save]   SlotIndices[%d] = inv_id %d\n", kv.first, kv.second);
        }
        for (const auto& kv : misc_items) {
            Logger::Log("armor",
                "[equip_save]   MiscItems[%d]   = inv_id %d\n", kv.first, kv.second);
        }

        if (character_id != 0) {
            const bool ok = PlayerSessionStore::SaveEquippedDevices(
                character_id, session->user_id_, session->selected_profile_id_,
                slot_to_inventory, misc_items);
            if (!ok) {
                Logger::Log("armor",
                    "[equip_save] REJECTED — DB state unchanged; client will resync from existing\n");
            }
            // Push the authoritative equipped set back regardless of
            // accept/reject — accept = client sees its save; reject = client's
            // optimistic UI gets corrected to what's actually persisted.
            if (pawn_id != 0) {
                session->send_inventory_response(pawn_id, character_id);
            }
        }
    }
    else if (subtype == "quest") {
        int64_t character_id = j.value("character_id", (int64_t)0);
        int quest_id         = j.value("quest_id", 0);
        bool abandon         = j.value("abandon", false);

        Logger::Log("ipc", "[TcpSession] DeliverGameEvent: quest char=%lld quest=%d abandon=%d\n",
            (long long)character_id, quest_id, (int)abandon);

        if (abandon) {
            PlayerSessionStore::Quests().Abandon(character_id, quest_id);
            session->send_quest_abandon_response(quest_id);
        } else {
            std::string status = PlayerSessionStore::Quests().GetStatus(character_id, quest_id);
            if (status.empty()) {
                PlayerSessionStore::Quests().Accept(character_id, quest_id);
                session->send_quest_accept_response(quest_id);
            } else if (status == "active") {
                PlayerSessionStore::Quests().Complete(character_id, quest_id);
                session->send_quest_complete_response(quest_id);
            } else {
                // Already complete -- ignore
                Logger::Log("ipc", "[TcpSession] DeliverGameEvent: quest already complete, ignored\n");
            }
        }
    }
    else {
        Logger::Log("ipc", "[TcpSession] DeliverGameEvent: unknown subtype=%s\n", subtype.c_str());
    }
}

// ---------------------------------------------------------------------------
// DeliverPlayerAction -- forward a PLAYER_ACTION IPC to the game-DLL instance
// the player is currently assigned to. Mirrors DeliverGameEvent's locking
// pattern. Silent on failure (logged on the "chat-command" channel).
// ---------------------------------------------------------------------------

bool TcpSession::DeliverPlayerAction(const std::string& session_guid, const nlohmann::json& payload) {
    std::shared_ptr<TcpSession> session;
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        auto it = g_sessions_.find(session_guid);
        if (it == g_sessions_.end()) {
            Logger::Log("chat-command", "[ChatCmd] DeliverPlayerAction: no TcpSession for guid=%s\n",
                session_guid.c_str());
            return false;
        }
        session = it->second.lock();
        if (!session) {
            g_sessions_.erase(it);
            Logger::Log("chat-command", "[ChatCmd] DeliverPlayerAction: expired TcpSession for guid=%s\n",
                session_guid.c_str());
            return false;
        }
    }
    if (session->assigned_instance_id_ == 0) {
        Logger::Log("chat-command", "[ChatCmd] DeliverPlayerAction: guid=%s has no assigned instance\n",
            session_guid.c_str());
        return false;
    }
    bool ok = IpcServer::SendToInstance(session->assigned_instance_id_, payload.dump());
    Logger::Log("chat-command", "[ChatCmd] DeliverPlayerAction: guid=%s instance=%lld send=%d\n",
        session_guid.c_str(), (long long)session->assigned_instance_id_, (int)ok);
    return ok;
}

bool TcpSession::AdminMovePlayerToInstance(const std::string& session_guid,
                                           int64_t target_instance_id,
                                           int target_task_force,
                                           int64_t source_instance_id,
                                           int source_task_force,
                                           std::string& message) {
    if (target_instance_id == 0) {
        message = "missing target_instance_id";
        return false;
    }
    if (target_task_force != 1 && target_task_force != 2) {
        message = "target_task_force must be 1 or 2";
        return false;
    }

    std::shared_ptr<TcpSession> session;
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        auto it = g_sessions_.find(session_guid);
        if (it == g_sessions_.end()) {
            message = "player session not found";
            return false;
        }
        session = it->second.lock();
        if (!session) {
            g_sessions_.erase(it);
            message = "player session expired";
            return false;
        }
    }

    auto target = InstanceRegistry::GetInstanceById(target_instance_id);
    if (!target) {
        message = "target instance not found";
        return false;
    }
    if (target->state != "READY") {
        message = "target instance is not READY";
        return false;
    }

    int64_t current_instance_id = session->assigned_instance_id_;
    if (current_instance_id == 0 && source_instance_id != 0) {
        current_instance_id = source_instance_id;
    }

    if (current_instance_id == target_instance_id) {
        if (source_task_force == target_task_force) {
            message = "already on requested side";
            return true;
        }

        nlohmann::json action;
        action["type"] = IpcProtocol::MSG_PLAYER_ACTION;
        action["session_guid"] = session_guid;
        action["action"] = "change_team";
        action["args"] = {
            {"target", target_task_force == 1 ? "attackers" : "defenders"}
        };

        bool ok = IpcServer::SendToInstance(current_instance_id, action.dump());
        if (!ok) {
            message = "failed to send change_team action";
            return false;
        }

        InstanceRegistry::UpdateInstancePlayerTaskForce(current_instance_id, session_guid,
            target_task_force);
        message = "team change requested";
        return true;
    }

    if (current_instance_id != 0) {
        InstanceRegistry::MarkInstancePlayerLeft(current_instance_id, session_guid);
    }

    Logger::Log("tcp", "[TcpSession] Admin move player %s from instance=%lld to instance=%lld tf=%d\n",
        session_guid.c_str(), (long long)current_instance_id, (long long)target_instance_id,
        target_task_force);
    session->initiate_player_register_for_target(*target, target_task_force);
    message = "instance move requested";
    return true;
}

void TcpSession::initiate_player_register_and_go_play() {
    // Build PLAYER_REGISTER JSON with full character state.
    auto session = PlayerSessionStore::GetByGuid(session_guid_);
    if (!session) {
        Logger::Log("tcp", "[TcpSession] Cannot send PLAYER_REGISTER: session %s not found\n",
            session_guid_.c_str());
        return;
    }

    nlohmann::json reg;
    reg["type"]         = IpcProtocol::MSG_PLAYER_REGISTER;
    reg["session_guid"] = session_guid_;
    reg["profile_id"]   = selected_profile_id_;
    reg["player_name"]  = player_name;
    reg["user_id"]      = user_id_;
    reg["character_id"] = selected_character_id_;
    reg["task_force"]   = 1;  // Hardcoded per user decision; matchmaking decides later

    // Fetch character for morph_data, head_asm_id, gender_type_value_id
    auto charInfo = PlayerSessionStore::GetCharacterById(selected_character_id_);
    if (charInfo) {
        reg["head_asm_id"]          = charInfo->head_asm_id;
        reg["gender_type_value_id"] = charInfo->gender_type_value_id;
        reg["morph_data"]           = HexUtils::hex_encode(charInfo->morph_data);
    } else {
        reg["head_asm_id"]          = 0;
        reg["gender_type_value_id"] = 0;
        reg["morph_data"]           = "";
    }

    // Embed the character's skill-tree allocation so the game server can apply
    // skill-based effect groups during pawn spawn without a second IPC round-
    // trip. Saves (inbound 0xA0) persist to the same DB, so the next spawn or
    // PLAYER_REGISTER sees fresh rows.
    {
        nlohmann::json skillsArr = nlohmann::json::array();
        for (const auto& s : PlayerSessionStore::GetSkillsForCharacter(selected_character_id_)) {
            nlohmann::json row;
            row["skill_group_id"] = s.skill_group_id;
            row["skill_id"]       = s.skill_id;
            row["points"]         = s.points;
            skillsArr.push_back(row);
        }
        reg["skills"] = skillsArr;
        reg["last_respec_at"] = PlayerSessionStore::GetLastRespecAt(selected_character_id_);
    }

    // Arm 60-second ACK-wait timer.
    auto self(shared_from_this());
    pending_ack_timer_ = std::make_shared<asio::steady_timer>(io_ctx_);
    pending_ack_timer_->expires_after(std::chrono::seconds(60));

    // Register callback for when ACK arrives.
    auto timer = pending_ack_timer_;  // capture by value
    IpcServer::RegisterPendingAck(session_guid_,
        [this, self, timer](bool success, int pawn_id) {
            timer->cancel();
            if (success) {
                Logger::Log("tcp", "[TcpSession] PLAYER_REGISTER ACK success for %s pawn_id=%d\n",
                    session_guid_.c_str(), pawn_id);
                // Track player in home map instance
                auto home = InstanceRegistry::GetReadyHomeInstance();
                if (home) {
                    InstanceRegistry::InsertInstancePlayer(
                        home->instance_id, session_guid_, selected_character_id_, 1,
                        selected_profile_id_);
                }
                send_go_play_response();
            } else {
                Logger::Log("tcp", "[TcpSession] PLAYER_REGISTER ACK failed for %s\n",
                    session_guid_.c_str());
                // Silent: do not send go_play, do not send error to client
            }
        });

    // Look up the home map instance to route PLAYER_REGISTER to the correct session.
    auto home_instance = InstanceRegistry::GetReadyHomeInstance();
    if (!home_instance) {
        Logger::Log("tcp", "[TcpSession] No READY home map instance -- cannot send PLAYER_REGISTER for %s\n",
            session_guid_.c_str());
        pending_ack_timer_->cancel();
        IpcServer::ClearPendingAck(session_guid_);
        pending_ack_timer_.reset();
        return;
    }

    // Send PLAYER_REGISTER to game instance.
    if (!IpcServer::SendToInstance(home_instance->instance_id, reg.dump())) {
        Logger::Log("tcp", "[TcpSession] No IPC session -- cannot send PLAYER_REGISTER for %s\n",
            session_guid_.c_str());
        pending_ack_timer_->cancel();
        IpcServer::ClearPendingAck(session_guid_);
        pending_ack_timer_.reset();
        return;
    }

    // Timeout handler.
    pending_ack_timer_->async_wait([this, self, timer](std::error_code ec) {
        if (!ec) {
            // Timer expired (60-second timeout). ACK never arrived.
            Logger::Log("tcp", "[TcpSession] PLAYER_REGISTER ACK timeout for %s\n",
                session_guid_.c_str());
            IpcServer::ClearPendingAck(session_guid_);
            // Silent: do not send go_play, do not send error to client
        }
        // ec == operation_aborted means timer was cancelled (ACK arrived or disconnect) -- do nothing
    });
}


void TcpSession::initiate_player_register_for_target(const InstanceInfo& target, int task_force) {
    auto session = PlayerSessionStore::GetByGuid(session_guid_);
    if (!session) {
        Logger::Log("tcp", "[TcpSession] Cannot send PLAYER_REGISTER (target): session %s not found\n",
            session_guid_.c_str());
        return;
    }

    nlohmann::json reg;
    reg["type"]         = IpcProtocol::MSG_PLAYER_REGISTER;
    reg["session_guid"] = session_guid_;
    reg["profile_id"]   = selected_profile_id_;
    reg["player_name"]  = player_name;
    reg["user_id"]      = user_id_;
    reg["character_id"] = selected_character_id_;
    reg["task_force"]   = task_force;

    auto charInfo = PlayerSessionStore::GetCharacterById(selected_character_id_);
    if (charInfo) {
        reg["head_asm_id"]          = charInfo->head_asm_id;
        reg["gender_type_value_id"] = charInfo->gender_type_value_id;
        reg["morph_data"]           = HexUtils::hex_encode(charInfo->morph_data);
    } else {
        reg["head_asm_id"]          = 0;
        reg["gender_type_value_id"] = 0;
        reg["morph_data"]           = "";
    }

    {
        nlohmann::json skillsArr = nlohmann::json::array();
        for (const auto& s : PlayerSessionStore::GetSkillsForCharacter(selected_character_id_)) {
            nlohmann::json row;
            row["skill_group_id"] = s.skill_group_id;
            row["skill_id"]       = s.skill_id;
            row["points"]         = s.points;
            skillsArr.push_back(row);
        }
        reg["skills"] = skillsArr;
        reg["last_respec_at"] = PlayerSessionStore::GetLastRespecAt(selected_character_id_);
    }

    int64_t target_instance_id = target.instance_id;
    std::string target_map = target.map_name;

    // ACK timer — same 60s budget as the home path.
    auto self(shared_from_this());
    pending_ack_timer_ = std::make_shared<asio::steady_timer>(io_ctx_);
    pending_ack_timer_->expires_after(std::chrono::seconds(60));
    auto timer = pending_ack_timer_;

    IpcServer::RegisterPendingAck(session_guid_,
        [this, self, timer, target_instance_id, target_map, task_force](bool success, int pawn_id) {
            timer->cancel();
            if (success) {
                Logger::Log("tcp", "[TcpSession] PLAYER_REGISTER ACK (target) success for %s instance=%lld pawn_id=%d\n",
                    session_guid_.c_str(), (long long)target_instance_id, pawn_id);
                // Bind player to the successor instance and dispatch GSC_GO_PLAY.
                assigned_instance_id_ = target_instance_id;
                InstanceRegistry::InsertInstancePlayer(
                    target_instance_id, session_guid_, selected_character_id_, task_force,
                    selected_profile_id_);
                auto fresh = InstanceRegistry::GetInstanceById(target_instance_id);
                if (fresh && fresh->state == "READY") {
                    send_go_play_to_instance(*fresh, task_force);
                } else {
                    Logger::Log("tcp", "[TcpSession] Successor instance %lld no longer READY (state=%s) — falling back to home\n",
                        (long long)target_instance_id, fresh ? fresh->state.c_str() : "<missing>");
                    assigned_instance_id_ = 0;
                    wait_for_home_map_then_register(120);
                }
            } else {
                // ACK failure usually means "no seat" (successor full, or game
                // server rejected for some other reason). Fall back to home so
                // the player still ends up somewhere.
                Logger::Log("tcp", "[TcpSession] PLAYER_REGISTER ACK (target) failed for %s instance=%lld — falling back to home\n",
                    session_guid_.c_str(), (long long)target_instance_id);
                assigned_instance_id_ = 0;
                wait_for_home_map_then_register(120);
            }
        });

    if (!IpcServer::SendToInstance(target_instance_id, reg.dump())) {
        Logger::Log("tcp", "[TcpSession] No IPC session for successor %lld — falling back to home for %s\n",
            (long long)target_instance_id, session_guid_.c_str());
        pending_ack_timer_->cancel();
        IpcServer::ClearPendingAck(session_guid_);
        pending_ack_timer_.reset();
        wait_for_home_map_then_register(120);
        return;
    }

    pending_ack_timer_->async_wait([this, self, timer, target_instance_id](std::error_code ec) {
        if (!ec) {
            Logger::Log("tcp", "[TcpSession] PLAYER_REGISTER ACK (target) timeout for %s instance=%lld — falling back to home\n",
                session_guid_.c_str(), (long long)target_instance_id);
            IpcServer::ClearPendingAck(session_guid_);
            wait_for_home_map_then_register(120);
        }
    });
}


void TcpSession::wait_for_home_map_then_register(int remaining_seconds) {
    if (remaining_seconds <= 0) {
        Logger::Log("tcp", "[TcpSession] Home map instance not ready (timeout): %s\n",
            session_guid_.c_str());
        return;  // Client stays on loading screen; no go_play sent
    }

    // Check for READY home instance first
    auto ready = InstanceRegistry::GetReadyHomeInstance();
    if (ready) {
        assigned_instance_id_ = ready->instance_id;
        Logger::Log("tcp", "[TcpSession] Home map ready: instance_id=%lld for %s\n",
            assigned_instance_id_, session_guid_.c_str());
        initiate_player_register_and_go_play();
        return;
    }

    // Check if a home instance is STARTING (someone else triggered it)
    auto home = InstanceRegistry::GetHomeInstance();
    if (!home && on_need_home_map_) {
        // No home instance at all — spawn one
        Logger::Log("tcp", "[TcpSession] No home map instance — spawning on demand for %s\n",
            session_guid_.c_str());
        on_need_home_map_();
    }

    // Poll again in 2 seconds
    auto self(shared_from_this());
    auto timer = std::make_shared<asio::steady_timer>(io_ctx_);
    timer->expires_after(std::chrono::seconds(2));
    timer->async_wait([this, self, timer, remaining_seconds](std::error_code ec) {
        if (!ec) {
            wait_for_home_map_then_register(remaining_seconds - 2);
        }
    });
}


void TcpSession::handle_packet(const uint8_t* data, size_t length) {
	if (length < 6) {
		Logger::Log("tcp", "[%s] Packet too short\n", Logger::GetTime());
		return;
	}

	const uint8_t* p = data + 2;
	uint16_t packet_type = p[0] | (p[1] << 8);
	uint16_t item_count  = p[2] | (p[3] << 8);

	switch (packet_type) {
		case GA_U::GSC_USER_LOGIN: {

			PacketView pkt(data + 6, length - 6);
			player_name = pkt.ReadString(GA_T::USER_NAME).value_or("unknown");
			session_guid_ = GenerateSessionGuid();
			RegisterSession(session_guid_, shared_from_this());
			ip_address_ = socket_.remote_endpoint().address().to_string() + ":" + std::to_string(socket_.remote_endpoint().port());
			user_id_ = PlayerSessionStore::UpsertUser(player_name);

			{
				SessionInfo info;
				info.session_guid = session_guid_;
				info.player_name = player_name;
				info.ip_address = ip_address_;
				info.user_id = user_id_;
				PlayerSessionStore::Register(info);
			}

			Logger::Log("tcp", "[%s] Received: GSC_USER_LOGIN [0x%04X], name: %s guid: %s ip: %s\n",
			   Logger::GetTime(), packet_type, player_name.c_str(), session_guid_.c_str(), ip_address_.c_str());

			// Preemptively ensure a home map instance is starting so it's
			// likely READY by the time the player selects a character.
			EnsureHomeMapWarm("login pre-spawn");

			send_login_response();
			break;
		}
		case GA_U::GSC_CHARACTER_LIST:
			Logger::Log("tcp", "[%s] Received: GSC_CHARACTER_LIST [0x%04X], item count: %d\n", Logger::GetTime(), packet_type, item_count);
			send_character_list_response();
			// send_character_list_queue_response();
			break;
		case GA_U::PLAYER_UPDATE_CLIENT:
			Logger::Log("tcp", "[%s] Received: PLAYER_UPDATE_CLIENT [0x%04X], item count: %d\n", Logger::GetTime(), packet_type, item_count);
			send_player_update_client_response();
			break;
		case GA_U::GET_LOOT_TABLE_ITEMS_BY_ID_FILTERED:
			Logger::Log("tcp", "[%s] Received: GET_LOOT_TABLE_ITEMS_BY_ID_FILTERED [0x%04X], item count: %d\n", Logger::GetTime(), packet_type, item_count);
			send_get_loot_table_items_by_id_filtered_response();
			break;
		case GA_U::RELAY_LOG: {
			// Phase 10: IPC GAME_EVENT will populate event queues here.
			// For now, RELAY_LOG is a keepalive -- no action needed.
			break;
		}
		case GA_U::GSC_SELECT_CHARACTER: {
			PacketView pkt(data + 6, length - 6);
			int64_t nCharacterId = pkt.Read4B(GA_T::CHARACTER_ID).value_or(0);

			Logger::Log("tcp", "[%s] Received: GSC_SELECT_CHARACTER [0x%04X] charId=%lld session_guid=%s\n",
				Logger::GetTime(), packet_type, nCharacterId, session_guid_.c_str());

			LogData(data, length);

			// Look up the requested character from DB and select it.
			if (nCharacterId != 0) {
				auto charInfo = PlayerSessionStore::GetCharacterById(nCharacterId);
				if (charInfo && charInfo->user_id == user_id_) {
					selected_character_id_ = charInfo->id;
					selected_profile_id_   = charInfo->profile_id;
					PlayerSessionStore::SetSelectedCharacter(session_guid_, charInfo->id, charInfo->profile_id);
					// Top up this user's account-scoped inventory pool from
					// ClassLoadouts.cpp. Idempotent — existing inventory rows
					// keep their stable `inventory_id`. Brand-new characters
					// with no ga_character_devices rows also receive an opening
					// equipped loadout in the same call. Stable IDs are what
					// make the equip screen safe between sessions; the previous
					// per-character resync wiped+rewrote on every select, which
					// would now also wipe any equip choices the player made
					// last session.
					PlayerSessionStore::SeedInventoryFromLoadouts(charInfo->user_id);
					Logger::Log("tcp", "[%s] GSC_SELECT_CHARACTER: selected charId=%lld profile=%u\n",
						Logger::GetTime(), selected_character_id_, selected_profile_id_);
				} else {
					Logger::Log("tcp", "[%s] GSC_SELECT_CHARACTER: charId=%lld not found or wrong user (user_id_=%lld)\n",
						Logger::GetTime(), nCharacterId, user_id_);
				}
			}

			send_select_character_response();

			// Wait for a READY home map instance, then send PLAYER_REGISTER.
			wait_for_home_map_then_register(120);  // 120 second timeout

			break;
		}
		case GA_U::PLAYER_LOGOFF:
			Logger::Log("tcp", "[%s] Received: PLAYER_LOGOFF [0x%04X], item count: %d\n", Logger::GetTime(), packet_type, item_count);
			break;
		case GA_U::PLAYER_LEAVE_GAME: {
			// Client sends this when the user clicks "Disconnect" in the in-game
			// menu — TCP stays alive but the player goes back to character select.
			// We must tell the game-DLL instance to tear down the UDP UNetConnection,
			// PlayerController and Pawn for this session_guid right now. Without
			// this the stale connection lingers in NetDriver->ClientConnections
			// until the engine's ConnectionTimeout (~30s), so picking a character
			// again routes the new HELLO into the stale USOCK_Open connection
			// and the client times out back to the login screen.
			Logger::Log("tcp", "[%s] Received: PLAYER_LEAVE_GAME [0x%04X] guid=%s instance=%lld\n",
				Logger::GetTime(), packet_type, session_guid_.c_str(), (long long)assigned_instance_id_);

			if (assigned_instance_id_ != 0 && !session_guid_.empty()) {
				nlohmann::json leave;
				leave["type"]         = IpcProtocol::MSG_PLAYER_LEAVE;
				leave["session_guid"] = session_guid_;
				bool ok = IpcServer::SendToInstance(assigned_instance_id_, leave.dump());
				Logger::Log("tcp", "[TcpSession] PLAYER_LEAVE dispatched guid=%s instance=%lld send=%d\n",
					session_guid_.c_str(), (long long)assigned_instance_id_, (int)ok);
			}

			// Drop the instance assignment so a subsequent SELECT_CHARACTER goes
			// through wait_for_home_map_then_register again from a clean slate.
			assigned_instance_id_      = 0;
			pending_match_instance_id_ = 0;
			pending_match_game_mode_.clear();
			break;
		}
		case GA_U::GSC_CHANGE_INSTANCE: {
			// Sent when the player clicks "End Mission" on the post-match
			// screen OR opens the in-mission escape menu and chooses Exit.
			// Same packet, two scenarios — distinguished by whether the
			// parent's BeginEndMission has fired (end_mission_at IS NOT NULL).
			//
			// Decision matrix:
			//   parent.end_mission_at IS NULL    → in-mission exit, always home
			//   ... NOT NULL, queue.continue=0   → end-mission button, home (vanilla)
			//   ... NOT NULL, queue.continue=1   → end-mission button, try successor
			//                                       — if no successor or full → home fallback
			PacketView pkt(data + 6, length - 6);
			uint32_t targetMapGameId  = pkt.Read4B(GA_T::MAP_GAME_ID).value_or(0);
			uint32_t targetInstanceId = pkt.Read4B(GA_T::MAP_INSTANCE_ID).value_or(0);

			Logger::Log("tcp", "[%s] Received: GSC_CHANGE_INSTANCE [0x%04X] guid=%s mapGameId=%u instanceId=%u (from=%lld)\n",
				Logger::GetTime(), packet_type, session_guid_.c_str(),
				targetMapGameId, targetInstanceId, (long long)assigned_instance_id_);

			if (targetMapGameId != 0 || targetInstanceId != 0) {
				Logger::Log("tcp", "[TcpSession] GSC_CHANGE_INSTANCE: targeted switch not implemented (mapGameId=%u instanceId=%u) — ignoring\n",
					targetMapGameId, targetInstanceId);
				break;
			}

			// Snapshot the parent before we tear down the seat — we need
			// its end_mission_at + queue_id to decide where to route.
			int64_t parent_instance_id = assigned_instance_id_;
			std::optional<InstanceInfo> parent;
			if (parent_instance_id != 0) {
				parent = InstanceRegistry::GetInstanceById(parent_instance_id);
			}

			// Decide BEFORE teardown whether we should attempt to route to a
			// successor. We need to know if (a) parent had ended, (b) the
			// queue opts into continue, and (c) a READY successor exists.
			std::optional<InstanceInfo> successor;
			bool tryContinue = false;
			if (parent && parent->end_mission_at != 0 && parent->queue_id != 0) {
				if (MatchmakingService::GetContinueInQueue(parent->queue_id)) {
					successor = InstanceRegistry::GetSuccessor(parent_instance_id);
					if (successor && successor->state == "READY") {
						// Cheap seat-availability pre-check — final word comes
						// from the PLAYER_REGISTER ACK, which falls back to
						// home on failure.
						bool hasSeat = (successor->max_players == 0)
						            || (successor->player_count < successor->max_players);
						if (hasSeat) {
							tryContinue = true;
						} else {
							Logger::Log("tcp", "[TcpSession] Successor %lld for parent=%lld is full (%d/%d) — routing home instead\n",
								(long long)successor->instance_id, (long long)parent_instance_id,
								successor->player_count, successor->max_players);
						}
					} else {
						Logger::Log("tcp", "[TcpSession] No READY successor for parent=%lld (got %s) — routing home instead\n",
							(long long)parent_instance_id,
							successor ? successor->state.c_str() : "<none>");
					}
				}
			}

			// NB: do NOT send MSG_PLAYER_LEAVE here, even though that would
			// tear the mission-side connection down immediately. PLAYER_LEAVE
			// → NetConnection__Cleanup → engine UNetConnection::Cleanup
			// crashes mid-mission with a null deref at GlobalAgenda.exe+0x4283,
			// because the connection is still fully live (live PlayerController,
			// Pawn, channel array, AI/objective references). The Disconnect
			// path doesn't crash because the client tears down its UDP socket
			// first so the engine has already quiesced the connection by the
			// time we ask it to clean up.
			//
			// Letting the engine's 30s ConnectionTimeout reap the parent
			// connection naturally avoids the crash. The trade-off is purely
			// cosmetic: the departed player remains visible in the parent's
			// scoreboard / r_PRIArray for ~30s. The new home (or successor)
			// PLAYER_REGISTER is a separate instance with its own NetDriver,
			// so nothing races.
			//
			// TODO(crash-debug): find what's null inside CallOriginal at
			// NetConnection__Cleanup.cpp:84 when called on a live connection,
			// then re-enable an immediate teardown here.
			assigned_instance_id_      = 0;
			pending_match_instance_id_ = 0;
			pending_match_game_mode_.clear();

			if (tryContinue) {
				Logger::Log("tcp", "[TcpSession] Continue-in-queue: routing %s to successor %lld of parent=%lld (queue=%u)\n",
					session_guid_.c_str(), (long long)successor->instance_id,
					(long long)parent_instance_id, parent->queue_id);
				// task_force defaults to 1 for now — the matchmaker doesn't
				// yet rebalance survivors between teams on continuation.
				initiate_player_register_for_target(*successor, /*task_force=*/1);
				break;
			}

			// Wait for a READY home map instance, then PLAYER_REGISTER → GSC_GO_PLAY.
			// Identical entry point GSC_SELECT_CHARACTER uses; spawns on demand
			// if no home is starting yet (e.g. instance was shut down for idle).
			wait_for_home_map_then_register(120);
			break;
		}
		case GA_U::ADD_PLAYER_CHARACTER: {
			Logger::Log("tcp", "[%s] Received: ADD_PLAYER_CHARACTER [0x%04X]\n", Logger::GetTime(), packet_type);
			PacketView pkt(data + 6, length - 6);
			// uint32_t trainingMapGameId  = pkt.Read4B(GA_T::TRAINING_MAP_GAME_ID).value_or(0);  // unused: both paths now use initiate_player_register_and_go_play
			pkt.Read4B(GA_T::TRAINING_MAP_GAME_ID);  // consume but ignore
			uint32_t profileId          = pkt.Read4B(GA_T::PROFILE_ID).value_or(GA_G::PROFILE_ID_ASSAULT);
			uint32_t headAsmId          = pkt.Read4B(GA_T::HEAD_ASM_ID).value_or(1605);
			uint32_t genderTypeId       = pkt.Read4B(GA_T::GENDER_TYPE_VALUE_ID).value_or(853);

			// DWORDS payload: 4B length prefix + raw morph bytes — strip prefix before storing.
			std::vector<uint8_t> morphBlob;
			auto dwordsRaw = pkt.ReadNBytes(GA_T::DWORDS);
			if (dwordsRaw && dwordsRaw->size() > 4)
				morphBlob.assign(dwordsRaw->begin() + 4, dwordsRaw->end());

			selected_character_id_ = PlayerSessionStore::InsertCharacter(user_id_, profileId, headAsmId, genderTypeId, morphBlob);
			selected_profile_id_   = profileId;

			Logger::Log("tcp", "[%s] ADD_PLAYER_CHARACTER: profile=%u head=%u gender=%u morphBytes=%u charId=%lld\n",
				Logger::GetTime(), profileId, headAsmId, genderTypeId, (unsigned)morphBlob.size(), selected_character_id_);

			send_add_player_character_response();
			// Both tutorial and normal paths use wait-for-READY then PLAYER_REGISTER flow.
			wait_for_home_map_then_register(120);  // 120 second timeout
			break;
		}
		case GA_U::DELETE_CHARACTER:
			Logger::Log("tcp", "[%s] Received: DELETE_CHARACTER [0x%04X], item count: %d\n", Logger::GetTime(), packet_type, item_count);
			// todo
			break;
		case GA_U::UPDATE_NEW_MAIL_COUNT:
			Logger::Log("tcp", "[%s] Received: UPDATE_NEW_MAIL_COUNT [0x%04X], item count: %d\n", Logger::GetTime(), packet_type, item_count);
			send_update_new_mail_count_response();
			break;
		case GA_U::GET_TICKET_INFO:
			Logger::Log("tcp", "[%s] Received: GET_TICKET_INFO [0x%04X], item count: %d\n", Logger::GetTime(), packet_type, item_count);
			send_get_ticket_info_response();
			break;
		case GA_U::MATCH_JOIN: {
			Logger::Log("tcp", "[%s] Received: MATCH_JOIN [0x%04X], item count: %d\n", Logger::GetTime(), packet_type, item_count);
			PacketView pkt(data + 6, length - 6);
			uint32_t matchQueueId = pkt.Read4B(GA_T::MATCH_QUEUE_ID).value_or(0);
			uint32_t matchFilters = pkt.Read4B(GA_T::MATCH_FILTERS).value_or(0);

			send_match_join_response(matchQueueId, matchFilters);

			break;
		}
		case GA_U::MATCH_LEAVE: {
			Logger::Log("tcp", "[%s] Received: MATCH_LEAVE [0x%04X], item count: %d\n", Logger::GetTime(), packet_type, item_count);
			// todo: MATCH_TEAM_FLAG is being passed

			send_match_leave_response();

			break;

		}
		case GA_U::MATCH_ACCEPT:
			Logger::Log("tcp", "[%s] Received: MATCH_ACCEPT [0x%04X], item count: %d\n", Logger::GetTime(), packet_type, item_count);
			send_match_accept_response();
			break;
		case GA_U::AGENCY_GET_ROSTER:
			Logger::Log("tcp", "[%s] Received: AGENCY_GET_ROSTER [0x%04X], item count: %d\n", Logger::GetTime(), packet_type, item_count);
			send_agency_get_roster_response();
			break;
		case GA_U::SEND_PLAYER_SKILLS: {
			// CGameClient::SendSkillsToServer @ 0x1141fd00 sends this when the player
			// hits Save on the skill tree. Payload carries a DATA_SET_CHARACTER_SKILLS
			// array; each record is {SKILL_GROUP_ID, SKILL_ID, POINTS_ALLOCATED}.
			// Persist the allocation and echo the saved state back so the UI picks up
			// the authoritative values.
			Logger::Log("tcp", "[%s] Received: SEND_PLAYER_SKILLS [0x%04X], item count: %d\n",
				Logger::GetTime(), packet_type, item_count);

			if (selected_character_id_ == 0) {
				Logger::Log("tcp", "[%s] SEND_PLAYER_SKILLS: no selected character, dropping\n",
					Logger::GetTime());
				break;
			}

			PacketView pkt(data + 6, length - 6);
			std::vector<SkillRow> skills;
			for (const auto& rec : pkt.GetDataSet(GA_T::DATA_SET_CHARACTER_SKILLS)) {
				SkillRow row{};
				row.skill_group_id = (int)rec.Read4B(GA_T::SKILL_GROUP_ID).value_or(0);
				row.skill_id       = (int)rec.Read4B(GA_T::SKILL_ID).value_or(0);
				row.points         = (int)rec.Read4B(GA_T::POINTS_ALLOCATED).value_or(0);
				// The client only emits rows with points > 0, but guard anyway.
				if (row.skill_group_id && row.skill_id && row.points > 0)
					skills.push_back(row);
			}

			Logger::Log("tcp", "[TCP] SEND_PLAYER_SKILLS: charId=%lld parsed %d skill rows\n",
				(long long)selected_character_id_, (int)skills.size());

			PlayerSessionStore::SetSkillsForCharacter(selected_character_id_, skills);

			// Echo back so the UI updates; also refreshes LAST_RESPEC_DATETIME client-side.
			send_player_skills_response();
			break;
		}
		default:
			Logger::Log("tcp", "[%s] Received unknown packet type: %04X, raw data:\n", Logger::GetTime(), packet_type);
			LogData(data, length);
			break;
	}

}

void TcpSession::send_match_join_response(uint32_t matchQueueId, uint32_t matchFilters) {
    current_match_queue_id_ = matchQueueId;

    QueuedPlayer player;
    player.session_guid = session_guid_;
    player.profile_id = selected_profile_id_;
    player.joined_at = std::chrono::steady_clock::now();

    MatchmakingService::AddPlayer(matchQueueId, player);
}

void TcpSession::send_match_leave_response() {
    MatchmakingService::RemovePlayer(session_guid_);
    current_match_queue_id_       = 0;
    // Also clear the invitation-side state in case MATCH_LEAVE arrived
    // between MATCH_INVITATION and MATCH_ACCEPT (player declined the popup
    // by leaving the queue). Otherwise pending_match_instance_id_ would
    // stick around and a later MATCH_ACCEPT could route them to a stale
    // instance.
    pending_match_instance_id_    = 0;
    pending_match_game_mode_.clear();
    pending_match_task_force_     = 1;
}

void TcpSession::send_match_invitation(int64_t instance_id, const std::string& game_mode, int task_force) {
    pending_match_instance_id_ = instance_id;
    pending_match_game_mode_ = game_mode;
    pending_match_task_force_ = task_force;

    std::vector<uint8_t> response;

    uint16_t packet_type = GA_U::MATCH_INVITATION;
    uint16_t item_count = 0;

    append(response, packet_type & 0xFF, packet_type >> 8);
    append(response, item_count & 0xFF, item_count >> 8);

    send_response(response);

    Logger::Log("tcp", "[TcpSession] Sent MATCH_INVITATION to %s (instance %lld)\n",
        player_name.c_str(), (long long)instance_id);
}

void TcpSession::DeliverMatchInvitation(const std::string& session_guid, int64_t instance_id, const std::string& game_mode, int task_force) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    auto it = g_sessions_.find(session_guid);
    if (it == g_sessions_.end()) return;
    auto session = it->second.lock();
    if (!session) return;
    session->send_match_invitation(instance_id, game_mode, task_force);
}

void TcpSession::DeliverMatchCancelled(const std::string& session_guid, const char* reason) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    auto it = g_sessions_.find(session_guid);
    if (it == g_sessions_.end()) return;
    auto session = it->second.lock();
    if (!session) return;
    Logger::Log("tcp", "[TcpSession] Match cancelled for guid=%s (reason: %s) — clearing queue/match state\n",
        session_guid.c_str(), reason ? reason : "unspecified");
    session->current_match_queue_id_       = 0;
    session->pending_match_instance_id_    = 0;
    session->pending_match_game_mode_.clear();
    session->pending_match_task_force_     = 1;
    // No client-side wire response: GET_TICKET_INFO polling already drives
    // the UI off CURRENT_MATCH_QUEUE_ID; clearing it here lets the next
    // refresh show the player as not queued and re-enables the Join button.
}

void TcpSession::send_match_accept_response() {
    if (pending_match_instance_id_ == 0) {
        Logger::Log("tcp", "[TcpSession] MATCH_ACCEPT from %s but no pending instance\n", player_name.c_str());
        return;
    }

    auto instance = InstanceRegistry::GetInstanceById(pending_match_instance_id_);
    if (!instance || instance->state != "READY") {
        Logger::Log("tcp", "[TcpSession] MATCH_ACCEPT: instance %lld not ready\n",
            (long long)pending_match_instance_id_);
        pending_match_instance_id_ = 0;
        return;
    }

    Logger::Log("tcp", "[TcpSession] MATCH_ACCEPT from %s — registering with instance %lld then sending go_play\n",
        player_name.c_str(), (long long)instance->instance_id);

    // Build PLAYER_REGISTER JSON with full character state (same as initiate_player_register_and_go_play).
    nlohmann::json reg;
    reg["type"]         = IpcProtocol::MSG_PLAYER_REGISTER;
    reg["session_guid"] = session_guid_;
    reg["profile_id"]   = selected_profile_id_;
    reg["player_name"]  = player_name;
    reg["user_id"]      = user_id_;
    reg["character_id"] = selected_character_id_;
    reg["task_force"]   = pending_match_task_force_;

    auto charInfo = PlayerSessionStore::GetCharacterById(selected_character_id_);
    if (charInfo) {
        reg["head_asm_id"]          = charInfo->head_asm_id;
        reg["gender_type_value_id"] = charInfo->gender_type_value_id;
        reg["morph_data"]           = HexUtils::hex_encode(charInfo->morph_data);
    } else {
        reg["head_asm_id"]          = 0;
        reg["gender_type_value_id"] = 0;
        reg["morph_data"]           = "";
    }

    // Mirror the skill-tree embedding from initiate_player_register_and_go_play
    // so a match-accept spawn gets the same skill effects as a home-map spawn.
    {
        nlohmann::json skillsArr = nlohmann::json::array();
        for (const auto& s : PlayerSessionStore::GetSkillsForCharacter(selected_character_id_)) {
            nlohmann::json row;
            row["skill_group_id"] = s.skill_group_id;
            row["skill_id"]       = s.skill_id;
            row["points"]         = s.points;
            skillsArr.push_back(row);
        }
        reg["skills"] = skillsArr;
        reg["last_respec_at"] = PlayerSessionStore::GetLastRespecAt(selected_character_id_);
    }

    // Send PLAYER_REGISTER to the MISSION instance (not home map).
    int64_t target_instance_id = pending_match_instance_id_;
    int target_task_force = pending_match_task_force_;
    if (!IpcServer::SendToInstance(target_instance_id, reg.dump())) {
        Logger::Log("tcp", "[TcpSession] No IPC session for instance %lld — cannot register player %s\n",
            (long long)target_instance_id, player_name.c_str());
        pending_match_instance_id_ = 0;
        pending_match_game_mode_.clear();
        return;
    }

    // Arm 60-second ACK-wait timer, then send go_play on success.
    auto self(shared_from_this());
    pending_ack_timer_ = std::make_shared<asio::steady_timer>(io_ctx_);
    pending_ack_timer_->expires_after(std::chrono::seconds(60));

    auto timer = pending_ack_timer_;
    IpcServer::RegisterPendingAck(session_guid_,
        [this, self, timer, target_instance_id, target_task_force](bool success, int pawn_id) {
            timer->cancel();
            if (!success) {
                Logger::Log("tcp", "[TcpSession] PLAYER_REGISTER ACK failed for %s on mission instance\n",
                    session_guid_.c_str());
                pending_match_instance_id_ = 0;
                pending_match_game_mode_.clear();
                return;
            }

            Logger::Log("tcp", "[TcpSession] PLAYER_REGISTER ACK success for %s pawn_id=%d — sending go_play to mission\n",
                session_guid_.c_str(), pawn_id);

            // The player now lives on the mission instance, not the home
            // instance. Keep assigned_instance_id_ in sync so any subsequent
            // control-server-→-DLL messages (e.g. PLAYER_ACTION /changeteam)
            // route to the right DLL.
            assigned_instance_id_ = target_instance_id;

            // Track player in mission instance
            InstanceRegistry::InsertInstancePlayer(
                target_instance_id, session_guid_, selected_character_id_, target_task_force,
                selected_profile_id_);

            auto inst = InstanceRegistry::GetInstanceById(target_instance_id);
            if (!inst || inst->state != "READY") {
                Logger::Log("tcp", "[TcpSession] Mission instance %lld no longer ready\n",
                    (long long)target_instance_id);
                pending_match_instance_id_ = 0;
                pending_match_game_mode_.clear();
                return;
            }

            // Route through the single GSC_GO_PLAY builder so map_game_info
            // resolution applies on this path too. The original duplicate
            // had hardcoded HQ values (22845 / 0x050B / 5716) and was the
            // reason MATCH_ACCEPT-driven launches still saw the fallback.
            send_go_play_to_instance(*inst, target_task_force);

            pending_match_instance_id_ = 0;
            pending_match_game_mode_.clear();
            pending_match_task_force_ = 1;
            current_match_queue_id_ = 0;
        });

    pending_ack_timer_->async_wait([this, self, timer](std::error_code ec) {
        if (!ec) {
            Logger::Log("tcp", "[TcpSession] PLAYER_REGISTER ACK timeout for %s on mission instance\n",
                session_guid_.c_str());
            IpcServer::ClearPendingAck(session_guid_);
            pending_match_instance_id_ = 0;
            pending_match_game_mode_.clear();
        }
    });
}

void TcpSession::send_get_ticket_info_response() {

	const auto queues = MatchmakingService::GetEnabledQueueConfigs();

	std::vector<uint8_t> response;

	const uint16_t packet_type = GA_U::GET_TICKET_INFO;
	const uint16_t item_count  = 3;  // TEAM_LEADER_FLAG + CURRENT_MATCH_QUEUE_ID + DATA_SET

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count  & 0xFF, item_count  >> 8);

	Write1B(response, GA_T::TEAM_LEADER_FLAG, 0x1);
	Write4B(response, GA_T::CURRENT_MATCH_QUEUE_ID, current_match_queue_id_);

	append(response, GA_T::DATA_SET & 0xFF, GA_T::DATA_SET >> 8);
	append(response, (uint8_t)(queues.size() & 0xFF), (uint8_t)(queues.size() >> 8));

	for (const auto& cfg : queues) {
		TicketInfoEncoder::EncodeRecord(response, cfg, RuntimeStats::ForQueue(cfg.queue_id));
	}

	send_response(response);

	// ─── Reference templates for queue rows we haven't added yet. ────────────
	// Kept for documentation while implementing new queue types — copy the
	// magic numbers you need and seed them into ga_queues / ga_queue_map_pool.
	// Compiled out; the active encode path is TicketInfoEncoder above.
#if 0
		// MATCH_QUEUE_ID 5 — Low security (commonwealth missions, tier 1)
		append(response, 0x1c, 0x00);  // inner item count
			Write4B(response, GA_T::QUEUE_TYPE_VALUE_ID, 0x000003fd);
			Write4B(response, GA_T::MATCH_QUEUE_ID, 0x00000005);
			Write4B(response, GA_T::NAME_MSG_ID, 0x0000d8a1); // Low security
			Write4B(response, GA_T::DESC_MSG_ID, 0x0000d8a0);
			Write4B(response, GA_T::ICON_ID, 0x00000219);
			Write4B(response, GA_T::PLAYER_COUNT, 0x00000000);
			Write4B(response, GA_T::MAX_PLAYERS_PER_SIDE, 0x0000000a);
			Write4B(response, GA_T::MIN_PLAYERS_PER_TEAM, 0x00000001);
			Write4B(response, GA_T::MAX_PLAYERS_PER_TEAM, 0x0000000a);
			Write4B(response, GA_T::INSTANCE_COUNT, 0x00000000);
			Write4B(response, GA_T::LEVEL_MIN, 0x00000001);
			Write4B(response, GA_T::LEVEL_MAX, 0x000000c8);
			Write4B(response, GA_T::TAB, 0x000001bb);
			WriteFloat(response, GA_T::MAP_X, 6.0f);
			Write4B(response, GA_T::MAP_Y, 0x00);
			WriteNBytes(response, GA_T::MAP_ACTIVE_FLAG, {0x01, 0x00, 0x79});
			Write4B(response, GA_T::MAP_ICON_TEXTURE_RES_ID, 0x00001406);
			Write4B(response, GA_T::VIDEO_RES_ID, 0x00000000);
			Write4B(response, GA_T::LOCATION_VALUE_ID, 0x000005c5);
			WriteNBytes(response, GA_T::DOUBLE_AGENT_FLAG, {0x01, 0x00, 0x6e});
			Write4B(response, GA_T::SYS_SITE_ID, 0x00000000);
			Write4B(response, GA_T::SORT_ORDER, 0x00000000);
			WriteNBytes(response, GA_T::BONUS_QUEUE_FLAG, {0x00, 0x08, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
			Write4B(response, GA_T::DIFFICULTY_VALUE_ID, 0x00000404);
			WriteNBytes(response, GA_T::ACTIVE_FLAG, {0x01, 0x00, 0x79});
			append(response, GA_T::DATA_SET_PROFILE_COUNTS & 0xFF, GA_T::DATA_SET_PROFILE_COUNTS >> 8);
			append(response, 0x00, 0x00);        // count elements
			Write1B(response, GA_T::LOCKED_FLAG, 0x00);

		// MATCH_QUEUE_ID 6 — Medium security
		append(response, 0x1c, 0x00);  // inner item count
			Write4B(response, GA_T::QUEUE_TYPE_VALUE_ID, 0x000003fd);
			Write4B(response, GA_T::MATCH_QUEUE_ID, 0x00000006);
			Write4B(response, GA_T::NAME_MSG_ID, 0x0000a1f7); // Medium security
			Write4B(response, GA_T::DESC_MSG_ID, 0x0000a1f6);
			Write4B(response, GA_T::ICON_ID, 0x00000219);
			Write4B(response, GA_T::PLAYER_COUNT, 0x00000000);
			Write4B(response, GA_T::MAX_PLAYERS_PER_SIDE, 0x0000000a);
			Write4B(response, GA_T::MIN_PLAYERS_PER_TEAM, 0x00000001);
			Write4B(response, GA_T::MAX_PLAYERS_PER_TEAM, 0x0000000a);
			Write4B(response, GA_T::INSTANCE_COUNT, 0x00000000);
			Write4B(response, GA_T::LEVEL_MIN, 0x00000001);
			Write4B(response, GA_T::LEVEL_MAX, 0x000000c8);
			Write4B(response, GA_T::TAB, 0x000001bb);
			WriteFloat(response, GA_T::MAP_X, 6.0f);
			Write4B(response, GA_T::MAP_Y, 0x00);
			WriteNBytes(response, GA_T::MAP_ACTIVE_FLAG, {0x01, 0x00, 0x79});
			Write4B(response, GA_T::MAP_ICON_TEXTURE_RES_ID, 0x00001406);
			Write4B(response, GA_T::VIDEO_RES_ID, 0x00000000);
			Write4B(response, GA_T::LOCATION_VALUE_ID, 0x000005c5);
			WriteNBytes(response, GA_T::DOUBLE_AGENT_FLAG, {0x01, 0x00, 0x6e});
			Write4B(response, GA_T::SYS_SITE_ID, 0x00000000);
			Write4B(response, GA_T::SORT_ORDER, 0x00000000);
			WriteNBytes(response, GA_T::BONUS_QUEUE_FLAG, {0x00, 0x08, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
			Write4B(response, GA_T::DIFFICULTY_VALUE_ID, 0x00000405);
			WriteNBytes(response, GA_T::ACTIVE_FLAG, {0x01, 0x00, 0x79});
			append(response, GA_T::DATA_SET_PROFILE_COUNTS & 0xFF, GA_T::DATA_SET_PROFILE_COUNTS >> 8);
			append(response, 0x00, 0x00);        // count elements
			Write1B(response, GA_T::LOCKED_FLAG, 0x00);

		// MATCH_QUEUE_ID 7 — High security
		append(response, 0x1c, 0x00);  // inner item count
			Write4B(response, GA_T::QUEUE_TYPE_VALUE_ID, 0x000003fd);
			Write4B(response, GA_T::MATCH_QUEUE_ID, 0x00000007);
			Write4B(response, GA_T::NAME_MSG_ID, 0x0000a1f9); // High security
			Write4B(response, GA_T::DESC_MSG_ID, 0x0000a1f8);
			Write4B(response, GA_T::ICON_ID, 0x00000219);
			Write4B(response, GA_T::PLAYER_COUNT, 0x00000000);
			Write4B(response, GA_T::MAX_PLAYERS_PER_SIDE, 0x0000000a);
			Write4B(response, GA_T::MIN_PLAYERS_PER_TEAM, 0x00000001);
			Write4B(response, GA_T::MAX_PLAYERS_PER_TEAM, 0x0000000a);
			Write4B(response, GA_T::INSTANCE_COUNT, 0x00000000);
			Write4B(response, GA_T::LEVEL_MIN, 0x00000001);
			Write4B(response, GA_T::LEVEL_MAX, 0x000000c8);
			Write4B(response, GA_T::TAB, 0x000001bb);
			WriteFloat(response, GA_T::MAP_X, 6.0f);
			Write4B(response, GA_T::MAP_Y, 0x00);
			WriteNBytes(response, GA_T::MAP_ACTIVE_FLAG, {0x01, 0x00, 0x79});
			Write4B(response, GA_T::MAP_ICON_TEXTURE_RES_ID, 0x00001406);
			Write4B(response, GA_T::VIDEO_RES_ID, 0x00000000);
			Write4B(response, GA_T::LOCATION_VALUE_ID, 0x000005c5);
			WriteNBytes(response, GA_T::DOUBLE_AGENT_FLAG, {0x01, 0x00, 0x6e});
			Write4B(response, GA_T::SYS_SITE_ID, 0x00000000);
			Write4B(response, GA_T::SORT_ORDER, 0x00000000);
			WriteNBytes(response, GA_T::BONUS_QUEUE_FLAG, {0x00, 0x08, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
			Write4B(response, GA_T::DIFFICULTY_VALUE_ID, 0x00000406);
			WriteNBytes(response, GA_T::ACTIVE_FLAG, {0x01, 0x00, 0x79});
			append(response, GA_T::DATA_SET_PROFILE_COUNTS & 0xFF, GA_T::DATA_SET_PROFILE_COUNTS >> 8);
			append(response, 0x00, 0x00);        // count elements
			Write1B(response, GA_T::LOCKED_FLAG, 0x00);

		// MATCH_QUEUE_ID 8 — Maximum security (header only; rest of fields follow the same pattern as 5/6/7)
		// Write4B(response, GA_T::MATCH_QUEUE_ID, 0x00000008);
		// Write4B(response, GA_T::NAME_MSG_ID,    0x0000a1fa); // Maximum security

		// MATCH_QUEUE_ID 1 — Ultra max (Commonwealth Prime) — ALREADY SEEDED in ga_queues
		// MATCH_QUEUE_ID 2 — Mercenary / PvP                — ALREADY SEEDED in ga_queues

		// MATCH_QUEUE_ID 10 — Ultra max security (Sonoran Desert)
		append(response, 0x1c, 0x00);  // inner item count
			Write4B(response, GA_T::QUEUE_TYPE_VALUE_ID, 0x000003fd);
			Write4B(response, GA_T::MATCH_QUEUE_ID, 0x0000000a);
			Write4B(response, GA_T::NAME_MSG_ID, 0x0000d8a9);
			Write4B(response, GA_T::DESC_MSG_ID, 0x0000d8a8);
			Write4B(response, GA_T::ICON_ID, 0x00000219);
			Write4B(response, GA_T::PLAYER_COUNT, 0x00000000);
			Write4B(response, GA_T::MAX_PLAYERS_PER_SIDE, 0x0000000a);
			Write4B(response, GA_T::MIN_PLAYERS_PER_TEAM, 0x00000001);
			Write4B(response, GA_T::MAX_PLAYERS_PER_TEAM, 0x0000000a);
			Write4B(response, GA_T::INSTANCE_COUNT, 0x00000000);
			Write4B(response, GA_T::LEVEL_MIN, 0x00000005);
			Write4B(response, GA_T::LEVEL_MAX, 0x000000c8);
			Write4B(response, GA_T::TAB, 0x000001bb);
			WriteFloat(response, GA_T::MAP_X, 6.0f);
			Write4B(response, GA_T::MAP_Y, 0x00);
			WriteNBytes(response, GA_T::MAP_ACTIVE_FLAG, {0x01, 0x00, 0x79});
			Write4B(response, GA_T::MAP_ICON_TEXTURE_RES_ID, 0x00001406);
			Write4B(response, GA_T::VIDEO_RES_ID, 0x00000000);
			Write4B(response, GA_T::LOCATION_VALUE_ID, 0x000005cb);
			WriteNBytes(response, GA_T::DOUBLE_AGENT_FLAG, {0x01, 0x00, 0x6e});
			Write4B(response, GA_T::SYS_SITE_ID, 0x00000000);
			Write4B(response, GA_T::SORT_ORDER, 0x00000000);
			WriteNBytes(response, GA_T::BONUS_QUEUE_FLAG, {0x00, 0x08, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
			Write4B(response, GA_T::DIFFICULTY_VALUE_ID, 0x000005bf);
			WriteNBytes(response, GA_T::ACTIVE_FLAG, {0x01, 0x00, 0x79});
			append(response, GA_T::DATA_SET_PROFILE_COUNTS & 0xFF, GA_T::DATA_SET_PROFILE_COUNTS >> 8);
			append(response, 0x00, 0x00);        // count elements
			Write1B(response, GA_T::LOCKED_FLAG, 0x00);

		// MATCH_QUEUE_ID 11 — Ultra max security (Mining Province)
		append(response, 0x1c, 0x00);  // inner item count
			Write4B(response, GA_T::QUEUE_TYPE_VALUE_ID, 0x000003fd);
			Write4B(response, GA_T::MATCH_QUEUE_ID, 0x0000000b);
			Write4B(response, GA_T::NAME_MSG_ID, 0x0000d8a9);
			Write4B(response, GA_T::DESC_MSG_ID, 0x0000d8a8);
			Write4B(response, GA_T::ICON_ID, 0x00000219);
			Write4B(response, GA_T::PLAYER_COUNT, 0x00000000);
			Write4B(response, GA_T::MAX_PLAYERS_PER_SIDE, 0x0000000a);
			Write4B(response, GA_T::MIN_PLAYERS_PER_TEAM, 0x00000001);
			Write4B(response, GA_T::MAX_PLAYERS_PER_TEAM, 0x0000000a);
			Write4B(response, GA_T::INSTANCE_COUNT, 0x00000000);
			Write4B(response, GA_T::LEVEL_MIN, 0x00000005);
			Write4B(response, GA_T::LEVEL_MAX, 0x000000c8);
			Write4B(response, GA_T::TAB, 0x000001bb);
			WriteFloat(response, GA_T::MAP_X, 6.0f);
			Write4B(response, GA_T::MAP_Y, 0x00);
			WriteNBytes(response, GA_T::MAP_ACTIVE_FLAG, {0x01, 0x00, 0x79});
			Write4B(response, GA_T::MAP_ICON_TEXTURE_RES_ID, 0x00001406);
			Write4B(response, GA_T::VIDEO_RES_ID, 0x00000000);
			Write4B(response, GA_T::LOCATION_VALUE_ID, 0x000005c6);
			WriteNBytes(response, GA_T::DOUBLE_AGENT_FLAG, {0x01, 0x00, 0x6e});
			Write4B(response, GA_T::SYS_SITE_ID, 0x00000000);
			Write4B(response, GA_T::SORT_ORDER, 0x00000000);
			WriteNBytes(response, GA_T::BONUS_QUEUE_FLAG, {0x00, 0x08, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
			Write4B(response, GA_T::DIFFICULTY_VALUE_ID, 0x000005bf);
			WriteNBytes(response, GA_T::ACTIVE_FLAG, {0x01, 0x00, 0x79});
			append(response, GA_T::DATA_SET_PROFILE_COUNTS & 0xFF, GA_T::DATA_SET_PROFILE_COUNTS >> 8);
			append(response, 0x00, 0x00);        // count elements
			Write1B(response, GA_T::LOCKED_FLAG, 0x00);

		// MATCH_QUEUE_ID 3 — Double Agent (timed/rotating queue, uses REMAINING_SECONDS)
		append(response, 0x1c, 0x00);  // inner item count
			Write4B(response, GA_T::QUEUE_TYPE_VALUE_ID, 0x000003fd);
			Write4B(response, GA_T::MATCH_QUEUE_ID, 0x00000003);
			Write4B(response, GA_T::NAME_MSG_ID, 0x0000a1fc); // Double Agent
			Write4B(response, GA_T::DESC_MSG_ID, 0x0000a1fb);
			Write4B(response, GA_T::ICON_ID, 0x000001c9);
			Write4B(response, GA_T::PLAYER_COUNT, 0x00000000);
			Write4B(response, GA_T::MAX_PLAYERS_PER_SIDE, 0x00000004);
			Write4B(response, GA_T::MIN_PLAYERS_PER_TEAM, 0x00000001);
			Write4B(response, GA_T::MAX_PLAYERS_PER_TEAM, 0x00000004);
			Write4B(response, GA_T::INSTANCE_COUNT, 0x00000000);
			Write4B(response, GA_T::LEVEL_MIN, 0x00000005);
			Write4B(response, GA_T::LEVEL_MAX, 0x000000c8);
			Write4B(response, GA_T::TAB, 0x000000e8);
			WriteFloat(response, GA_T::MAP_X, 10.0f);
			WriteFloat(response, GA_T::MAP_Y, 0.5f);
			WriteNBytes(response, GA_T::MAP_ACTIVE_FLAG, {0x01, 0x00, 0x79});
			Write4B(response, GA_T::MAP_ICON_TEXTURE_RES_ID, 0x00001406);
			Write4B(response, GA_T::VIDEO_RES_ID, 0x00000000);
			Write4B(response, GA_T::LOCATION_VALUE_ID, 0x000005c5);
			WriteNBytes(response, GA_T::DOUBLE_AGENT_FLAG, {0x01, 0x00, 0x79});
			Write4B(response, GA_T::SYS_SITE_ID, 0x00000000);
			Write4B(response, GA_T::SORT_ORDER, 0x00000004);
			WriteNBytes(response, GA_T::BONUS_QUEUE_FLAG, {0x00, 0x08, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
			Write4B(response, GA_T::DIFFICULTY_VALUE_ID, 0x000004ec);
			WriteNBytes(response, GA_T::ACTIVE_FLAG, {0x01, 0x00, 0x79});
			// Write4B(response, GA_T::REMAINING_SECONDS, 0x00000001);
			append(response, GA_T::DATA_SET_PROFILE_COUNTS & 0xFF, GA_T::DATA_SET_PROFILE_COUNTS >> 8);
			append(response, 0x00, 0x00);        // count elements
			Write1B(response, GA_T::LOCKED_FLAG, 0x00);

		// MATCH_QUEUE_ID 4 — Sonoran Raid (large-side raid queue)
		append(response, 0x1c, 0x00);  // inner item count
			Write4B(response, GA_T::QUEUE_TYPE_VALUE_ID, 0x000005ae);
			Write4B(response, GA_T::MATCH_QUEUE_ID, 0x00000004);
			Write4B(response, GA_T::NAME_MSG_ID, 0x00008fdc); // Sonoran Raid
			Write4B(response, GA_T::DESC_MSG_ID, 0x0000e7b0);
			Write4B(response, GA_T::ICON_ID, 0x000006b4);
			Write4B(response, GA_T::PLAYER_COUNT, 0x00000000);
			Write4B(response, GA_T::MAX_PLAYERS_PER_SIDE, 0x00000015);
			Write4B(response, GA_T::MIN_PLAYERS_PER_TEAM, 0x00000000);
			Write4B(response, GA_T::MAX_PLAYERS_PER_TEAM, 0x00000015);
			Write4B(response, GA_T::INSTANCE_COUNT, 0x00000000);
			Write4B(response, GA_T::LEVEL_MIN, 0x00000005);
			Write4B(response, GA_T::LEVEL_MAX, 0x0000003c);
			Write4B(response, GA_T::TAB, 0x000000e7);
			Write4B(response, GA_T::MAP_X, 0x00);
			Write4B(response, GA_T::MAP_Y, 0x00);
			WriteNBytes(response, GA_T::MAP_ACTIVE_FLAG, {0x01, 0x00, 0x79});
			Write4B(response, GA_T::MAP_ICON_TEXTURE_RES_ID, 0x00001406);
			Write4B(response, GA_T::VIDEO_RES_ID, 0x00000000);
			Write4B(response, GA_T::LOCATION_VALUE_ID, 0x00000000);
			WriteNBytes(response, GA_T::DOUBLE_AGENT_FLAG, {0x01, 0x00, 0x6e});
			Write4B(response, GA_T::SYS_SITE_ID, 0x00000000);
			Write4B(response, GA_T::SORT_ORDER, 0x00000000);
			WriteNBytes(response, GA_T::BONUS_QUEUE_FLAG, {0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
			Write4B(response, GA_T::DIFFICULTY_VALUE_ID, 0x000004eb);
			WriteNBytes(response, GA_T::ACTIVE_FLAG, {0x01, 0x00, 0x79});
			// Write4B(response, GA_T::REMAINING_SECONDS, 0x00000001);
			append(response, GA_T::DATA_SET_PROFILE_COUNTS & 0xFF, GA_T::DATA_SET_PROFILE_COUNTS >> 8);
			append(response, 0x00, 0x00);        // count elements
			Write1B(response, GA_T::LOCKED_FLAG, 0x00);
#endif
}

void TcpSession::send_character_inventory_response(int nPawnId) {
	// Stub — no active callers. Reimplement from ga_character_devices if needed.
	Logger::Log("tcp", "[TCP] send_character_inventory_response: STUB (no-op) pawnId=%d\n", nPawnId);
}

void TcpSession::send_inventory_response(int nPawnId, int64_t character_id) {
	auto devices = PlayerSessionStore::GetDevicesForCharacter(character_id);

	// Ships TWO blocks of records over SEND_INVENTORY (0x0182):
	//   1. Currently-equipped devices for `character_id`, with
	//      LOCATION_VALUE_ID=369 (EQUIPPED), DEVICE_ID=<inv_id> (= the
	//      backing pawn device's instance id), EQUIPPED_SLOT_VALUE_ID=<svid>,
	//      ACTIVE_FLAG=T.
	//   2. The rest of the account-scoped inventory pool visible to this
	//      character's class profile, with LOCATION_VALUE_ID=370 (ON_HAND),
	//      DEVICE_ID=0, EQUIPPED_SLOT_VALUE_ID=0, ACTIVE_FLAG=F.
	//
	// The DEVICE_ID field is the gate that decides whether the client tries
	// to bind this inventory record to a live actor on the pawn. The receiver
	// path (CGameClient::SendInventory @ 0x109ddb40) at the bottom of its
	// per-record loop scans the inventory manager's item array and runs
	//
	//     if (FUN_10a12f80(item) > 0 && FUN_10a14a50(item) == 0)
	//         debugf("Resubmitting an inventory item that could not find its device");
	//
	// FUN_10a12f80 is literally `return *(item + 0xb4);` and the value at
	// +0xb4 is written by FUN_10a12f90 from the marshal DEVICE_ID (0x0206).
	// FUN_10a14a50 iterates all actors and matches *(actor + 0x220) ==
	// *(item + 0xb4). For bag items there's no backing actor, so we send
	// DEVICE_ID=0 → +0xb4=0 → the gate skips the item entirely.
	//
	// LOCATION_VALUE_ID (marshal 0x0310) defaults to 0x172 (= 370 = ON_HAND)
	// on the client side when omitted or zero — see FUN_10a12340. The 369
	// constant in the equipped block above is the semantic mark; the actual
	// don't-resubmit knob is DEVICE_ID.
	//
	// Earlier attempt that triggered the every-frame resubmit loop sent the
	// bag pool with LOCATION_VALUE_ID=369 and DEVICE_ID=<inv_id>, which made
	// the client believe each bag item was currently equipped — the recovery
	// scan then resubmitted them all back to the server forever.

	const auto charInfo = PlayerSessionStore::GetCharacterById(character_id);
	if (!charInfo) {
		Logger::Log("tcp",
			"[TCP] send_inventory_response: GetCharacterById(%lld) returned null — skipping\n",
			character_id);
		return;
	}
	const int64_t user_id    = charInfo->user_id;
	const uint32_t profile   = charInfo->profile_id;

	if (devices.empty()) {
		Logger::Log("tcp", "[TCP] send_inventory_response: no equipped devices for charId=%lld pawnId=%d (will still ship bag pool)\n",
			character_id, nPawnId);
	}

	Logger::Log("tcp",
		"[TCP] send_inventory_response: charId=%lld pawnId=%d equipped=%d (one packet per item)\n",
		character_id, nPawnId, (int)devices.size());

	// One SEND_INVENTORY message per device. Bundling everything into a single
	// DATA_SET worked for small loadouts but exceeded the client's per-packet
	// receive cap (~1450 bytes) on heavily-modded loadouts — items past the cap
	// silently dropped. The parser accumulates records into m_InventoryMap
	// across messages, so per-device messages are equivalent semantically and
	// each is comfortably small (~150-200 bytes worst case with 6 mods).
	for (const auto& d : devices) {
		std::vector<uint8_t> response;

		append(response, GA_U::SEND_INVENTORY & 0xFF, GA_U::SEND_INVENTORY >> 8);
		append(response, 0x02, 0x00);   // 2 top-level fields: PAWN_ID + DATA_SET

		Write4B(response, GA_T::PAWN_ID, nPawnId);

		append(response, GA_T::DATA_SET & 0xFF, GA_T::DATA_SET >> 8);
		append(response, 0x01, 0x00);   // 1 item in DATA_SET

		append(response, 0x0E, 0x00);   // 14 fields in this record

		// nBlueprintId gates m_pAmBlueprint on the client (FUN_10a12740 sets it
		// from this field). The mod-letter [...] suffix is rendered only when
		// m_pAmBlueprint is non-null, so for any item carrying rolled mods we
		// must send a real blueprint_id whose created_item_id matches this
		// device. Items with no mods send 0 (no fake blueprint, no suffix).
		// When d.oc is set, picks a blueprint with override_name_msg_id != 0
		// so the client renders the "OC" name suffix.
		//
		// v76: cosmetic rows (item_id > 0, device_id = 0) marshal as their
		// item_id and ship DEVICE_ID=0 so the client's FUN_10a12f80 recovery
		// scan doesn't try to bind them to a live actor (cosmetics have no
		// ATgDevice). BLUEPRINT_ID stays 0 for cosmetics (no rolled mods).
		//
		// Armor rows have the same shape as cosmetics (item_id > 0,
		// device_id = 0, no backing actor) but DO carry mods — so they need
		// a non-zero BLUEPRINT_ID to make the client's FUN_10a13820
		// mod-application path apply the rolled effects. Discriminator: armor
		// has mods, cosmetics don't. Universal blueprint works because the
		// client only nullity-checks the resolved blueprint pointer — the
		// rendered `[…]` letters come from each effect's `prop.ui_code`, not
		// from blueprint metadata.
		const bool itemRow       = d.item_id > 0;
		const bool isArmor       = itemRow && !d.mods.empty();
		const int marshalItemId  = itemRow ? d.item_id : d.device_id;
		const int marshalDeviceField = itemRow ? 0 : d.inventory_id;
		const int blueprintId    =
			isArmor                    ? ArmorLoadouts::kUniversalArmorBlueprintId :
			(itemRow || d.mods.empty()) ? 0
			                            : PlayerSessionStore::GetBlueprintIdForDevice(d.device_id, d.oc);

		Write4B(response, GA_T::INV_REPLICATION_STATE, 0x1);
		Write4B(response, GA_T::ITEM_ID, marshalItemId);
		Write4B(response, GA_T::INVENTORY_ID, d.inventory_id);
		Write4B(response, GA_T::BLUEPRINT_ID, blueprintId);
		Write4B(response, GA_T::CRAFTED_QUALITY_VALUE_ID, d.quality);
		Write4B(response, GA_T::DURABILITY, 100);
		WriteDouble(response, GA_T::ACQUIRE_DATETIME, 1700000000.0);
		WriteString(response, GA_T::BOUND_FLAG, "T");
		Write4B(response, GA_T::LOCATION_VALUE_ID, 369);
		Write4B(response, GA_T::INSTANCE_COUNT, 0x1);
		WriteString(response, GA_T::ACTIVE_FLAG, "T");
		Write4B(response, GA_T::DEVICE_ID, marshalDeviceField);

		// DATA_SET_INVENTORY_STATE carries a list of {INVENTORY_ID, EFFECT_GROUP_ID}
		// pairs. Client FUN_10a13820 @ 0x10a13820 appends every row whose
		// INVENTORY_ID matches this item's inv_id into the inventory object's
		// effect-group list (m_nStateEffectGroupIdArray), which drives the
		// [...] letter suffix in the UI (one letter per row, derived from each
		// effect's property.ui_code).
		//
		// Send ONLY rolled mods here. The device's built-in equip effects
		// (asm_data_set_device_effect_groups) and built-in fire-mode effects
		// (asm_data_set_device_mode_effect_groups) are NOT replicated through
		// this pipe — the client already has them from its own asm.dat (via
		// Device.m_EquipEffect + m_pFireModeSetup native data). Mixing them in
		// here leaks built-in props into the suffix (e.g. a built-in +25% Mech
		// Damage equip-effect with ui_code='m' would render as a stray 'm'
		// alongside the rolled-mod letters).
		//
		// Server-side: built-in equip effects are applied by
		// Inventory::ApplyDeviceEquipEffects (direct s_Properties writes), and
		// TgDeviceFire::GetEffectGroup reads m_pFireModeSetup+0x98 directly —
		// neither path consults m_nStateEffectGroupIdArray, so dropping built-in
		// effects from this blob has no gameplay side-effects.
		std::vector<int> effectGroups = d.mods;
		if (effectGroups.empty()) effectGroups.push_back(0); // preserve 1-row shape

		append(response, GA_T::DATA_SET_INVENTORY_STATE & 0xFF, GA_T::DATA_SET_INVENTORY_STATE >> 8);
		append(response, (uint8_t)(effectGroups.size() & 0xFF), (uint8_t)(effectGroups.size() >> 8));
		for (int egid : effectGroups) {
			append(response, 0x02, 0x00);
			Write4B(response, GA_T::INVENTORY_ID, d.inventory_id);
			Write4B(response, GA_T::EFFECT_GROUP_ID, egid);
		}

		// Ship one DATA_SET_CHARACTER_PROFILES row per build profile (1..5)
		// with the same EQUIPPED_SLOT_VALUE_ID. Each row populates one of the
		// five `invObj+0x68..+0x78` slot-table entries on the client. The
		// equip-screen UI reads this table directly (not through the
		// `r_nItemProfileId`-gated path), and unless every profile entry is
		// non-zero it treats the item as unequipped — slots render blank.
		// 5 rows × 4 fields × 4 bytes ≈ 80 extra bytes per item, well under
		// the per-message cap. See inventory-tcp-protocol.md "Concrete fixes
		// for the inconsistent loadout bug" #2.
		append(response, GA_T::DATA_SET_CHARACTER_PROFILES & 0xFF, GA_T::DATA_SET_CHARACTER_PROFILES >> 8);
		append(response, 0x05, 0x00);
		for (int profileId = 1; profileId <= 5; ++profileId) {
			append(response, 0x04, 0x00);
			Write4B(response, GA_T::CHARACTER_ID, 0);
			Write4B(response, GA_T::INVENTORY_ID, d.inventory_id);
			Write4B(response, GA_T::PROFILE_ID, (uint32_t)profileId);
			Write4B(response, GA_T::EQUIPPED_SLOT_VALUE_ID, d.slot_value_id);
		}

		send_response(response);
	}

	// --- Bag pool: account-scoped inventory rows not currently equipped on
	// this character. Wire format mirrors the equipped block above except
	// LOCATION_VALUE_ID=370, DEVICE_ID=0, EQUIPPED_SLOT_VALUE_ID=0, and
	// ACTIVE_FLAG=F. See the top-of-function comment for the gate analysis.
	std::set<int> equipped_inv_ids;
	for (const auto& d : devices) equipped_inv_ids.insert(d.inventory_id);

	const auto bag = PlayerSessionStore::GetInventoryForUser(user_id, profile);
	int bag_shipped = 0;
	for (const auto& row : bag) {
		if (equipped_inv_ids.count(row.inventory_id)) continue;

		std::vector<uint8_t> response;

		append(response, GA_U::SEND_INVENTORY & 0xFF, GA_U::SEND_INVENTORY >> 8);
		append(response, 0x02, 0x00);   // PAWN_ID + DATA_SET

		Write4B(response, GA_T::PAWN_ID, nPawnId);

		append(response, GA_T::DATA_SET & 0xFF, GA_T::DATA_SET >> 8);
		append(response, 0x01, 0x00);

		append(response, 0x0E, 0x00);   // 14 fields per record (same as equipped)

		// v76: cosmetic dispatch + armor extension (see equipped-loop comment).
		const bool   rowIsItem     = row.item_id > 0;
		const bool   rowIsArmor    = rowIsItem && !row.mods.empty();
		const int    marshalItemId = rowIsItem ? row.item_id : row.device_id;
		const int    blueprintId   =
			rowIsArmor                    ? ArmorLoadouts::kUniversalArmorBlueprintId :
			(rowIsItem || row.mods.empty()) ? 0
			                                : PlayerSessionStore::GetBlueprintIdForDevice(row.device_id, row.oc);

		Write4B(response, GA_T::INV_REPLICATION_STATE, 0x1);
		Write4B(response, GA_T::ITEM_ID, marshalItemId);
		Write4B(response, GA_T::INVENTORY_ID, row.inventory_id);
		Write4B(response, GA_T::BLUEPRINT_ID, blueprintId);
		Write4B(response, GA_T::CRAFTED_QUALITY_VALUE_ID, row.quality);
		Write4B(response, GA_T::DURABILITY, 100);
		WriteDouble(response, GA_T::ACQUIRE_DATETIME, 1700000000.0);
		WriteString(response, GA_T::BOUND_FLAG, "T");
		Write4B(response, GA_T::LOCATION_VALUE_ID, 370);   // ON_HAND
		Write4B(response, GA_T::INSTANCE_COUNT, 0x1);
		WriteString(response, GA_T::ACTIVE_FLAG, "F");     // not actively in use
		Write4B(response, GA_T::DEVICE_ID, 0);             // no backing pawn device → recovery scan skips

		std::vector<int> effectGroups = row.mods;
		if (effectGroups.empty()) effectGroups.push_back(0);

		append(response, GA_T::DATA_SET_INVENTORY_STATE & 0xFF, GA_T::DATA_SET_INVENTORY_STATE >> 8);
		append(response, (uint8_t)(effectGroups.size() & 0xFF), (uint8_t)(effectGroups.size() >> 8));
		for (int egid : effectGroups) {
			append(response, 0x02, 0x00);
			Write4B(response, GA_T::INVENTORY_ID, row.inventory_id);
			Write4B(response, GA_T::EFFECT_GROUP_ID, egid);
		}

		append(response, GA_T::DATA_SET_CHARACTER_PROFILES & 0xFF, GA_T::DATA_SET_CHARACTER_PROFILES >> 8);
		append(response, 0x01, 0x00);
			append(response, 0x04, 0x00);
			Write4B(response, GA_T::CHARACTER_ID, 0);
			Write4B(response, GA_T::INVENTORY_ID, row.inventory_id);
			Write4B(response, GA_T::PROFILE_ID, 0x1);
			Write4B(response, GA_T::EQUIPPED_SLOT_VALUE_ID, 0);

		send_response(response);
		++bag_shipped;
	}

	Logger::Log("tcp",
		"[TCP] send_inventory_response: charId=%lld pawnId=%d bag=%d (LOCATION=370, DEVICE_ID=0)\n",
		character_id, nPawnId, bag_shipped);
}

void TcpSession::send_loadout_inventory_response(int nPawnId, const std::vector<LoadoutItem>& items) {
    if (items.empty()) {
        Logger::Log("tcp", "[TCP] send_loadout_inventory_response: empty loadout for pawnId=%d\n", nPawnId);
        return;
    }
    Logger::Log("tcp",
        "[TCP] send_loadout_inventory_response: %d items for pawnId=%d (one packet per item)\n",
        (int)items.size(), nPawnId);

    // Same per-record wire format as send_inventory_response, minus the DB
    // lookup. BLUEPRINT_ID is 0 (no rolled mods on bot devices, so no [...]
    // suffix needed). DATA_SET_INVENTORY_STATE carries a single placeholder
    // row with EFFECT_GROUP_ID=0 to preserve the 1-row shape — harmless when
    // BLUEPRINT_ID=0 (parser skips mod application).
    for (const auto& d : items) {
        std::vector<uint8_t> response;

        append(response, GA_U::SEND_INVENTORY & 0xFF, GA_U::SEND_INVENTORY >> 8);
        append(response, 0x02, 0x00);   // 2 top-level fields: PAWN_ID + DATA_SET

        Write4B(response, GA_T::PAWN_ID, nPawnId);

        append(response, GA_T::DATA_SET & 0xFF, GA_T::DATA_SET >> 8);
        append(response, 0x01, 0x00);   // 1 item per DATA_SET

        append(response, 0x0E, 0x00);   // 14 fields per record

        Write4B(response, GA_T::INV_REPLICATION_STATE, 0x1);
        Write4B(response, GA_T::ITEM_ID, d.device_id);
        Write4B(response, GA_T::INVENTORY_ID, d.inventory_id);
        Write4B(response, GA_T::BLUEPRINT_ID, 0);
        Write4B(response, GA_T::CRAFTED_QUALITY_VALUE_ID, d.quality);
        Write4B(response, GA_T::DURABILITY, 100);
        WriteDouble(response, GA_T::ACQUIRE_DATETIME, 1700000000.0);
        WriteString(response, GA_T::BOUND_FLAG, "T");
        Write4B(response, GA_T::LOCATION_VALUE_ID, 369);
        Write4B(response, GA_T::INSTANCE_COUNT, 0x1);
        WriteString(response, GA_T::ACTIVE_FLAG, "T");
        Write4B(response, GA_T::DEVICE_ID, d.inventory_id);

        append(response, GA_T::DATA_SET_INVENTORY_STATE & 0xFF, GA_T::DATA_SET_INVENTORY_STATE >> 8);
        append(response, 0x01, 0x00);
        append(response, 0x02, 0x00);
        Write4B(response, GA_T::INVENTORY_ID, d.inventory_id);
        Write4B(response, GA_T::EFFECT_GROUP_ID, 0);

        append(response, GA_T::DATA_SET_CHARACTER_PROFILES & 0xFF, GA_T::DATA_SET_CHARACTER_PROFILES >> 8);
        append(response, 0x01, 0x00);
        append(response, 0x04, 0x00);
        Write4B(response, GA_T::CHARACTER_ID, 0);
        Write4B(response, GA_T::INVENTORY_ID, d.inventory_id);
        Write4B(response, GA_T::PROFILE_ID, 0x1);
        Write4B(response, GA_T::EQUIPPED_SLOT_VALUE_ID, d.slot_value_id);

        send_response(response);
    }
}

void TcpSession::send_beacon_pickup_response(int nPawnId, int nDeviceId, int nInventoryId, int nEquipSlotValueId) {
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::SEND_INVENTORY;
	uint16_t item_count = 2;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	Write4B(response, GA_T::PAWN_ID, nPawnId);

	append(response, GA_T::DATA_SET & 0xFF, GA_T::DATA_SET >> 8);
	append(response, 0x01, 0x00);  // 1 item in DATA_SET

		append(response, 0x0E, 0x00);  // 14 fields per item

		Write4B(response, GA_T::INV_REPLICATION_STATE, 0x1);
		Write4B(response, GA_T::ITEM_ID, nDeviceId);
		Write4B(response, GA_T::INVENTORY_ID, nInventoryId);
		Write4B(response, GA_T::BLUEPRINT_ID, 0);
		Write4B(response, GA_T::CRAFTED_QUALITY_VALUE_ID, 1162);
		Write4B(response, GA_T::DURABILITY, 100);
		WriteDouble(response, GA_T::ACQUIRE_DATETIME, 1700000000.0);
		WriteString(response, GA_T::BOUND_FLAG, "T");
		Write4B(response, GA_T::LOCATION_VALUE_ID, 369);
		Write4B(response, GA_T::INSTANCE_COUNT, 0x1);
		WriteString(response, GA_T::ACTIVE_FLAG, "T");
		Write4B(response, GA_T::DEVICE_ID, nInventoryId);

		append(response, GA_T::DATA_SET_INVENTORY_STATE & 0xFF, GA_T::DATA_SET_INVENTORY_STATE >> 8);
		append(response, 0x01, 0x00);
			append(response, 0x02, 0x00);
			Write4B(response, GA_T::INVENTORY_ID, nInventoryId);
			Write4B(response, GA_T::EFFECT_GROUP_ID, 0);

		append(response, GA_T::DATA_SET_CHARACTER_PROFILES & 0xFF, GA_T::DATA_SET_CHARACTER_PROFILES >> 8);
		append(response, 0x01, 0x00);
			append(response, 0x04, 0x00);
			Write4B(response, GA_T::CHARACTER_ID, 0);
			Write4B(response, GA_T::INVENTORY_ID, nInventoryId);
			Write4B(response, GA_T::PROFILE_ID, 0x1);
			Write4B(response, GA_T::EQUIPPED_SLOT_VALUE_ID, nEquipSlotValueId);

	send_response(response);
}

void TcpSession::send_beacon_remove_response(int nPawnId, int nInventoryId) {
	std::vector<uint8_t> response;

	append(response, GA_U::SEND_INVENTORY & 0xFF, GA_U::SEND_INVENTORY >> 8);
	append(response, 0x02, 0x00);  // 2 top-level items: PAWN_ID + DATA_SET

	Write4B(response, GA_T::PAWN_ID, nPawnId);

	append(response, GA_T::DATA_SET & 0xFF, GA_T::DATA_SET >> 8);
	append(response, 0x01, 0x00);  // 1 item in DATA_SET
		append(response, 0x02, 0x00);  // 2 fields
		Write4B(response, GA_T::INV_REPLICATION_STATE, 0x2);  // 2 = deleted
		Write4B(response, GA_T::INVENTORY_ID, nInventoryId);

	send_response(response);
}

void TcpSession::send_quest_accept_response(int nQuestId) {
	std::vector<uint8_t> response;

	append(response, GA_U::QUEST_UPDATE & 0xFF, GA_U::QUEST_UPDATE >> 8);
	append(response, 0x01, 0x00);  // 1 top-level item: DATA_SET_QUESTS

	append(response, GA_T::DATA_SET_QUESTS & 0xFF, GA_T::DATA_SET_QUESTS >> 8);
	append(response, 0x01, 0x00);  // 1 quest entry
		append(response, 0x02, 0x00);  // 2 fields: QUEST_ID + ACCEPT_FLAG
		Write4B(response, GA_T::QUEST_ID, nQuestId);
		Write1B(response, GA_T::ACCEPT_FLAG, 0x01);

	send_response(response);
}

void TcpSession::send_quest_complete_response(int nQuestId) {
	std::vector<uint8_t> response;

	// Current time as a non-zero 64-bit timestamp — triggers the complete branch in LoadQuests.
	uint64_t ts = static_cast<uint64_t>(
		std::chrono::duration_cast<std::chrono::seconds>(
			std::chrono::system_clock::now().time_since_epoch()).count());

	append(response, GA_U::QUEST_UPDATE & 0xFF, GA_U::QUEST_UPDATE >> 8);
	append(response, 0x01, 0x00);  // 1 top-level item: DATA_SET_QUESTS

	append(response, GA_T::DATA_SET_QUESTS & 0xFF, GA_T::DATA_SET_QUESTS >> 8);
	append(response, 0x01, 0x00);  // 1 quest entry
		append(response, 0x03, 0x00);  // 3 fields: QUEST_ID + COMPLETED_DATETIME + COMPLETE_FLAG
		Write4B(response, GA_T::QUEST_ID, nQuestId);
		// COMPLETED_DATETIME (0x00D8): 8-byte value — non-zero triggers completion branch
		append(response, GA_T::COMPLETED_DATETIME & 0xFF, GA_T::COMPLETED_DATETIME >> 8);
		for (int i = 0; i < 8; i++) append(response, static_cast<uint8_t>((ts >> (i * 8)) & 0xFF));
		Write1B(response, GA_T::COMPLETE_FLAG, 0x01);

	send_response(response);
}

void TcpSession::send_quest_abandon_response(int nQuestId) {
	std::vector<uint8_t> response;

	append(response, GA_U::QUEST_UPDATE & 0xFF, GA_U::QUEST_UPDATE >> 8);
	append(response, 0x01, 0x00);  // 1 top-level item: DATA_SET_QUESTS

	append(response, GA_T::DATA_SET_QUESTS & 0xFF, GA_T::DATA_SET_QUESTS >> 8);
	append(response, 0x01, 0x00);  // 1 quest entry
		append(response, 0x02, 0x00);  // 2 fields: QUEST_ID + ABANDON_FLAG
		Write4B(response, GA_T::QUEST_ID, nQuestId);
		Write1B(response, GA_T::ABANDON_FLAG, 0x01);

	send_response(response);
}

void TcpSession::send_get_loot_table_items_by_id_filtered_response()
{
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::GET_LOOT_TABLE_ITEMS_BY_ID_FILTERED;
	uint16_t item_count = 2;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	append(response, GA_T::DATA_SET & 0xFF, GA_T::DATA_SET >> 8);
	append(response, 0x01, 0x00);        // count elements
	{
		append(response, 0x06, 0x00);  // inner item count

		Write4B(response, GA_T::LOOT_TABLE_ITEM_ID, 3470);
		Write4B(response, GA_T::ITEM_ID, 2991);
		Write4B(response, GA_T::CREATED_ITEM_ID, 2991);
		Write4B(response, GA_T::AUTO_CREATE_FLAG, 122319617);
		Write4B(response, GA_T::MAX_QUALITY_VALUE_ID, 1165);
		Write4B(response, GA_T::SORT_ORDER, 0);
	}

	append(response, GA_T::DATA_SET_PRICES & 0xFF, GA_T::DATA_SET_PRICES >> 8);
	append(response, 0x01, 0x00);        // count elements
	{
		append(response, 0x03, 0x00);  // inner item count

		Write4B(response, GA_T::LOOT_TABLE_ITEM_ID, 3470);
		Write4B(response, GA_T::CURRENT_PRICE, 0);
		Write4B(response, GA_T::CURRENCY_TYPE_VALUE_ID, 1603);
	}

	send_response(response);
}

void TcpSession::send_player_skills_response()
{
	std::vector<uint8_t> response;

	// Authoritative skill allocation for the currently-selected character.
	std::vector<SkillRow> skills;
	double respecUnix = 1.0;
	if (selected_character_id_ != 0) {
		skills = PlayerSessionStore::GetSkillsForCharacter(selected_character_id_);
		int64_t t = PlayerSessionStore::GetLastRespecAt(selected_character_id_);
		if (t > 0) respecUnix = (double)t;
	}

	uint16_t packet_type = GA_U::SEND_PLAYER_SKILLS;
	// Top-level fields: LAST_RESPEC_DATETIME + DATA_SET_CHARACTER_SKILLS.
	uint16_t item_count = 2;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	// LAST_RESPEC_DATETIME is encoded as DATETIME (type 7 = 8 bytes / double).
	// The client parses it via CMarshal::get_datetime_2 — a 4-byte write here
	// would leak garbage into the next field.
	WriteDouble(response, GA_T::LAST_RESPEC_DATETIME, respecUnix);

	uint16_t count = (uint16_t)skills.size();
	append(response, GA_T::DATA_SET_CHARACTER_SKILLS & 0xFF, GA_T::DATA_SET_CHARACTER_SKILLS >> 8);
	append(response, count & 0xFF, count >> 8);
	for (const auto& s : skills) {
		// Client's receive-side lookup (FUN_1141f750) matches rows on THREE
		// keys: SKILL_GROUP_ID, SKILL_ID, and PROFILE_ID — matched against
		// character+0x1634 = r_nItemProfileId (NOT the class profile id).
		// Item profiles aren't implemented yet, so hardcode 0 to match the
		// default; revisit when multi-profile item sets come online.
		append(response, 0x04, 0x00);
		Write4B(response, GA_T::SKILL_GROUP_ID,   (uint32_t)s.skill_group_id);
		Write4B(response, GA_T::SKILL_ID,         (uint32_t)s.skill_id);
		Write4B(response, GA_T::PROFILE_ID,       (uint32_t)item_profile_id_);
		Write4B(response, GA_T::POINTS_ALLOCATED, (uint32_t)s.points);
	}

	Logger::Log("tcp", "[TCP] SEND_PLAYER_SKILLS: sent %d skills for charId=%lld item_profile_id=%d\n",
		(int)skills.size(), (long long)selected_character_id_, (int)item_profile_id_);

	send_response(response);
}

void TcpSession::send_agency_get_roster_response()
{
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::AGENCY_GET_ROSTER;
	uint16_t item_count = 1;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	Write4B(response, GA_T::AGENCY_ID, 0x1);

	send_response(response);
}

void TcpSession::send_start_listen_response()
{
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::GSC_START_LISTEN;
	uint16_t item_count = 15;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	WriteString(response, GA_T::PLAYER_NAME, player_name);
	Write4B(response, GA_T::CHARACTER_ID, 0x2AC950);
	Write4B(response, GA_T::PROFILE_ID, GA_G::PROFILE_ID_ASSAULT);

	Write4B(response, GA_T::CLASS_MSG_ID, 0x022976);
	Write4B(response, GA_T::HOME_MAP_GAME_ID, 0x050B);
	WriteIP(response, GA_T::CHAT_NET_ADDR, s_host_, s_chat_port_);
	WriteNBytes(response, GA_T::SESSION_GUID, std::vector<uint8_t>(16));

	WriteIP(response, GA_T::HOST_NET_ADDR, "127.0.0.1", 9002);
	WriteString(response, GA_T::MAP_FILENAME, "Rot_Redistribution05");
	Write2B(response, GA_T::PARAMETERS, 0x0);
	Write4B(response, GA_T::MAP_GAME_ID, 0x050B);
	Write4B(response, GA_T::MAP_INSTANCE_ID, 0x1);
	Write1B(response, GA_T::NON_COMBAT, 0x0);
	Write1B(response, GA_T::BRIEFING_MOVIES_ONLY_FLAG, 0x0);
	Write4B(response, GA_T::FRIENDLY_NAME_MSG_ID, 0x0000AB95);

	send_response(response);
}

void TcpSession::send_player_update_client_response()
{
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::PLAYER_UPDATE_CLIENT;
	uint16_t item_count = 2;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	Write4B(response, GA_T::SYS_SITE_ID, 0x2);
	Write4B(response, GA_T::SYS_AVA_TEAMS_OK, 0x1);

	send_response(response);
}

void TcpSession::send_update_new_mail_count_response()
{
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::UPDATE_NEW_MAIL_COUNT;
	uint16_t item_count = 1;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	Write4B(response, GA_T::NEW_MAIL_COUNT, 0x0);

	send_response(response);
}

void TcpSession::send_add_player_character_response()
{
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::ADD_PLAYER_CHARACTER;
	uint16_t item_count = 5;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	WriteString(response, GA_T::PLAYER_NAME, player_name);
	Write4B(response, GA_T::CHARACTER_ID, static_cast<uint32_t>(selected_character_id_));
	Write4B(response, GA_T::PROFILE_ID, selected_profile_id_);

	Write4B(response, GA_T::CLASS_MSG_ID, 0x022976);
	Write4B(response, GA_T::HOME_MAP_GAME_ID, 0x050B);

	send_response(response);
}

void TcpSession::send_select_character_response()
{
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::GSC_SELECT_CHARACTER;
	uint16_t item_count = 9;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	// Write4B(response, GA_T::CALLER_ID, 0x0);
	Write4B(response, GA_T::ERROR_CODE, 0x0);
	Write4B(response, GA_T::HOME_MAP_GAME_ID, 0x050B);
	Write4B(response, GA_T::TASK_FORCE, 0x1);
	Write4B(response, GA_T::CHARACTER_ID, static_cast<uint32_t>(selected_character_id_));
	Write4B(response, GA_T::PROFILE_ID, selected_profile_id_ != 0 ? selected_profile_id_ : GA_G::PROFILE_ID_ASSAULT);
	WriteString(response, GA_T::CLASS_MSG_ID, "22976");

	WriteString(response, GA_T::PLAYER_NAME, player_name);
	WriteNBytes(response, GA_T::SESSION_GUID, GuidHexToBytes(session_guid_));
	Logger::Log("tcp", "[%s] send_select_character_response: charId=%lld profile=%u session_guid=%s\n",
		Logger::GetTime(), selected_character_id_, selected_profile_id_, session_guid_.c_str());

	// TODO Phase 8: get instance address from instance registry
	WriteIP(response, GA_T::CHAT_NET_ADDR, s_host_, s_chat_port_);

	send_response(response);
}

void TcpSession::send_change_map_prep_response()
{
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::CHANGE_MAP_PREP;
	uint16_t item_count = 15;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	WriteString(response, GA_T::PLAYER_NAME, player_name);
	Write4B(response, GA_T::CHARACTER_ID, 0x2AC950);
	Write4B(response, GA_T::PROFILE_ID, GA_G::PROFILE_ID_ASSAULT);

	Write4B(response, GA_T::CLASS_MSG_ID, 0x022976);
	Write4B(response, GA_T::HOME_MAP_GAME_ID, 0x050B);
	WriteIP(response, GA_T::CHAT_NET_ADDR, s_host_, s_chat_port_);
	WriteNBytes(response, GA_T::SESSION_GUID, std::vector<uint8_t>(16));

	WriteIP(response, GA_T::HOST_NET_ADDR, "127.0.0.1", 9002);
	WriteString(response, GA_T::MAP_FILENAME, "Rot_Redistribution05");
	Write2B(response, GA_T::PARAMETERS, 0x0);
	Write4B(response, GA_T::MAP_GAME_ID, 0x050B);
	Write4B(response, GA_T::MAP_INSTANCE_ID, 0x1);
	Write1B(response, GA_T::NON_COMBAT, 0x0);
	Write1B(response, GA_T::BRIEFING_MOVIES_ONLY_FLAG, 0x0);
	Write4B(response, GA_T::FRIENDLY_NAME_MSG_ID, 0x0000AB95);

	send_response(response);
}

void TcpSession::send_change_map_response()
{
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::GSC_CHANGE_MAP;
	uint16_t item_count = 15;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	WriteString(response, GA_T::PLAYER_NAME, player_name);
	Write4B(response, GA_T::CHARACTER_ID, 0x2AC950);
	Write4B(response, GA_T::PROFILE_ID, GA_G::PROFILE_ID_ASSAULT);

	Write4B(response, GA_T::CLASS_MSG_ID, 0x022976);
	Write4B(response, GA_T::HOME_MAP_GAME_ID, 0x050B);
	WriteIP(response, GA_T::CHAT_NET_ADDR, s_host_, s_chat_port_);
	WriteNBytes(response, GA_T::SESSION_GUID, std::vector<uint8_t>(16));

	WriteIP(response, GA_T::HOST_NET_ADDR, "127.0.0.1", 9002);
	WriteString(response, GA_T::MAP_FILENAME, "Rot_Redistribution05");
	Write2B(response, GA_T::PARAMETERS, 0x0);
	Write4B(response, GA_T::MAP_GAME_ID, 0x050B);
	Write4B(response, GA_T::MAP_INSTANCE_ID, 0x1);
	Write1B(response, GA_T::NON_COMBAT, 0x0);
	Write1B(response, GA_T::BRIEFING_MOVIES_ONLY_FLAG, 0x0);
	Write4B(response, GA_T::FRIENDLY_NAME_MSG_ID, 0x0000AB95);

	send_response(response);
}

void TcpSession::send_match_launch_response()
{
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::GSC_LAUNCH_MAP_INSTANCE;
	uint16_t item_count = 15;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	WriteString(response, GA_T::PLAYER_NAME, player_name);
	Write4B(response, GA_T::CHARACTER_ID, 0x2AC950);
	Write4B(response, GA_T::PROFILE_ID, GA_G::PROFILE_ID_ASSAULT);

	Write4B(response, GA_T::CLASS_MSG_ID, 0x022976);
	Write4B(response, GA_T::HOME_MAP_GAME_ID, 0x050B);
	WriteIP(response, GA_T::CHAT_NET_ADDR, s_host_, s_chat_port_);
	WriteNBytes(response, GA_T::SESSION_GUID, std::vector<uint8_t>(16));

	WriteIP(response, GA_T::HOST_NET_ADDR, "127.0.0.1", 9002);
	WriteString(response, GA_T::MAP_FILENAME, "Rot_Redistribution05");
	Write2B(response, GA_T::PARAMETERS, 0x0);
	Write4B(response, GA_T::MAP_GAME_ID, 0x050B);
	Write4B(response, GA_T::MAP_INSTANCE_ID, 0x1);
	Write1B(response, GA_T::NON_COMBAT, 0x0);
	Write1B(response, GA_T::BRIEFING_MOVIES_ONLY_FLAG, 0x0);
	Write4B(response, GA_T::FRIENDLY_NAME_MSG_ID, 0x0000AB95);

	send_response(response);
}

void TcpSession::send_go_play_tutorial_response()
{
	auto instance = InstanceRegistry::GetReadyHomeInstance();
	if (!instance) {
		Logger::Log("tcp", "[TcpSession] No READY home map instance for go_play_tutorial\n");
		return;
	}

	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::GSC_GO_PLAY;
	uint16_t item_count = 10;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	WriteString(response, GA_T::PLAYER_NAME, player_name);
	WriteIP(response, GA_T::HOST_NET_ADDR, instance->ip_address, instance->udp_port);
	WriteNBytes(response, GA_T::SESSION_GUID, GuidHexToBytes(session_guid_));
	WriteString(response, GA_T::MAP_FILENAME, "Inception_ALL");
	WriteString(response, GA_T::PARAMETERS, "?Game=TgGame.TgGame_Mission");
	Write4B(response, GA_T::FRIENDLY_NAME_MSG_ID, 22845);
	Write4B(response, GA_T::MAP_GAME_ID, 0x050B);
	Write4B(response, GA_T::MAP_INSTANCE_ID, 0x1);
	Write4B(response, GA_T::ENTRY_BACKGROUND_IMAGE_RES_ID, 5289);
	Write4B(response, GA_T::TASK_FORCE, 0x1);
	WriteIP(response, GA_T::CHAT_NET_ADDR, s_host_, s_chat_port_);

	send_response(response);
}

void TcpSession::send_marshal_channel_response()
{
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::MARSHAL_CHANNEL;
	uint16_t item_count = 3;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	WriteNBytes(response, GA_T::SESSION_GUID, std::vector<uint8_t>(16));
	Write4B(response, GA_T::VERSION, 4869);
	WriteString(response, GA_T::TEXT_VALUE, player_name);

	send_response(response);
}

void TcpSession::send_go_play_response()
{
	auto instance = InstanceRegistry::GetReadyHomeInstance();
	if (!instance) {
		Logger::Log("tcp", "[TcpSession] No READY home map instance for go_play\n");
		return;
	}
	send_go_play_to_instance(*instance, /*task_force=*/1);
}

void TcpSession::send_go_play_to_instance(const InstanceInfo& target, int task_force)
{
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::GSC_GO_PLAY;
	uint16_t item_count = 12;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	// Resolve map metadata from the preloaded map_game_info registry. Falls
	// back to the legacy hardcoded HQ values if the map isn't in the table
	// (NULL map_name rows, manually-added maps, etc.) — preserves the old
	// behavior for any unseeded map.
	uint32_t friendly_name_msg_id          = 22845;   // Commonwealth HQ (legacy fallback)
	uint32_t map_game_id                   = 0x050B;  // legacy fallback
	uint32_t entry_background_image_res_id = 5716;    // legacy fallback
	if (auto info = MapGameInfo::LookupByName(target.map_name)) {
		friendly_name_msg_id          = info->friendly_name_msg_id;
		map_game_id                   = info->map_game_id;
		entry_background_image_res_id = info->entry_background_image_res_id;
	} else {
		Logger::Log("tcp", "[TcpSession] send_go_play_to_instance: no map_game_info row "
			"for map_name='%s'; falling back to HQ defaults\n", target.map_name.c_str());
	}

	WriteString(response, GA_T::PLAYER_NAME, player_name);
	WriteIP(response, GA_T::HOST_NET_ADDR, target.ip_address, target.udp_port);
	WriteNBytes(response, GA_T::SESSION_GUID, GuidHexToBytes(session_guid_));
	WriteString(response, GA_T::MAP_FILENAME, target.map_name);
	WriteString(response, GA_T::PARAMETERS, "?Game=" + target.game_mode);
	Write4B(response, GA_T::FRIENDLY_NAME_MSG_ID, friendly_name_msg_id);
	Write4B(response, GA_T::MAP_GAME_ID, map_game_id);
	Write4B(response, GA_T::MAP_INSTANCE_ID, static_cast<uint32_t>(target.instance_id));
	Write4B(response, GA_T::ENTRY_BACKGROUND_IMAGE_RES_ID, entry_background_image_res_id);
	Write4B(response, GA_T::TASK_FORCE, static_cast<uint32_t>(task_force));
	WriteIP(response, GA_T::CHAT_NET_ADDR, s_host_, s_chat_port_);

	Write4B(response, GA_T::CLASS_MSG_ID, 22976);

	send_response(response);
	Logger::Log("tcp", "[TcpSession] Sent GSC_GO_PLAY → instance=%lld map=%s game_id=%u name_msg=%u bg_res=%u tf=%d for %s\n",
		(long long)target.instance_id, target.map_name.c_str(),
		map_game_id, friendly_name_msg_id, entry_background_image_res_id,
		task_force, session_guid_.c_str());
}

void TcpSession::send_instance_ready_response()
{
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::MATCH_LAUNCH_TIMEOUT;
	uint16_t item_count = 2;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	Write4B(response, GA_T::CALLER_ID, 0x0);
	Write4B(response, GA_T::MSG_ID, 33528);

	send_response(response);
}


void TcpSession::send_character_list_response()
{
	auto characters = PlayerSessionStore::GetCharactersByUserId(user_id_);

	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::GSC_CHARACTER_LIST;
	uint16_t item_count = 3;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	WriteString(response, GA_T::PLAYER_NAME, player_name.c_str());

	append(response, GA_T::DATA_SET & 0xFF, GA_T::DATA_SET >> 8);
	append(response, static_cast<uint8_t>(characters.size() & 0xFF),
	                 static_cast<uint8_t>(characters.size() >> 8));

	for (const auto& c : characters) {
		append(response, 0x0B, 0x00);  // inner item count = 11
		Write4B(response, GA_T::CHARACTER_ID,           static_cast<uint32_t>(c.id));
		Write4B(response, GA_T::CHARACTER_LEVEL,        0x32);
		Write4B(response, GA_T::CURRENT_LEVEL,          0x32);
		Write4B(response, GA_T::FORCED_CHARACTER_LEVEL, 0x32);
		Write4B(response, GA_T::PROFILE_ID,             c.profile_id);
		Write4B(response, GA_T::HEAD_ASM_ID,            c.head_asm_id);
		Write4B(response, GA_T::HAIR_ASM_ID,            0x85D);
		Write4B(response, GA_T::GENDER_TYPE_VALUE_ID,   c.gender_type_value_id);
		WriteString(response, GA_T::MAP_NAME,           "Scramble: Tetra Pier");
		Write4B(response, GA_T::XP_VALUE,               0xbbddc);
		Write4B(response, GA_T::SKILL_GROUP_SET_ID,     GetClassConfig(c.profile_id).skillGroupSetId);
	}

	append(response, GA_T::DATA_SET_PLAYER_INVENTORY & 0xFF, GA_T::DATA_SET_PLAYER_INVENTORY >> 8);
	append(response, 0x00, 0x00);  // count elements

	send_response(response);
}

void TcpSession::send_character_list_response_mock()
{
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::GSC_CHARACTER_LIST;
	uint16_t item_count = 3;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	WriteString(response, GA_T::PLAYER_NAME, player_name.c_str());

	// ==== arrayofenumblockarrays (010C) ====
	append(response, GA_T::DATA_SET & 0xFF, GA_T::DATA_SET >> 8);
	append(response, 0x04, 0x00);        // count elements

	// ASSAULT
	append(response, 0x0B, 0x00);  // inner item count
	Write4B(response, GA_T::CHARACTER_ID, 0x2AC950);
	Write4B(response, GA_T::CHARACTER_LEVEL, 0x32);
	Write4B(response, GA_T::CURRENT_LEVEL, 0x32);
	Write4B(response, GA_T::FORCED_CHARACTER_LEVEL, 0x32);
	Write4B(response, GA_T::PROFILE_ID, GA_G::PROFILE_ID_ASSAULT);
	Write4B(response, GA_T::HEAD_ASM_ID, GA_G::HEAD_ASM_ID_TROLL);
	Write4B(response, GA_T::HAIR_ASM_ID, 0x85D);
	Write4B(response, GA_T::GENDER_TYPE_VALUE_ID, 0x355);
	WriteString(response, GA_T::MAP_NAME, "Scramble: Tetra Pier");
	Write4B(response, GA_T::XP_VALUE, 0xbbddc);
	Write4B(response, GA_T::SKILL_GROUP_SET_ID, GA_G::SKILL_GROUP_SET_ID_ASSAULT);

	// MEDIC
	append(response, 0x0B, 0x00);  // inner item count
	Write4B(response, GA_T::CHARACTER_ID, 0x0000f69c);
	Write4B(response, GA_T::CHARACTER_LEVEL, 0x32);
	Write4B(response, GA_T::CURRENT_LEVEL, 0x32);
	Write4B(response, GA_T::FORCED_CHARACTER_LEVEL, 0x32);
	Write4B(response, GA_T::PROFILE_ID, GA_G::PROFILE_ID_MEDIC);
	Write4B(response, GA_T::HEAD_ASM_ID, GA_G::HEAD_ASM_ID_TROLL);
	Write4B(response, GA_T::HAIR_ASM_ID, 0x85D);
	Write4B(response, GA_T::GENDER_TYPE_VALUE_ID, 0x355);
	WriteString(response, GA_T::MAP_NAME, "Commonwealth HQ");
	Write4B(response, GA_T::XP_VALUE, 0xbbddc);
	Write4B(response, GA_T::SKILL_GROUP_SET_ID, GA_G::SKILL_GROUP_SET_ID_MEDIC);

	// RECON
	append(response, 0x0B, 0x00);  // inner item count
	Write4B(response, GA_T::CHARACTER_ID, 0x2AC950);
	Write4B(response, GA_T::CHARACTER_LEVEL, 0x32);
	Write4B(response, GA_T::CURRENT_LEVEL, 0x32);
	Write4B(response, GA_T::FORCED_CHARACTER_LEVEL, 0x32);
	Write4B(response, GA_T::PROFILE_ID, GA_G::PROFILE_ID_RECON);
	Write4B(response, GA_T::HEAD_ASM_ID, GA_G::HEAD_ASM_ID_TROLL);
	Write4B(response, GA_T::HAIR_ASM_ID, 0x85D);
	Write4B(response, GA_T::GENDER_TYPE_VALUE_ID, 0x355);
	WriteString(response, GA_T::MAP_NAME, "Scramble: Tetra Pier");
	Write4B(response, GA_T::XP_VALUE, 0xbbddc);
	Write4B(response, GA_T::SKILL_GROUP_SET_ID, GA_G::SKILL_GROUP_SET_ID_RECON);

	// ROBO
	append(response, 0x0B, 0x00);  // inner item count
	Write4B(response, GA_T::CHARACTER_ID, 0x2AC950);
	Write4B(response, GA_T::CHARACTER_LEVEL, 0x32);
	Write4B(response, GA_T::CURRENT_LEVEL, 0x32);
	Write4B(response, GA_T::FORCED_CHARACTER_LEVEL, 0x32);
	Write4B(response, GA_T::PROFILE_ID, GA_G::PROFILE_ID_ROBOTIC);
	Write4B(response, GA_T::HEAD_ASM_ID, GA_G::HEAD_ASM_ID_TROLL);
	Write4B(response, GA_T::HAIR_ASM_ID, 0x85D);
	Write4B(response, GA_T::GENDER_TYPE_VALUE_ID, 0x355);
	WriteString(response, GA_T::MAP_NAME, "alsdfslfjlj");
	Write4B(response, GA_T::XP_VALUE, 0xbbddc);
	Write4B(response, GA_T::SKILL_GROUP_SET_ID, GA_G::SKILL_GROUP_SET_ID_ROBOTIC);

	append(response, GA_T::DATA_SET_PLAYER_INVENTORY & 0xFF, GA_T::DATA_SET_PLAYER_INVENTORY >> 8);
	append(response, 0x00, 0x00);        // count elements

	send_response(response);
}

void TcpSession::send_character_list_queue_response()
{
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::GSC_CHARACTER_LIST;
	uint16_t item_count = 3;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	Write4B(response, GA_T::MIN_WAIT_SEC, 0x04567);
	Write4B(response, GA_T::ACTIVE_POSITION, 0x11);
	Write4B(response, GA_T::ENTRY_NUMBER, 0x19);

	send_response(response);
}


void TcpSession::send_login_response()
{
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::GSC_USER_LOGIN_RESPONSE;
	uint16_t item_count = 9;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	Write1B(response,     GA_T::SUCCESS, 1);
	WriteNBytes(response, GA_T::SESSION_GUID, GuidHexToBytes(session_guid_));
	Write4B(response,     GA_T::GAME_BITS, 0x0000000F);
	Write2B(response,     GA_T::NET_ACCESS_FLAGS, 0xF3F8);
	Write4B(response,     GA_T::AFK_TIMEOUT_SEC, 0x00000384);
	WriteNBytes(response, GA_T::DISPLAY_EULA_FLAG, {0x01, 0x00, 0x6E});
	WriteString(response, GA_T::PLAYER_NAME, player_name.c_str());

	append(response, GA_T::DATA_SET & 0xFF, GA_T::DATA_SET >> 8);
	append(response, 0x01, 0x00);        // count elements

	append(response, 0x02, 0x00);  // inner item count
	Write4B(response, GA_T::SYS_SITE_ID, 0x00000002);
	Write4B(response, GA_T::NAME_MSG_ID, 0x0000A20B);

	append(response, GA_T::DATA_SET_MAP_CONFIGS & 0xFF, GA_T::DATA_SET_MAP_CONFIGS >> 8);
	append(response, 0x01, 0x00);    // 1 item

	append(response, 0x02, 0x00);  // inner item count
	Write4B(response, GA_T::MAP_GAME_ID, 0x0000046B);
	Write4B(response, GA_T::FRIENDLY_NAME_MSG_ID, 0x0000AB95);

	send_response(response);
}
