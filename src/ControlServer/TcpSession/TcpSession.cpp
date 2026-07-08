#include "src/ControlServer/TcpSession/TcpSession.hpp"
#include "src/ControlServer/Database/Database.hpp"
#include "src/ControlServer/Auth/LoginAuth.hpp"
#include "src/ControlServer/Constants/DeviceIds.hpp"
#include "src/ControlServer/Loadouts/ArmorLoadouts.hpp"
#include "src/ControlServer/MapGameInfo/MapGameInfo.hpp"
#include "src/ControlServer/MatchmakingService/TicketInfoEncoder.hpp"
#include "src/ControlServer/TeamService/TeamService.hpp"
#include "src/ControlServer/MatchmakingService/RuntimeStats.hpp"
#include "src/ControlServer/MatchmakingService/RoleWeightedSplit.hpp"
#include "src/Shared/IpcProtocol.hpp"
#include <set>
#include <map>
#include <array>
#include <cctype>
#include <cstdlib>
#include <ctime>
#include <mutex>
#ifndef _WIN32
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <cerrno>
#endif

// ---------------------------------------------------------------------------
// TcpSession static member definitions
// ---------------------------------------------------------------------------

std::mutex TcpSession::sessions_mutex_;
std::map<std::string, std::weak_ptr<TcpSession>> TcpSession::g_sessions_;
std::function<void()> TcpSession::on_need_home_map_;
std::string TcpSession::s_host_ = "127.0.0.1";
uint16_t    TcpSession::s_chat_port_ = 9001;
bool        TcpSession::s_allow_duplicate_account_logins_ = false;
bool        TcpSession::s_require_password_verification_   = true;
std::string TcpSession::s_ban_spoof_mode_                = "silent";
int         TcpSession::s_ban_spoof_fallback_close_sec_  = 0;
int         TcpSession::s_kick_fallback_close_sec_       = 30;

// Tuned tighter than ChatSession's helper because this is the gating socket
// for re-login: dead peers MUST be reaped fast or the next attempt hits the
// "duplicate account" rejection. Detection ~50s: 20s idle + 3 probes * 10s.
// Tolerates a ~20s network blip before probing starts; declared dead at 50s.
void TcpSession::EnableKeepAlive() {
    std::error_code ec;
    socket_.set_option(asio::socket_base::keep_alive(true), ec);
    if (ec) {
        Logger::Log("tcp", "[TCP] set keep_alive failed: %s\n", ec.message().c_str());
        return;
    }
#ifndef _WIN32
    int fd = socket_.native_handle();
    int idle = 20, intvl = 10, count = 3;
    if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE,  &idle,  sizeof(idle))  < 0 ||
        setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &intvl, sizeof(intvl)) < 0 ||
        setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT,   &count, sizeof(count)) < 0) {
        Logger::Log("tcp", "[TCP] TCP_KEEPIDLE/INTVL/CNT tuning failed (errno=%d)\n", errno);
    }
#endif
}

namespace {

constexpr auto kFailedLoginCooldown = std::chrono::seconds(5);
std::mutex g_failed_login_cooldown_mutex;
std::map<std::string, std::chrono::steady_clock::time_point> g_failed_login_cooldowns;

bool IsLoginIpCoolingDown(const std::string& remote_ip) {
    if (remote_ip.empty()) {
        return false;
    }

    const auto now = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> lock(g_failed_login_cooldown_mutex);
    auto it = g_failed_login_cooldowns.find(remote_ip);
    if (it == g_failed_login_cooldowns.end()) {
        return false;
    }
    if (it->second <= now) {
        g_failed_login_cooldowns.erase(it);
        return false;
    }
    return true;
}

void MarkFailedLoginIp(const std::string& remote_ip) {
    if (remote_ip.empty()) {
        return;
    }

    std::lock_guard<std::mutex> lock(g_failed_login_cooldown_mutex);
    g_failed_login_cooldowns[remote_ip] =
        std::chrono::steady_clock::now() + kFailedLoginCooldown;
}

uint32_t FallbackClassMsgId(uint32_t profile_id) {
    switch (profile_id) {
        case GA_G::PROFILE_ID_ROBOTIC: return 15447;
        case GA_G::PROFILE_ID_ASSAULT: return 15448;
        case GA_G::PROFILE_ID_RECON:   return 15449;
        case GA_G::PROFILE_ID_MEDIC:   return 15450;
        default:                       return 22976;
    }
}

std::string TrimAsciiWhitespace(const std::string& s) {
    size_t first = 0;
    while (first < s.size() && std::isspace(static_cast<unsigned char>(s[first]))) {
        ++first;
    }
    size_t last = s.size();
    while (last > first && std::isspace(static_cast<unsigned char>(s[last - 1]))) {
        --last;
    }
    return s.substr(first, last - first);
}

bool IsPrintableAsciiUsername(const std::string& s) {
    if (s.empty()) return false;
    for (unsigned char c : s) {
        if (c < 0x20 || c > 0x7E) return false;
    }
    return true;
}

bool EqualsIgnoreAsciiCase(const std::string& a, const char* b) {
    size_t i = 0;
    for (; i < a.size() && b[i] != '\0'; ++i) {
        const unsigned char ca = static_cast<unsigned char>(a[i]);
        const unsigned char cb = static_cast<unsigned char>(b[i]);
        if (std::tolower(ca) != std::tolower(cb)) return false;
    }
    return i == a.size() && b[i] == '\0';
}

uint32_t ResolveClassMsgId(uint32_t profile_id, uint32_t fallback, const char* context) {
    const uint32_t profile_fallback = FallbackClassMsgId(profile_id);
    const uint32_t resolved_fallback =
        (profile_fallback != 22976 || fallback == 0) ? profile_fallback : fallback;
    sqlite3* db = Database::GetConnection();
    if (!db) {
        Logger::Log("tcp", "[TcpSession] %s: no DB for class_msg profile=%u, fallback=%u\n",
            context ? context : "class_msg", profile_id, resolved_fallback);
        return resolved_fallback;
    }

    sqlite3_stmt* stmt = nullptr;
    const int rc = sqlite3_prepare_v2(db,
        "SELECT COALESCE(name_msg_id, 0) FROM asm_data_set_bots WHERE bot_id = ? LIMIT 1",
        -1, &stmt, nullptr);
    if (rc != SQLITE_OK || !stmt) {
        Logger::Log("tcp", "[TcpSession] %s: class_msg prepare failed profile=%u fallback=%u err=%s\n",
            context ? context : "class_msg", profile_id, resolved_fallback, sqlite3_errmsg(db));
        if (stmt) sqlite3_finalize(stmt);
        return resolved_fallback;
    }

    sqlite3_bind_int(stmt, 1, static_cast<int>(profile_id));
    uint32_t class_msg_id = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        class_msg_id = static_cast<uint32_t>(sqlite3_column_int(stmt, 0));
    }
    sqlite3_finalize(stmt);

    if (class_msg_id == 0) {
        Logger::Log("tcp", "[TcpSession] %s: no class_msg row for profile=%u, fallback=%u\n",
            context ? context : "class_msg", profile_id, resolved_fallback);
        return resolved_fallback;
    }

    return class_msg_id;
}

uint32_t ResolveHomeMapGameId() {
    if (auto info = MapGameInfo::LookupByName("Dome3_VR_Arena_P")) {
        return info->map_game_id;
    }
    return 100005;
}

int ChooseVrHomeTaskForce(const InstanceInfo& home_instance,
                          uint32_t joining_profile_id,
                          const std::string& joining_session_guid) {
    if (home_instance.map_name != "Dome3_VR_Arena_P") {
        return 1;
    }

    struct TeamComposition {
        int players = 0;
        int same_class = 0;
    };

    TeamComposition team1;
    TeamComposition team2;
    for (const auto& row : InstanceRegistry::GetActivePlayersForInstance(home_instance.instance_id)) {
        if (row.guid == joining_session_guid) {
            continue;
        }

        TeamComposition& team = (row.task_force == 2) ? team2 : team1;
        team.players += 1;
        if (joining_profile_id != 0 && row.profile_id == joining_profile_id) {
            team.same_class += 1;
        }
    }

    int task_force = 1;
    if (team2.players < team1.players) {
        task_force = 2;
    } else if (team1.players == team2.players && team2.same_class < team1.same_class) {
        task_force = 2;
    }

    Logger::Log("tcp",
        "[TcpSession] VR join team balance: instance=%lld profile=%u team1=(players=%d class=%d) team2=(players=%d class=%d) -> tf=%d\n",
        (long long)home_instance.instance_id, joining_profile_id,
        team1.players, team1.same_class, team2.players, team2.same_class, task_force);
    return task_force;
}

bool SendProfileUiRefresh(int64_t instance_id, const std::string& session_guid, int item_profile_id,
                          uint64_t refresh_token, const char* reason) {
    if (instance_id == 0) {
        return false;
    }

    nlohmann::json action;
    action["type"] = IpcProtocol::MSG_PLAYER_ACTION;
    action["session_guid"] = session_guid;
    action["action"] = "refresh_profile_ui";
    action["item_profile_id"] = item_profile_id;
    action["refresh_token"] = refresh_token;
    action["phase"] = reason ? reason : "unknown";
    const bool ok = IpcServer::SendToInstance(instance_id, action.dump());
    Logger::Log("loadout",
        "[TcpSession] profile_switch refresh_profile_ui %s instance=%lld send=%d guid=%s itemProf=%d token=%llu\n",
        reason ? reason : "unknown", (long long)instance_id, (int)ok, session_guid.c_str(),
        item_profile_id, (unsigned long long)refresh_token);
    return ok;
}

}  // namespace

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

void TcpSession::SetLoginPolicy(bool allow_duplicate_account_logins,
                               bool require_password_verification) {
    s_allow_duplicate_account_logins_ = allow_duplicate_account_logins;
    s_require_password_verification_  = require_password_verification;
}

void TcpSession::SetModerationConfig(const std::string& ban_spoof_mode,
                                     int ban_spoof_fallback_close_sec,
                                     int kick_fallback_close_sec) {
    s_ban_spoof_mode_               = ban_spoof_mode;
    s_ban_spoof_fallback_close_sec_ = ban_spoof_fallback_close_sec;
    s_kick_fallback_close_sec_      = kick_fallback_close_sec;
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
        const int64_t character_id = session->selected_character_id_;
        Logger::Log("ipc", "[TcpSession] DeliverGameEvent: spawn pawn_id=%d item_profile_id=%d char=%lld guid=%s\n",
            pawn_id, (int)session->item_profile_id_, (long long)character_id, session_guid.c_str());
        if (pawn_id != 0 && character_id != 0) {
            session->send_inventory_clear(pawn_id, character_id);
        }
        session->send_inventory_response(pawn_id, character_id);
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
        int item_profile_id    = j.value("item_profile_id", 1);
        Logger::Log("ipc", "[TcpSession] DeliverGameEvent: beacon_pickup pawn=%d dev=%d inv=%d slot=%d profile=%d\n",
            pawn_id, device_id, inventory_id, equip_slot_value_id, item_profile_id);
        session->send_beacon_pickup_response(pawn_id, device_id, inventory_id, equip_slot_value_id, item_profile_id);
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
        Logger::Log("ipc",
            "[TcpSession] DeliverGameEvent: skill_save char=%lld pawnId=%d rows=%d guid=%s\n",
            (long long)character_id, pawn_id, (int)skills.size(), session_guid.c_str());
        if (character_id != 0) {
            PlayerSessionStore::SetSkillsForCharacter(
                character_id, (int)session->item_profile_id_, skills);
        }
        // Echo current DB state so the UI's inbound 0x00A0 handler has
        // authoritative data whether this was a save or a request.
        session->send_player_skills_response();
		// Avoid a full SEND_INVENTORY blast here. Skill saves can happen while
		// the profile UI is open and the bag pool is 1000+ entries; repeatedly
		// rebuilding it tanks the client. Equipment changes own inventory sync.
    }
    else if (subtype == "skill_respec") {
        int64_t character_id = j.value("character_id", (int64_t)0);
        int     pawn_id      = j.value("pawn_id", 0);
        Logger::Log("ipc", "[TcpSession] DeliverGameEvent: skill_respec char=%lld pawnId=%d\n",
            (long long)character_id, pawn_id);
        if (character_id != 0) {
            PlayerSessionStore::ClearSkillsForCharacter(
                character_id, (int)session->item_profile_id_);
        }
        // Echo the zeroed allocation to the client so the UI matches.
        session->send_player_skills_response();
		// Do not resend inventory for respec; skill response is authoritative
		// and equipment changes have their own inventory sync.
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
        // Engine slots (6=Suit, 12=Helmet) the client explicitly blanked —
        // SlotIndices[slot]==0, a genuine "make this empty" signal, not
        // "didn't touch this slot". See CosmeticEquip::ClearSlot.
        std::set<int> cleared_cosmetic_slots;
        if (j.contains("cleared_cosmetic_slots") && j["cleared_cosmetic_slots"].is_array()) {
            for (const auto& v : j["cleared_cosmetic_slots"]) {
                if (v.is_number_integer()) cleared_cosmetic_slots.insert(v.get<int>());
            }
        }
        Logger::Log("armor",
            "[equip_save] char=%lld pawnId=%d loadout=%d slots=%zu misc=%zu cleared=%zu guid=%s\n",
            (long long)character_id, pawn_id, loadout_profile,
            slot_to_inventory.size(), misc_items.size(), cleared_cosmetic_slots.size(),
            session_guid.c_str());
        for (const auto& kv : slot_to_inventory) {
            Logger::Log("armor",
                "[equip_save]   SlotIndices[%d] = inv_id %d\n", kv.first, kv.second);
        }
        for (const auto& kv : misc_items) {
            Logger::Log("armor",
                "[equip_save]   MiscItems[%d]   = inv_id %d\n", kv.first, kv.second);
        }
        for (int slot : cleared_cosmetic_slots) {
            Logger::Log("armor",
                "[equip_save]   ClearedCosmeticSlot[%d]\n", slot);
        }

        if (character_id != 0) {
            const bool ok = PlayerSessionStore::SaveEquippedDevices(
                character_id, session->user_id_, session->selected_profile_id_,
                loadout_profile,
                slot_to_inventory, misc_items, cleared_cosmetic_slots);
            if (!ok) {
                Logger::Log("armor",
                    "[equip_save] REJECTED — DB state unchanged; client will resync from existing\n");
            }
			// Push the authoritative equipped set back regardless of
			// accept/reject — accept = client sees its save; reject = client's
			// optimistic UI gets corrected to what's actually persisted.
			if (pawn_id != 0) {
				session->send_inventory_clear(pawn_id, character_id);
				session->send_inventory_response(pawn_id, character_id);
			}
		}
	}
    else if (subtype == "profile_switch") {
        // Game-server has finished its atomic switch (revert + apply + IPC).
        // Control-server's job: persist new active profile, sync TcpSession
        // mirror, push refreshed inventory + skills so the client UI sees
        // the new build's stats and allocations.
        const int64_t character_id    = j.value("character_id", (int64_t)0);
        const int64_t instance_id      = j.value("instance_id", (int64_t)0);
        const int     pawn_id         = j.value("pawn_id", 0);
        const int     item_profile_id = j.value("item_profile_id", 1);
        Logger::Log("loadout",
            "[TcpSession] profile_switch char=%lld pawnId=%d itemProf=%d guid=%s\n",
            (long long)character_id, pawn_id, item_profile_id, session_guid.c_str());

        if (character_id != 0 && item_profile_id >= 1 && item_profile_id <= 5) {
            PlayerSessionStore::SetCurrentItemProfile(character_id, item_profile_id);
            session->item_profile_id_ = item_profile_id;

            if (pawn_id != 0) {
                // Clear-then-re-add so the client's m_InventoryMap entries get
                // fully rebuilt instead of merge-updated. The bar widget binds
                // to the per-profile slot data inside those entries; without
                // the wipe, stale "equipped in profile N" rows survive and
                // the bar keeps rendering old icons on full → empty switches.
                // Cheap (~12 bytes/record, batched) and mirrors the proven
                // disconnect+reconnect refresh shape.
                session->send_inventory_clear(pawn_id, character_id);
                session->send_inventory_response(pawn_id, character_id);
            }
            session->send_player_skills_response();

            if (instance_id != 0) {
                const uint64_t refresh_token = ++session->profile_refresh_token_;
                SendProfileUiRefresh(instance_id, session_guid, item_profile_id, refresh_token, "immediate");
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
    if (!ok) {
        Logger::Log("chat-command", "[ChatCmd] DeliverPlayerAction: guid=%s instance=%lld send=0\n",
            session_guid.c_str(), (long long)session->assigned_instance_id_);
    }
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

bool TcpSession::ForceMissionExit(const std::string& session_guid,
                                  int64_t parent_instance_id,
                                  const char* reason) {
    if (session_guid.empty() || parent_instance_id == 0) {
        return false;
    }

    std::shared_ptr<TcpSession> session;
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        auto it = g_sessions_.find(session_guid);
        if (it == g_sessions_.end()) {
            InstanceRegistry::MarkInstancePlayerLeft(parent_instance_id, session_guid);
            Logger::Log("tcp",
                "[TcpSession] ForceMissionExit: no TcpSession for guid=%s parent=%lld reason=%s\n",
                session_guid.c_str(), (long long)parent_instance_id,
                reason ? reason : "unspecified");
            return false;
        }
        session = it->second.lock();
        if (!session) {
            g_sessions_.erase(it);
            InstanceRegistry::MarkInstancePlayerLeft(parent_instance_id, session_guid);
            Logger::Log("tcp",
                "[TcpSession] ForceMissionExit: expired TcpSession for guid=%s parent=%lld reason=%s\n",
                session_guid.c_str(), (long long)parent_instance_id,
                reason ? reason : "unspecified");
            return false;
        }
    }

    if (session->assigned_instance_id_ != parent_instance_id) {
        InstanceRegistry::MarkInstancePlayerLeft(parent_instance_id, session_guid);
        Logger::Log("tcp",
            "[TcpSession] ForceMissionExit: guid=%s already assigned instance=%lld parent=%lld reason=%s\n",
            session_guid.c_str(), (long long)session->assigned_instance_id_,
            (long long)parent_instance_id, reason ? reason : "unspecified");
        return true;
    }

    session->route_from_mission_instance(parent_instance_id, reason);
    return true;
}

void TcpSession::route_from_mission_instance(int64_t parent_instance_id,
                                             const char* reason) {
    std::optional<InstanceInfo> parent;
    if (parent_instance_id != 0) {
        parent = InstanceRegistry::GetInstanceById(parent_instance_id);
    }
    if (parent && parent->is_home_map) {
        Logger::Log("tcp",
            "[TcpSession] Mission exit ignored for home instance=%lld guid=%s reason=%s\n",
            (long long)parent_instance_id, session_guid_.c_str(),
            reason ? reason : "unspecified");
        return;
    }

    std::optional<InstanceInfo> successor;
    bool tryContinue = false;
    if (parent && parent->end_mission_at != 0 && parent->queue_id != 0) {
        if (MatchmakingService::GetContinueInQueue(parent->queue_id)) {
            successor = InstanceRegistry::GetSuccessor(parent_instance_id);
            if (successor && successor->state == "READY") {
                const bool hasSeat = (successor->max_players == 0)
                    || (successor->player_count < successor->max_players);
                if (hasSeat) {
                    tryContinue = true;
                } else {
                    Logger::Log("tcp",
                        "[TcpSession] Successor %lld for parent=%lld is full (%d/%d) -- routing home instead\n",
                        (long long)successor->instance_id,
                        (long long)parent_instance_id,
                        successor->player_count, successor->max_players);
                }
            } else {
                Logger::Log("tcp",
                    "[TcpSession] No READY successor for parent=%lld (got %s) -- routing home instead\n",
                    (long long)parent_instance_id,
                    successor ? successor->state.c_str() : "<none>");
            }
        }
    }

    if (parent_instance_id != 0 && !session_guid_.empty()) {
        InstanceRegistry::MarkInstancePlayerLeft(parent_instance_id, session_guid_);
    }

    if (pending_ack_timer_) {
        pending_ack_timer_->cancel();
        pending_ack_timer_.reset();
        IpcServer::ClearPendingAck(session_guid_);
    }
    assigned_instance_id_      = 0;
    pending_match_instance_id_ = 0;
    pending_match_game_mode_.clear();

    if (tryContinue) {
        int tf = 1;
        auto cached = MatchmakingService::ConsumePreAssignedTeam(
            successor->instance_id, session_guid_);
        if (cached) {
            tf = *cached;
            Logger::Log("tcp",
                "[TcpSession] Continue-in-queue: pre-assigned tf=%d for %s on successor=%lld\n",
                tf, session_guid_.c_str(), (long long)successor->instance_id);
        } else {
            auto queue_cfg = MatchmakingService::GetQueueConfig(parent->queue_id);
            if (queue_cfg) {
                switch (queue_cfg->taskforce_policy) {
                    case TaskforcePolicy::Pinned1: tf = 1; break;
                    case TaskforcePolicy::Pinned2: tf = 2; break;
                    case TaskforcePolicy::Balanced: {
                        auto counts = InstanceRegistry::GetTeamCounts(successor->instance_id);
                        tf = (counts.first <= counts.second) ? 1 : 2;
                        break;
                    }
                    case TaskforcePolicy::BalancedPvp: {
                        auto roster =
                            InstanceRegistry::GetActivePlayersForInstance(successor->instance_id);
                        RoleWeightedSplit::TeamState t1, t2;
                        for (const auto& r : roster) {
                            const float v = RoleWeightedSplit::HealValue(r.profile_id, r.task_force);
                            if (r.task_force == 1) { t1.heal_score += v; t1.size += 1; }
                            else                   { t2.heal_score += v; t2.size += 1; }
                        }
                        tf = RoleWeightedSplit::PlaceSingle(selected_profile_id_, t1, t2);
                        break;
                    }
                }
            }
            Logger::Log("tcp",
                "[TcpSession] Continue-in-queue: cache miss for %s on successor=%lld -- policy fallback tf=%d\n",
                session_guid_.c_str(), (long long)successor->instance_id, tf);
        }

        Logger::Log("tcp",
            "[TcpSession] Mission exit: routing %s to successor %lld of parent=%lld queue=%u tf=%d reason=%s\n",
            session_guid_.c_str(), (long long)successor->instance_id,
            (long long)parent_instance_id, parent->queue_id, tf,
            reason ? reason : "unspecified");
        initiate_player_register_for_target(*successor, tf);
        return;
    }

    Logger::Log("tcp",
        "[TcpSession] Mission exit: routing %s to home from parent=%lld reason=%s\n",
        session_guid_.c_str(), (long long)parent_instance_id,
        reason ? reason : "unspecified");
    wait_for_home_map_then_register(120);
}

void TcpSession::initiate_player_register_and_go_play() {
    // Build PLAYER_REGISTER JSON with full character state.
    auto session = PlayerSessionStore::GetByGuid(session_guid_);
    if (!session) {
        Logger::Log("tcp", "[TcpSession] Cannot send PLAYER_REGISTER: session %s not found\n",
            session_guid_.c_str());
        return;
    }

    auto home_instance = InstanceRegistry::GetReadyHomeInstance();
    if (!home_instance) {
        Logger::Log("tcp", "[TcpSession] No READY home map instance -- cannot send PLAYER_REGISTER for %s\n",
            session_guid_.c_str());
        return;
    }

    const int task_force =
        ChooseVrHomeTaskForce(*home_instance, selected_profile_id_, session_guid_);
    home_task_force_ = task_force;
    const uint64_t register_token = next_register_token_++;
    active_register_token_ = register_token;

    nlohmann::json reg;
    reg["type"]         = IpcProtocol::MSG_PLAYER_REGISTER;
    reg["session_guid"] = session_guid_;
    reg["register_token"] = register_token;
    reg["profile_id"]   = selected_profile_id_;
    reg["player_name"]  = player_name;
    reg["user_id"]      = user_id_;
    reg["character_id"] = selected_character_id_;
    reg["task_force"]   = task_force;

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
        const int item_profile_id =
            PlayerSessionStore::GetCurrentItemProfile(selected_character_id_);
        this->item_profile_id_ = item_profile_id;
        reg["current_item_profile_id"] = item_profile_id;
        nlohmann::json skillsArr = nlohmann::json::array();
        for (const auto& s : PlayerSessionStore::GetSkillsForCharacter(
                 selected_character_id_, item_profile_id)) {
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
        [this, self, timer, home_instance_id = home_instance->instance_id, task_force](
            bool success, int pawn_id) {
            timer->cancel();
            if (success) {
                Logger::Log("tcp", "[TcpSession] PLAYER_REGISTER ACK success for %s pawn_id=%d\n",
                    session_guid_.c_str(), pawn_id);
                // Track player in home map instance
                auto home = InstanceRegistry::GetReadyHomeInstance();
                if (home && home->instance_id == home_instance_id) {
                    InstanceRegistry::InsertInstancePlayer(
                        home->instance_id, session_guid_, selected_character_id_, task_force,
                        selected_profile_id_);
                }
                send_go_play_response();
            } else {
                Logger::Log("tcp", "[TcpSession] PLAYER_REGISTER ACK failed for %s\n",
                    session_guid_.c_str());
                // Silent: do not send go_play, do not send error to client
            }
        });

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
    const uint64_t register_token = next_register_token_++;
    active_register_token_ = register_token;
    reg["register_token"] = register_token;
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
        const int item_profile_id =
            PlayerSessionStore::GetCurrentItemProfile(selected_character_id_);
        this->item_profile_id_ = item_profile_id;
        reg["current_item_profile_id"] = item_profile_id;
        nlohmann::json skillsArr = nlohmann::json::array();
        for (const auto& s : PlayerSessionStore::GetSkillsForCharacter(
                 selected_character_id_, item_profile_id)) {
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
			std::error_code endpoint_ec;
			auto remote_endpoint = socket_.remote_endpoint(endpoint_ec);
			std::string remote_ip;
			if (!endpoint_ec) {
				remote_ip = remote_endpoint.address().to_string();
				ip_address_ = remote_ip + ":" + std::to_string(remote_endpoint.port());
			} else {
				ip_address_ = "<unknown>";
			}

			if (IsLoginIpCoolingDown(remote_ip)) {
				Logger::Log("tcp",
					"[%s] GSC_USER_LOGIN ignored during failed-login cooldown ip=%s\n",
					Logger::GetTime(), remote_ip.c_str());
				break;
			}

			// IP ban check — runs BEFORE username parsing so banned-by-IP
			// sources don't even get the transient pre-login response.
			if (auto ip_ban = Database::FindActiveBanForIp(remote_ip)) {
				Database::InsertSession(0, /*username yet unknown*/"", remote_ip, "banned");
				Logger::Log("ban", "[ban] ip-match ip=%s reason=\"%s\" mode=%s\n",
					remote_ip.c_str(), ip_ban->reason.c_str(), s_ban_spoof_mode_.c_str());
				ApplyBanSpoof();
				break;
			}

			PacketView pkt(data + 6, length - 6);
			auto read_login_name = [&pkt](uint16_t type) -> std::string {
				auto field = pkt.ReadString(type);
				return field ? TrimAsciiWhitespace(*field) : std::string();
			};
			std::string requested_user_name = read_login_name(GA_T::USER_NAME);
			const char* login_field_name = "USER_NAME";
			if (requested_user_name.empty()) {
				requested_user_name = read_login_name(GA_T::PLAYER_NAME);
				login_field_name = "PLAYER_NAME";
			}
			if (requested_user_name.empty()) {
				requested_user_name = read_login_name(GA_T::DATABASE_LOGIN);
				login_field_name = "DATABASE_LOGIN";
			}
			if (requested_user_name.empty()) {
				requested_user_name = read_login_name(GA_T::HOST_LOGIN);
				login_field_name = "HOST_LOGIN";
			}
			if (requested_user_name.empty()) {
				requested_user_name = read_login_name(GA_T::HOST_SQL_LOGIN);
				login_field_name = "HOST_SQL_LOGIN";
			}
			const bool has_user_name = !requested_user_name.empty();

			if (!has_user_name) {
				Logger::Log("tcp",
					"[%s] GSC_USER_LOGIN pre-login frame with no account fields; transient response only ip=%s item_count=%u len=%zu\n",
					Logger::GetTime(), ip_address_.c_str(), item_count, length);
				// Remember the issued nonce: the client encrypts its credential
				// blob against it, and the next (credentialed) frame on this
				// connection verifies against it.
				pending_login_challenge_guid_ = GenerateSessionGuid();
				send_login_response_for("", pending_login_challenge_guid_);
				break;
			}
			if (EqualsIgnoreAsciiCase(requested_user_name, "unknown")) {
				Logger::Log("tcp",
					"[%s] GSC_USER_LOGIN reserved username rejected ip=%s\n",
					Logger::GetTime(), ip_address_.c_str());
				Database::InsertSession(0, requested_user_name, remote_ip, "rejected");
				MarkFailedLoginIp(remote_ip);
				close_after_login_rejection();
				break;
			}
			if (!IsPrintableAsciiUsername(requested_user_name)) {
				std::string hex_dump;
				hex_dump.reserve(requested_user_name.size() * 2);
				static const char kHex[] = "0123456789ABCDEF";
				for (unsigned char c : requested_user_name) {
					hex_dump.push_back(kHex[c >> 4]);
					hex_dump.push_back(kHex[c & 0x0F]);
				}
				Logger::Log("tcp",
					"[%s] GSC_USER_LOGIN invalid characters in account name rejected ip=%s hex=%s\n",
					Logger::GetTime(), ip_address_.c_str(), hex_dump.c_str());
				Database::InsertSession(0, requested_user_name, remote_ip, "rejected");
				MarkFailedLoginIp(remote_ip);
				send_login_rejected_response("Invalid characters in account name.");
				close_after_login_rejection();
				break;
			}

			if (!session_guid_.empty()) {
				Logger::Log("tcp",
					"[%s] GSC_USER_LOGIN replacing prior session guid=%s name=%s\n",
					Logger::GetTime(), session_guid_.c_str(), player_name.c_str());
				if (session_row_id_) {
					Database::FinalizeSession(session_row_id_);
					session_row_id_ = 0;
				}
				MatchmakingService::RemovePlayer(session_guid_);
				// Same-socket relogin means the player went back to the login
				// screen voluntarily — leave the team, don't park offline.
				TeamService::HandleVoluntaryExit(session_guid_);
				UnregisterSession(session_guid_);
				IpcServer::ClearPendingAck(session_guid_);
				PlayerSessionStore::Unregister(session_guid_);
				session_guid_.clear();
				user_id_ = 0;
				selected_character_id_ = 0;
				selected_profile_id_ = 0;
				item_profile_id_ = 0;
				assigned_instance_id_ = 0;
				current_match_queue_id_ = 0;
				pending_match_instance_id_ = 0;
				pending_match_game_mode_.clear();
				pending_match_task_force_ = 1;
			}

			if (!s_allow_duplicate_account_logins_) {
				auto existing = PlayerSessionStore::GetByPlayerName(requested_user_name);
				if (existing) {
					Logger::Log("tcp",
						"[%s] GSC_USER_LOGIN duplicate account rejected name=%s existing_guid=%s existing_ip=%s new_ip=%s\n",
						Logger::GetTime(), requested_user_name.c_str(),
						existing->session_guid.c_str(), existing->ip_address.c_str(), ip_address_.c_str());
					Database::InsertSession(0, requested_user_name, remote_ip, "rejected");
					MarkFailedLoginIp(remote_ip);
					send_login_rejected_response("This account is already logged in.");
					close_after_login_rejection();
					break;
				}
			}

			// ---- Password verification (challenge-response) -------------------
			// The client encrypted a credential blob against the SESSION_GUID
			// nonce we issued in the pre-login frame. Recover the per-account
			// verifier (SHA256 of the RC4 keystream) and compare. A brand-new
			// account — or a legacy account with no verifier yet — registers it
			// (trust-on-first-use). See
			// .planning/2026-06-08-password-verification-design.md.
			bool store_new_verifier = false;
			std::vector<uint8_t> new_verifier;
			if (s_require_password_verification_ &&
			    (login_field_name == std::string("USER_NAME") ||
			     login_field_name == std::string("PLAYER_NAME"))) {
				const std::string challenge = pending_login_challenge_guid_;
				pending_login_challenge_guid_.clear();  // one-shot: no replay

				// DYNAMIC_UINT32_SIZE fields come back with a 4B length prefix;
				// strip it to get the raw RC4 ciphertext (same as the DWORDS
				// morph-blob handler in ADD_PLAYER_CHARACTER).
				auto blobRaw = pkt.ReadNBytes(GA_T::BIN_BLOB);
				std::vector<uint8_t> blob;
				if (blobRaw && blobRaw->size() > 4)
					blob.assign(blobRaw->begin() + 4, blobRaw->end());

				std::vector<uint8_t> computed;
				if (!blob.empty() && !challenge.empty())
					computed = LoginAuth::ComputeVerifier(challenge, requested_user_name, blob);

				PlayerSessionStore::UserAuth auth =
					PlayerSessionStore::GetUserAuth(requested_user_name);

				if (auth.exists && !auth.verifier.empty()) {
					// Registered account — the blob MUST verify.
					if (computed.empty() || computed != auth.verifier) {
						Logger::Log("tcp",
							"[%s] GSC_USER_LOGIN incorrect password name=%s ip=%s blob=%s challenge=%s\n",
							Logger::GetTime(), requested_user_name.c_str(), ip_address_.c_str(),
							blob.empty() ? "missing" : "present", challenge.empty() ? "missing" : "present");
						Database::InsertSession(auth.user_id, requested_user_name, remote_ip, "rejected");
						// Unlike bans (which stay silent so the player just sees a
						// hang), a wrong password gets a real error the client can
						// display. 18760 = the client's "incorrect password" msg id.
						//
						// Do NOT close the socket here and do NOT trip the failed-
						// login IP cooldown: closing immediately after the write
						// races the response into a TCP RST (the client never parses
						// it → spinner hangs), and the cooldown would block the
						// retry. Leaving the connection open lets the client show
						// the error and re-attempt on the same socket, exactly like
						// the normal success response path.
						send_login_rejected_response("Incorrect password.", 18760);
						break;
					}
				} else if (!computed.empty()) {
					// New account, or legacy row with no verifier — capture it.
					store_new_verifier = true;
					new_verifier = std::move(computed);
				}
				// else: new/legacy account with no usable blob (non-standard
				// login path) — allow through; there is nothing to verify yet.
			}

			player_name = requested_user_name;
			const int64_t resolved_user_id = PlayerSessionStore::UpsertUser(player_name);

			if (store_new_verifier) {
				PlayerSessionStore::SetUserVerifier(resolved_user_id, new_verifier,
				                                    (int64_t)std::time(nullptr));
				Logger::Log("tcp",
					"[%s] GSC_USER_LOGIN registered password verifier name=%s user_id=%lld\n",
					Logger::GetTime(), player_name.c_str(), (long long)resolved_user_id);
			}

			// Account ban check — runs after username validation/dedup so a
			// banned user typing a different name doesn't accidentally match.
			if (auto user_ban = Database::FindActiveBanForUser(resolved_user_id)) {
				Database::InsertSession(resolved_user_id, player_name, remote_ip, "banned");
				Logger::Log("ban", "[ban] user-match user_id=%lld name=%s ip=%s reason=\"%s\" mode=%s\n",
					(long long)resolved_user_id, player_name.c_str(), remote_ip.c_str(),
					user_ban->reason.c_str(), s_ban_spoof_mode_.c_str());
				ApplyBanSpoof();
				break;
			}

			session_guid_ = GenerateSessionGuid();
			RegisterSession(session_guid_, shared_from_this());
			user_id_ = resolved_user_id;
			session_row_id_   = Database::InsertSession(user_id_, player_name, remote_ip, "ok");
			session_login_at_ = (int64_t)std::time(nullptr);

			// Strip any inventory rows / equip references for items the
			// operator has marked as crash-on-equip in ga_item_blacklist.
			// Runs before send_login_response so the client never sees the
			// bad item in any subsequent SEND_INVENTORY / character data.
			PlayerSessionStore::PurgeBlacklistedItems(user_id_);

			{
				SessionInfo info;
				info.session_guid = session_guid_;
				info.player_name = player_name;
				info.ip_address = ip_address_;
				info.user_id = user_id_;
				PlayerSessionStore::Register(info);
			}

			Logger::Log("tcp", "[%s] Received: GSC_USER_LOGIN [0x%04X], name: %s field=%s guid: %s ip: %s\n",
			   Logger::GetTime(), packet_type, player_name.c_str(), login_field_name, session_guid_.c_str(), ip_address_.c_str());

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
					// keep their stable `inventory_id`. Gameplay devices stay
					// bag-only until the player equips them; stable IDs make the
					// equip screen safe between sessions.
					PlayerSessionStore::SeedInventoryFromLoadouts(charInfo->user_id);
					// HUMAN BASE ATTRIBUTES (slot 14) is server-pinned — the
					// player can't equip it via the UI, so the row must exist
					// in ga_character_devices before SEND_INVENTORY ships or
					// the client hits the resubmit loop on the REST device.
					// INSERT OR IGNORE per loadout profile — defensive against
					// existing characters whose slot 14 was never written.
					PlayerSessionStore::PinClassDeviceSlot14(selected_character_id_);
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
			// Quitting the game is a voluntary exit — leave the team now
			// instead of lingering as an offline member.
			if (!session_guid_.empty())
				TeamService::HandleVoluntaryExit(session_guid_);
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

			// Disconnect button = voluntary exit → leave the team (a socket
			// drop keeps offline-marked membership instead).
			if (!session_guid_.empty())
				TeamService::HandleVoluntaryExit(session_guid_);

			if (assigned_instance_id_ != 0 && !session_guid_.empty()) {
				nlohmann::json leave;
				leave["type"]         = IpcProtocol::MSG_PLAYER_LEAVE;
				leave["session_guid"] = session_guid_;
				leave["register_token"] = active_register_token_;
				bool ok = IpcServer::SendToInstance(assigned_instance_id_, leave.dump());
				Logger::Log("tcp", "[TcpSession] PLAYER_LEAVE dispatched guid=%s instance=%lld token=%llu send=%d\n",
					session_guid_.c_str(), (long long)assigned_instance_id_,
					(unsigned long long)active_register_token_, (int)ok);
			}

			// Drop the instance assignment so a subsequent SELECT_CHARACTER goes
			// through wait_for_home_map_then_register again from a clean slate.
			assigned_instance_id_      = 0;
			active_register_token_     = 0;
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
			if (parent && parent->is_home_map && parent->state == "READY") {
				pending_match_instance_id_ = 0;
				pending_match_game_mode_.clear();

				if (parent->map_name == "Dome3_VR_Arena_P") {
					nlohmann::json action;
					action["type"] = IpcProtocol::MSG_PLAYER_ACTION;
					action["session_guid"] = session_guid_;
					action["action"] = "return_home_area";

					bool ok = IpcServer::SendToInstance(parent_instance_id, action.dump());
					Logger::Log("tcp",
						"[TcpSession] GSC_CHANGE_INSTANCE: same-home VR return action instance=%lld send=%d for %s\n",
						(long long)parent_instance_id, (int)ok, session_guid_.c_str());
					break;
				}

				Logger::Log("tcp",
					"[TcpSession] GSC_CHANGE_INSTANCE: home map %s has no same-home return handler; keeping connection for %s\n",
					parent->map_name.c_str(), session_guid_.c_str());
				break;
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
			// We still mark the control-server roster row left before routing,
			// so empty mission instances can be stopped without waiting for
			// the old NetConnection timeout. We just avoid calling the unsafe
			// in-engine cleanup path on a live connection.
			//
			// TODO(crash-debug): find what's null inside CallOriginal at
			// NetConnection__Cleanup.cpp:84 when called on a live connection,
			// then re-enable an immediate teardown here.
			route_from_mission_instance(parent_instance_id, "client change-instance");
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
			// CharacterInfoStruct also carries hair / skin / eye selections
			// from the head menu (TgDataInterface.uc + TgLoginHUD.uc:128).
			// Previously discarded → server hardcoded HAIR_ASM_ID=0x85D in
			// the character-list response, which is incompatible with most
			// non-default heads and crashes the client during pawn-asm
			// attach. Persist what the player actually picked.
			// 1974 = NewHair15 (asm_mesh_type_value_id=850, the in-game
			// pawn hair category) — same asm SpawnBotPawn sets on bots.
			// Asm 0 doesn't exist; asm 403 PC_CHARBUILD_Bald is in the
			// character-builder UI category (type 596) and isn't
			// loadable from the in-game pawn-attach pipeline — both
			// crash the client at 0x109d1f5b.
			uint32_t hairAsmId          = pkt.Read4B(GA_T::HAIR_ASM_ID).value_or(1974);
			uint32_t skinMatParamId     = pkt.Read4B(GA_T::SKIN_MATERIAL_PARAMETER_ID).value_or(0);
			uint32_t eyeMatParamId      = pkt.Read4B(GA_T::EYE_MATERIAL_PARAMETER_ID).value_or(0);

			// DWORDS payload: 4B length prefix + raw morph bytes — strip prefix before storing.
			std::vector<uint8_t> morphBlob;
			auto dwordsRaw = pkt.ReadNBytes(GA_T::DWORDS);
			if (dwordsRaw && dwordsRaw->size() > 4)
				morphBlob.assign(dwordsRaw->begin() + 4, dwordsRaw->end());

			selected_character_id_ = PlayerSessionStore::InsertCharacter(
				user_id_, profileId, headAsmId, genderTypeId, morphBlob,
				hairAsmId, skinMatParamId, eyeMatParamId);
			if (selected_character_id_ == 0) {
				selected_profile_id_ = 0;
				Logger::Log("tcp", "[%s] ADD_PLAYER_CHARACTER failed: profile=%u user=%lld -- not registering player\n",
					Logger::GetTime(), profileId, (long long)user_id_);
				break;
			}
			selected_profile_id_   = profileId;
			PlayerSessionStore::SetSelectedCharacter(session_guid_, selected_character_id_, selected_profile_id_);

			Logger::Log("tcp", "[%s] ADD_PLAYER_CHARACTER: profile=%u head=%u gender=%u hair=%u skin=%u eye=%u morphBytes=%u charId=%lld\n",
				Logger::GetTime(), profileId, headAsmId, genderTypeId,
				hairAsmId, skinMatParamId, eyeMatParamId,
				(unsigned)morphBlob.size(), selected_character_id_);

			send_add_player_character_response();
			// Pair the ADD with an implicit SELECT so the client opens its
			// chat connection. CGameClient__HandleSelectCharacterResponseInternal
			// is the only response handler in the binary that reads CHAT_NET_ADDR
			// and sends the SESSION_GUID handshake — without this, new-player
			// chat sockets never open. selected_character_id_/profile_id_ are
			// already set from InsertCharacter above, so the response auto-
			// targets the freshly-created character.
			send_select_character_response();
			// Both tutorial and normal paths use wait-for-READY then PLAYER_REGISTER flow.
			wait_for_home_map_then_register(120);  // 120 second timeout
			break;
		}
		case GA_U::DELETE_CHARACTER: {
			Logger::Log("tcp", "[%s] Received: DELETE_CHARACTER [0x%04X], item count: %d\n", Logger::GetTime(), packet_type, item_count);
			PacketView pkt(data + 6, length - 6);
			uint32_t charId = pkt.Read4B(GA_T::CHARACTER_ID).value_or(0);

			const bool ok = (charId != 0) &&
				PlayerSessionStore::DeleteCharacter(static_cast<int64_t>(charId), user_id_);

			// If the deleted character was the active selection, clear it so a
			// later SELECT/register can't target a now-gone row.
			if (ok && selected_character_id_ == static_cast<int64_t>(charId)) {
				selected_character_id_ = 0;
				selected_profile_id_ = 0;
			}

			Logger::Log("tcp", "[%s] DELETE_CHARACTER: charId=%u user=%lld ok=%d\n",
				Logger::GetTime(), charId, (long long)user_id_, (int)ok);

			send_delete_character_response(charId, ok ? 0 : 1);
			send_character_list_response();
			break;
		}
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

			// Teamed: only the leader manages the queue, and joining queues the
			// WHOLE team as one party.
			if (TeamService::IsTeamed(session_guid_)) {
				if (!TeamService::IsLeader(session_guid_)) {
					send_team_system_message(TeamService::MSG_QUEUE_LEADER_ONLY, "");
					break;
				}
				auto party = TeamService::BuildParty(session_guid_);
				if (!party) break;
				auto cfg = MatchmakingService::GetQueueConfig(matchQueueId);
				if (cfg && cfg->team_policy == TeamPolicy::Block) {
					Logger::Log("tcp", "[TcpSession] MATCH_JOIN: queue %u blocks teams — leader %s\n",
						matchQueueId, session_guid_.c_str());
					break;
				}
				if (cfg && party->size() > mm::MaxTeamSize(*cfg)) {
					send_team_system_message(TeamService::MSG_TEAM_TOO_LARGE, "");
					break;
				}
				MatchmakingService::AddParty(matchQueueId, *party);
				TeamService::NotifyTeamQueued(party->party_id, matchQueueId,
					cfg ? cfg->name_msg_id : 0);
				break;
			}

			send_match_join_response(matchQueueId, matchFilters);
			break;
		}
		case GA_U::MATCH_LEAVE: {
			Logger::Log("tcp", "[%s] Received: MATCH_LEAVE [0x%04X], item count: %d\n", Logger::GetTime(), packet_type, item_count);

			// Teamed: only the leader can pull the team out of queue. The
			// dequeue sends "you left the match queue" + clears every member's
			// HUD via the composition-change path.
			if (TeamService::IsTeamed(session_guid_)) {
				if (!TeamService::IsLeader(session_guid_)) {
					send_team_system_message(TeamService::MSG_QUEUE_LEADER_ONLY, "");
					break;
				}
				TeamService::DequeueForCompositionChange(session_guid_);
				break;
			}

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
		case GA_U::QUERY_PLAYERS: {
			Logger::Log("tcp", "[%s] Received: QUERY_PLAYERS [0x%04X], item count: %d\n", Logger::GetTime(), packet_type, item_count);
			PacketView pkt(data + 6, length - 6);
			send_query_players_response(pkt);
			break;
		}
		case GA_U::TEAM_INVITE: {
			Logger::Log("tcp", "[%s] Received: TEAM_INVITE [0x%04X], item count: %d\n", Logger::GetTime(), packet_type, item_count);
			PacketView pkt(data + 6, length - 6);
			std::string target = pkt.ReadString(GA_T::PLAYER_NAME).value_or("");
			if (target.empty() || session_guid_.empty()) {
				Logger::Log("tcp", "[TCP] TEAM_INVITE dropped (target='%s' guid='%s')\n",
					target.c_str(), session_guid_.c_str());
				break;
			}
			TeamService::RequestInvite(session_guid_, player_name, target);
			break;
		}
		case GA_U::TEAM_ACCEPT:
			// Bare frame — the invitee accepted the invitation popup.
			Logger::Log("tcp", "[%s] Received: TEAM_ACCEPT [0x%04X]\n", Logger::GetTime(), packet_type);
			if (!session_guid_.empty())
				TeamService::AcceptInvite(session_guid_);
			break;
		case GA_U::TEAM_LEAVE:
			// Bare frame — popup decline OR leave-team; TeamService routes.
			Logger::Log("tcp", "[%s] Received: TEAM_LEAVE [0x%04X]\n", Logger::GetTime(), packet_type);
			if (!session_guid_.empty())
				TeamService::HandleLeave(session_guid_);
			break;
		case GA_U::TEAM_KICK: {
			// C→S: team leader removes a member from the team.
			// Payload: CHARACTER_ID of the member to kick.
			Logger::Log("tcp", "[%s] Received: TEAM_KICK [0x%04X]\n", Logger::GetTime(), packet_type);
			PacketView pkt(data + 6, length - 6);
			int64_t target_char_id = (int64_t)pkt.Read4B(GA_T::CHARACTER_ID).value_or(0);
			if (!session_guid_.empty() && target_char_id != 0)
				TeamService::KickMember(session_guid_, target_char_id);
			break;
		}
		case GA_U::GROUP_SET_LEADER: {
			// C→S: current leader promotes a team member to leader.
			// Payload: CHARACTER_ID of the member to promote.
			Logger::Log("tcp", "[%s] Received: GROUP_SET_LEADER [0x%04X]\n", Logger::GetTime(), packet_type);
			PacketView pkt(data + 6, length - 6);
			int64_t target_char_id = (int64_t)pkt.Read4B(GA_T::CHARACTER_ID).value_or(0);
			if (!session_guid_.empty() && target_char_id != 0)
				TeamService::PromoteLeader(session_guid_, target_char_id);
			break;
		}
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

			PlayerSessionStore::SetSkillsForCharacter(
				selected_character_id_, (int)item_profile_id_, skills);

			// Echo back so the UI updates; also refreshes LAST_RESPEC_DATETIME client-side.
			send_player_skills_response();
			break;
		}
		default:
			// Challenge-system probe: if the retail client has a NATIVE challenge
			// send path (e.g. the stubbed TgPlayerController.TeamChallenge exec, or
			// a UI button), challenging a player should produce one of these on the
			// game-TCP connection. Seeing 0x01F3 here means we should HANDLE the
			// client's request rather than parse `/challenge` chat text; the raw
			// LogData below then hands us the on-wire payload format for free.
			if (packet_type == GA_U::CHALLENGE_REQUEST ||
			    packet_type == GA_U::CHALLENGE_INVITES ||
			    packet_type == GA_U::CHALLENGE_EXPIRED_INVITE) {
				Logger::Log("challenge",
					"[Challenge] INBOUND challenge-family packet 0x%04X player='%s' guid=%s item_count=%d "
					"-- client HAS a native challenge send path; raw bytes follow on 'tcp'\n",
					packet_type, player_name.c_str(), session_guid_.c_str(), item_count);
			}
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
    player.user_id = user_id_;  // drives the queue's requires_pvp_verification gate
    player.joined_at = std::chrono::steady_clock::now();

    MatchmakingService::AddPlayer(matchQueueId, player);

    // Drive the AgentInfo HUD "IN QUEUE: <name>" panel + leave-queue keybind.
    auto cfg = MatchmakingService::GetQueueConfig(matchQueueId);
    send_match_queue_status(matchQueueId, cfg ? cfg->name_msg_id : 0);

    // Chat confirmation via the vt[+0x54] display route (opcode-agnostic).
    send_team_system_message(18306, "");  // "You have joined a match queue."
}

void TcpSession::send_match_leave_response() {
    const bool was_queued = current_match_queue_id_ != 0;
    MatchmakingService::RemovePlayer(session_guid_);
    send_match_queue_status(0, 0);  // hide the HUD queue panel
    if (was_queued)
        send_team_system_message(18308, "");  // "You have left the match queue"
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
    // 0xE4 routes through the SAME vt[+0x6c] handler as MATCH_JOIN before
    // showing the popup, and the client's get_int32_t ZEROES the stored
    // queue-state fields when absent — a bare frame wipes the HUD's
    // "IN QUEUE" state the instant the queue pops. Carry the fields.
    uint16_t item_count = 3;

    append(response, packet_type & 0xFF, packet_type >> 8);
    append(response, item_count & 0xFF, item_count >> 8);

    auto cfg = MatchmakingService::GetQueueConfig(current_match_queue_id_);
    Write4B(response, GA_T::MATCH_QUEUE_ID, current_match_queue_id_);
    Write4B(response, GA_T::NAME_MSG_ID, cfg ? cfg->name_msg_id : 0);
    Write4B(response, GA_T::PENALTY_SECONDS, 0);

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

void TcpSession::send_team_system_message(uint32_t msg_id, const std::string& player_name) {
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::TEAM_INVITE;
	uint16_t item_count = player_name.empty() ? 1 : 2;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	Write4B(response, GA_T::MSG_ID, msg_id);
	if (!player_name.empty())
		WriteString(response, GA_T::PLAYER_NAME, player_name);

	send_response(response);
}

void TcpSession::send_team_invitation_popup(const std::string& leader_name) {
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::TEAM_INVITATION;
	uint16_t item_count = 2;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	Write4B(response, GA_T::MSG_ID, TeamService::MSG_INVITATION_POPUP);
	WriteString(response, GA_T::LEADER_NAME, leader_name);

	send_response(response);
}

void TcpSession::send_match_queue_status(uint32_t queue_id, uint32_t name_msg_id,
                                         uint32_t penalty_seconds) {
	// S→C MATCH_JOIN (0xE1) → CGameClient vt[+0x6c] @ 0x1091fe40 stores
	// queue id / name msg id / deserter penalty at CGameClient+0xb0..0xc0;
	// TgUIPrimaryHUD_AgentInfo::UpdateQueueValues (0x114d6ae0) renders them
	// as "IN QUEUE: <name>" + leave-queue keybind (or DESERTER countdown).
	// All fields written explicitly — the client stores into persistent
	// fields, so absent fields would keep stale values. queue_id=0 hides.
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::MATCH_JOIN;
	uint16_t item_count = 3;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	Write4B(response, GA_T::MATCH_QUEUE_ID, queue_id);
	Write4B(response, GA_T::NAME_MSG_ID, name_msg_id);
	// Write4B(response, GA_T::STATUS_CODE, 608);
	Write4B(response, GA_T::PENALTY_SECONDS, penalty_seconds);

	Logger::Log("tcp", "[TcpSession] send_match_queue_status guid=%s queue=%u nameMsg=%u penalty=%u\n",
		session_guid_.c_str(), queue_id, name_msg_id, penalty_seconds);

	send_response(response);
}

void TcpSession::send_team_invite_expired() {
	std::vector<uint8_t> response;

	// The client's 0x4C handler reads no fields — it just drops the queued
	// team-invitation node and dismisses the popup.
	uint16_t packet_type = GA_U::TEAM_EXPIRED_INVITE;
	uint16_t item_count = 0;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	send_response(response);
}

void TcpSession::send_team_update(const TeamRoster& roster) {
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::TEAM_UPDATE;
	uint16_t item_count = 6;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	// Do NOT send SETTING_FLAGS (0x0474) — its presence flips the client
	// parser into the settings-only branch and the roster is ignored.
	Write4B(response, GA_T::TEAM_MODE, roster.team_mode);
	Write4B(response, GA_T::TEAM_ID, (uint32_t)roster.team_id);
	Write4B(response, GA_T::OWNER_AGENCY_ID, 0);
	Write4B(response, GA_T::OWNER_ALLIANCE_ID, 0);
	Write4B(response, GA_T::LEADER_CHARACTER_ID, (uint32_t)roster.leader_character_id);

	append(response, GA_T::DATA_SET & 0xFF, GA_T::DATA_SET >> 8);
	append(response, roster.rows.size() & 0xFF, (roster.rows.size() >> 8) & 0xFF);
	for (const auto& row : roster.rows) {
		append(response, 0x0B, 0x00);  // 11 fields per member
		Write4B(response, GA_T::CHARACTER_ID, (uint32_t)row.character_id);
		WriteString(response, GA_T::PLAYER_NAME, row.player_name);
		Write4B(response, GA_T::PROFILE_ID, row.profile_id);
		// 0xC2 is wire-string typed; the parser coerces digits via _wtoi.
		WriteString(response, GA_T::CLASS_MSG_ID, std::to_string(row.class_msg_id));
		Write4B(response, GA_T::STATUS_MSG_ID, row.status_msg_id);
		Write4B(response, GA_T::MAP_NAME_MSG_ID, row.map_name_msg_id);
		Write4B(response, GA_T::LEVEL, row.level);
		WriteFloat(response, GA_T::SKILL_RATING, row.skill_rating);
		Write1B(response, GA_T::OFFLINE_FLAG, row.offline ? 1 : 0);
		WriteString(response, GA_T::AGENCY_NAME, row.agency_name);
		WriteString(response, GA_T::ALLIANCE_NAME, row.alliance_name);
	}

	send_response(response);
}

void TcpSession::DeliverTeamUpdate(const std::string& session_guid,
                                   const TeamRoster& roster) {
	std::lock_guard<std::mutex> lock(sessions_mutex_);
	auto it = g_sessions_.find(session_guid);
	if (it == g_sessions_.end()) return;
	auto session = it->second.lock();
	if (!session) return;
	session->send_team_update(roster);
}

void TcpSession::DeliverTeamSystemMessage(const std::string& session_guid,
                                          uint32_t msg_id, const std::string& player_name) {
	std::lock_guard<std::mutex> lock(sessions_mutex_);
	auto it = g_sessions_.find(session_guid);
	if (it == g_sessions_.end()) return;
	auto session = it->second.lock();
	if (!session) return;
	session->send_team_system_message(msg_id, player_name);
}

void TcpSession::DeliverTeamInvitation(const std::string& session_guid,
                                       const std::string& leader_name) {
	std::lock_guard<std::mutex> lock(sessions_mutex_);
	auto it = g_sessions_.find(session_guid);
	if (it == g_sessions_.end()) return;
	auto session = it->second.lock();
	if (!session) return;
	session->send_team_invitation_popup(leader_name);
}

void TcpSession::DeliverTeamInviteExpired(const std::string& session_guid) {
	std::lock_guard<std::mutex> lock(sessions_mutex_);
	auto it = g_sessions_.find(session_guid);
	if (it == g_sessions_.end()) return;
	auto session = it->second.lock();
	if (!session) return;
	session->send_team_invite_expired();
}

void TcpSession::DeliverMatchQueueStatus(const std::string& session_guid,
                                         uint32_t queue_id, uint32_t name_msg_id) {
	std::lock_guard<std::mutex> lock(sessions_mutex_);
	auto it = g_sessions_.find(session_guid);
	if (it == g_sessions_.end()) return;
	auto session = it->second.lock();
	if (!session) return;
	session->current_match_queue_id_ = queue_id;
	session->send_match_queue_status(queue_id, name_msg_id);
}

void TcpSession::DeliverMatchCancelled(const std::string& session_guid, const char* reason) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    auto it = g_sessions_.find(session_guid);
    if (it == g_sessions_.end()) return;
    auto session = it->second.lock();
    if (!session) return;
    MatchmakingService::RemoveReadyMatchReservation(session_guid, reason);
    Logger::Log("tcp", "[TcpSession] Match cancelled for guid=%s (reason: %s) — clearing queue/match state\n",
        session_guid.c_str(), reason ? reason : "unspecified");
    session->current_match_queue_id_       = 0;
    session->pending_match_instance_id_    = 0;
    session->pending_match_game_mode_.clear();
    session->pending_match_task_force_     = 1;
    // GET_TICKET_INFO polling picks up CURRENT_MATCH_QUEUE_ID=0 for the
    // queue menu; the HUD panel needs the explicit clear frame.
    session->send_match_queue_status(0, 0);
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
        MatchmakingService::RemoveReadyMatchReservation(
            pending_match_instance_id_, session_guid_, "match accept target not ready");
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
        const int item_profile_id =
            PlayerSessionStore::GetCurrentItemProfile(selected_character_id_);
        this->item_profile_id_ = item_profile_id;
        reg["current_item_profile_id"] = item_profile_id;
        nlohmann::json skillsArr = nlohmann::json::array();
        for (const auto& s : PlayerSessionStore::GetSkillsForCharacter(
                 selected_character_id_, item_profile_id)) {
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
        MatchmakingService::RemoveReadyMatchReservation(
            target_instance_id, session_guid_, "match accept target has no IPC session");
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
                MatchmakingService::RemoveReadyMatchReservation(
                    target_instance_id, session_guid_, "player register ack failed");
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
            MatchmakingService::RemoveReadyMatchReservation(
                target_instance_id, session_guid_, "player registered in mission");

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
            send_match_queue_status(0, 0);  // entering the match — hide HUD queue panel
        });

    pending_ack_timer_->async_wait([this, self, timer](std::error_code ec) {
        if (!ec) {
            Logger::Log("tcp", "[TcpSession] PLAYER_REGISTER ACK timeout for %s on mission instance\n",
                session_guid_.c_str());
            IpcServer::ClearPendingAck(session_guid_);
            MatchmakingService::RemoveReadyMatchReservation(
                pending_match_instance_id_, session_guid_, "player register ack timeout");
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

	// Unteamed players queue for themselves; teamed non-leaders lose the
	// join button (only the leader queues the team).
	const bool can_queue = !TeamService::IsTeamed(session_guid_)
	                    || TeamService::IsLeader(session_guid_);
	Write1B(response, GA_T::TEAM_LEADER_FLAG, can_queue ? 0x1 : 0x0);
	Write4B(response, GA_T::CURRENT_MATCH_QUEUE_ID, current_match_queue_id_);

	append(response, GA_T::DATA_SET & 0xFF, GA_T::DATA_SET >> 8);
	append(response, (uint8_t)(queues.size() & 0xFF), (uint8_t)(queues.size() >> 8));

	for (const auto& cfg : queues) {
		TicketInfoEncoder::EncodeRecord(response, cfg, RuntimeStats::ForQueue(cfg.queue_id));
	}

	send_response(response);

	// ─── Reference templates for queue rows we haven't added yet. ────────────
	// Kept for documentation while implementing new queue types — copy the
	// magic numbers you need and seed them into ga_queues / ga_map_pool_entries.
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
	// `item_profile_id_` tracks the active loadout slot — drives which rows
	// land in the equipped pass (the rest go to the bag pass).
	auto devices = PlayerSessionStore::GetDevicesForCharacter(
		character_id, (int)item_profile_id_);

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

	// Bag pool first (we need its size to write the outer DATA_SET count
	// before any record bodies). Filter out anything already covered by the
	// equipped pass — same item shipped twice would be a wasted record.
	std::set<int> equipped_inv_ids;
	for (const auto& d : devices) equipped_inv_ids.insert(d.inventory_id);
	const auto bag_all = PlayerSessionStore::GetInventoryForUser(user_id, profile);
	std::vector<InventoryRow> bag;
	bag.reserve(bag_all.size());
	for (const auto& row : bag_all) {
		if (equipped_inv_ids.count(row.inventory_id)) continue;
		bag.push_back(row);
	}

	const size_t equipped_count = devices.size();
	const size_t bag_count      = bag.size();
	const size_t total_records  = equipped_count + bag_count;

	Logger::Log("tcp",
		"[TCP] send_inventory_response: charId=%lld pawnId=%d equipped=%zu bag=%zu (bundled into one SEND_INVENTORY)\n",
		character_id, nPawnId, equipped_count, bag_count);

	if (total_records == 0) return;

	std::vector<uint8_t> response;

	append(response, GA_U::SEND_INVENTORY & 0xFF, GA_U::SEND_INVENTORY >> 8);
	append(response, 0x02, 0x00);   // 2 top-level fields: PAWN_ID + DATA_SET

	Write4B(response, GA_T::PAWN_ID, nPawnId);

	append(response, GA_T::DATA_SET & 0xFF, GA_T::DATA_SET >> 8);
	append(response, (uint8_t)(total_records & 0xFF), (uint8_t)(total_records >> 8));

	// --- Equipped block. 14 fields per record (12 scalars + 2 nested data_sets).
	//
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
	// Armor rows have the same shape as cosmetics (item_id > 0, device_id =
	// 0, no backing actor) but DO carry mods — so they need a non-zero
	// BLUEPRINT_ID to make the client's FUN_10a13820 mod-application path
	// apply the rolled effects. Discriminator: armor has mods, cosmetics
	// don't. Universal blueprint works because the client only nullity-
	// checks the resolved blueprint pointer — the rendered `[…]` letters
	// come from each effect's `prop.ui_code`, not from blueprint metadata.
	//
	// Slot 14 RestDevice is not cosmetic. R self-heal needs DEVICE_ID to
	// point at the live ATgDevice inventory id; sending 0 makes R no-op.
	for (const auto& d : devices) {
		const bool itemRow       = d.item_id > 0;
		const bool isArmor       = itemRow && !d.mods.empty();
		const int marshalItemId  = itemRow ? d.item_id : d.device_id;
		const int marshalDeviceField = itemRow ? 0 : d.inventory_id;
		const int blueprintId    =
			isArmor                    ? ArmorLoadouts::kUniversalArmorBlueprintId :
			(itemRow || d.mods.empty()) ? 0
			                            : PlayerSessionStore::GetBlueprintIdForDevice(d.device_id, d.oc);

		append(response, 0x0E, 0x00);   // 14 fields

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

		// DATA_SET_INVENTORY_STATE: {INVENTORY_ID, EFFECT_GROUP_ID} pairs for
		// rolled mods only. Client FUN_10a13820 @ 0x10a13820 appends matching
		// rows into the inventory object's effect-group list, driving the
		// `[...]` suffix in the UI. Built-in equip / fire-mode effects are
		// NOT shipped here — they come from the client's own asm.dat (via
		// Device.m_EquipEffect / m_pFireModeSetup). Mixing them in leaks
		// built-in props into the suffix.
		std::vector<int> effectGroups = d.mods;
		if (effectGroups.empty()) effectGroups.push_back(0); // preserve 1-row shape

		append(response, GA_T::DATA_SET_INVENTORY_STATE & 0xFF, GA_T::DATA_SET_INVENTORY_STATE >> 8);
		append(response, (uint8_t)(effectGroups.size() & 0xFF), (uint8_t)(effectGroups.size() >> 8));
		for (int egid : effectGroups) {
			append(response, 0x02, 0x00);
			Write4B(response, GA_T::INVENTORY_ID, d.inventory_id);
			Write4B(response, GA_T::EFFECT_GROUP_ID, egid);
		}

		// DATA_SET_CHARACTER_PROFILES: one row per build profile (1..5) with
		// the same EQUIPPED_SLOT_VALUE_ID. Each row populates one of the five
		// `invObj+0x68..+0x78` slot-table entries on the client. The equip-
		// screen UI's FixupWidgets path requires EVERY profile entry to be
		// non-zero — if ANY profile has 0, the slot renders blank for all
		// profiles (see reference_equip_screen_is_valid_gate.md).
		append(response, GA_T::DATA_SET_CHARACTER_PROFILES & 0xFF, GA_T::DATA_SET_CHARACTER_PROFILES >> 8);
		append(response, 0x05, 0x00);
		for (int profileId = 1; profileId <= 5; ++profileId) {
			append(response, 0x04, 0x00);
			Write4B(response, GA_T::CHARACTER_ID, 0);
			Write4B(response, GA_T::INVENTORY_ID, d.inventory_id);
			Write4B(response, GA_T::PROFILE_ID, (uint32_t)profileId);
			Write4B(response, GA_T::EQUIPPED_SLOT_VALUE_ID, d.slot_value_id);
		}
	}

	// --- Bag block. 13 fields per record (12 scalars + 1 nested data_set —
	// no DATA_SET_CHARACTER_PROFILES because bag items aren't bound to a
	// slot). LOCATION_VALUE_ID=370 (ON_HAND), DEVICE_ID=0, ACTIVE_FLAG=F.
	// See top-of-function comment for why DEVICE_ID=0 is the don't-resubmit
	// gate.
	for (const auto& row : bag) {
		const bool   rowIsItem     = row.item_id > 0;
		const bool   rowIsArmor    = rowIsItem && !row.mods.empty();
		const int    marshalItemId = rowIsItem ? row.item_id : row.device_id;
		const int    blueprintId   =
			rowIsArmor                    ? ArmorLoadouts::kUniversalArmorBlueprintId :
			(rowIsItem || row.mods.empty()) ? 0
			                                : PlayerSessionStore::GetBlueprintIdForDevice(row.device_id, row.oc);

		append(response, 0x0D, 0x00);   // 13 fields

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
		WriteString(response, GA_T::ACTIVE_FLAG, "F");
		Write4B(response, GA_T::DEVICE_ID, 0);

		std::vector<int> effectGroups = row.mods;
		if (effectGroups.empty()) effectGroups.push_back(0);

		append(response, GA_T::DATA_SET_INVENTORY_STATE & 0xFF, GA_T::DATA_SET_INVENTORY_STATE >> 8);
		append(response, (uint8_t)(effectGroups.size() & 0xFF), (uint8_t)(effectGroups.size() >> 8));
		for (int egid : effectGroups) {
			append(response, 0x02, 0x00);
			Write4B(response, GA_T::INVENTORY_ID, row.inventory_id);
			Write4B(response, GA_T::EFFECT_GROUP_ID, egid);
		}
	}

	send_response(response);
}

void TcpSession::send_inventory_clear(int nPawnId, int64_t character_id) {
	// Resolve user + class profile so we hit the same pool send_inventory_response does.
	const auto charInfo = PlayerSessionStore::GetCharacterById(character_id);
	if (!charInfo) return;
	const auto bag = PlayerSessionStore::GetInventoryForUser(charInfo->user_id, charInfo->profile_id);
	if (bag.empty()) return;

	// One STATE=2 record per invId, all bundled in a single SEND_INVENTORY.
	// Per-record body is tiny (2 fields = 12 bytes) so the bundled payload
	// for a fully-stocked account fits well under any reasonable cap; the
	// chunked-marker framing in send_response handles overflow if it ever
	// matters.
	const size_t count = bag.size();

	std::vector<uint8_t> response;
	append(response, GA_U::SEND_INVENTORY & 0xFF, GA_U::SEND_INVENTORY >> 8);
	append(response, 0x02, 0x00);   // PAWN_ID + DATA_SET
	Write4B(response, GA_T::PAWN_ID, nPawnId);
	append(response, GA_T::DATA_SET & 0xFF, GA_T::DATA_SET >> 8);
	append(response, (uint8_t)(count & 0xFF), (uint8_t)(count >> 8));
	for (const auto& row : bag) {
		append(response, 0x02, 0x00);  // 2 fields per record
		Write4B(response, GA_T::INV_REPLICATION_STATE, 0x2);  // delete
		Write4B(response, GA_T::INVENTORY_ID, row.inventory_id);
	}
	send_response(response);

	Logger::Log("tcp",
		"[TCP] send_inventory_clear: charId=%lld pawnId=%d wiped=%zu entries (STATE=2 delete pass)\n",
		character_id, nPawnId, count);
}

void TcpSession::send_loadout_inventory_response(int nPawnId, const std::vector<LoadoutItem>& items) {
    if (items.empty()) {
        Logger::Log("tcp", "[TCP] send_loadout_inventory_response: empty loadout for pawnId=%d\n", nPawnId);
        return;
    }
    Logger::Log("tcp",
        "[TCP] send_loadout_inventory_response: %zu items for pawnId=%d (bundled into one SEND_INVENTORY)\n",
        items.size(), nPawnId);

    // Same per-record wire format as send_inventory_response, minus the DB
    // lookup. BLUEPRINT_ID is 0 (no rolled mods on bot devices, so no [...]
    // suffix needed). DATA_SET_INVENTORY_STATE carries a single placeholder
    // row with EFFECT_GROUP_ID=0 to preserve the 1-row shape — harmless when
    // BLUEPRINT_ID=0 (parser skips mod application).
    std::vector<uint8_t> response;

    append(response, GA_U::SEND_INVENTORY & 0xFF, GA_U::SEND_INVENTORY >> 8);
    append(response, 0x02, 0x00);   // 2 top-level fields: PAWN_ID + DATA_SET

    Write4B(response, GA_T::PAWN_ID, nPawnId);

    append(response, GA_T::DATA_SET & 0xFF, GA_T::DATA_SET >> 8);
    append(response, (uint8_t)(items.size() & 0xFF), (uint8_t)(items.size() >> 8));

    for (const auto& d : items) {
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
    }

    send_response(response);
}

void TcpSession::send_beacon_pickup_response(int nPawnId, int nDeviceId, int nInventoryId, int nEquipSlotValueId, int nItemProfileId) {
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

		// One row per profile (1..5) with the SAME EQUIPPED_SLOT_VALUE_ID —
		// the FixupWidgets gate (see send_inventory_response) requires all 5
		// non-zero or the slot widget renders blank. A picked-up beacon is
		// bound to the PAWN, not to a loadout profile — same SVID on every
		// profile is the desired pawn-scoped behavior anyway.
		(void)nItemProfileId;  // no longer needed — kept in signature for the IPC carry
		append(response, GA_T::DATA_SET_CHARACTER_PROFILES & 0xFF, GA_T::DATA_SET_CHARACTER_PROFILES >> 8);
		append(response, 0x05, 0x00);
			for (int p = 1; p <= 5; ++p) {
				append(response, 0x04, 0x00);
				Write4B(response, GA_T::CHARACTER_ID, 0);
				Write4B(response, GA_T::INVENTORY_ID, nInventoryId);
				Write4B(response, GA_T::PROFILE_ID, (uint32_t)p);
				Write4B(response, GA_T::EQUIPPED_SLOT_VALUE_ID, (uint32_t)nEquipSlotValueId);
			}

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
	// Ship EVERY profile's rows (each tagged with its own profile_id) so the
	// client's skill-tree widget can re-filter against r_nItemProfileId after
	// a profile switch without needing a fresh SEND_PLAYER_SKILLS. r_nItemProfileId
	// has no UC repnotify, so on switch the widget receives no callback —
	// previously the active-only ship meant the widget saw 0 rows for the new
	// profile until a close+reopen rebuilt its bindings.
	std::vector<SkillRow> skills;
	double respecUnix = 1.0;
	if (selected_character_id_ != 0) {
		skills = PlayerSessionStore::GetAllSkillsForCharacter(selected_character_id_);
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
		// Each row carries its OWN item_profile_id from the DB so all five
		// loadouts ship in one packet; the widget filters by active profile.
		append(response, 0x04, 0x00);
		Write4B(response, GA_T::SKILL_GROUP_ID,   (uint32_t)s.skill_group_id);
		Write4B(response, GA_T::SKILL_ID,         (uint32_t)s.skill_id);
		Write4B(response, GA_T::PROFILE_ID,       (uint32_t)s.item_profile_id);
		Write4B(response, GA_T::POINTS_ALLOCATED, (uint32_t)s.points);
	}

	Logger::Log("tcp", "[TCP] SEND_PLAYER_SKILLS: sent %d skills (all profiles) for charId=%lld active=%d\n",
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

void TcpSession::send_query_players_response(const PacketView& pkt)
{
	// CALLER_ID is the GObjObjects index of the client's TgDataSet; without
	// it MarshalReceived2 can't route the response (see player-search-tcp-protocol.md).
	const uint32_t caller_id = pkt.Read4B(GA_T::CALLER_ID).value_or(0);
	if (caller_id == 0) {
		Logger::Log("tcp", "[TCP] QUERY_PLAYERS without CALLER_ID, dropping\n");
		return;
	}

	// Filters — absent field = no constraint.
	auto name_filter     = pkt.ReadString(GA_T::PLAYER_NAME);
	auto agency_filter   = pkt.ReadString(GA_T::AGENCY_NAME);
	auto alliance_filter = pkt.ReadString(GA_T::ALLIANCE_NAME);
	auto level_min       = pkt.Read4B(GA_T::LEVEL_MIN);
	auto level_max       = pkt.Read4B(GA_T::LEVEL_MAX);
	auto profile_filter  = pkt.Read4B(GA_T::PROFILE_ID);
	auto team_filter     = pkt.Read1B(GA_T::TEAM_FLAG);
	auto mission_filter  = pkt.ReadString(GA_T::MISSION_FLAG);  // string flag "T"/"F"
	// SYS_AVA_TEAMS_OK (the Team menu's AvA checkbox) isn't tracked yet — ignored.

	auto to_lower = [](std::string s) {
		for (auto& c : s) c = (char)std::tolower((unsigned char)c);
		return s;
	};
	auto contains_ci = [&](const std::string& haystack, const std::string& needle) {
		return to_lower(haystack).find(to_lower(needle)) != std::string::npos;
	};

	constexpr uint32_t kLevel   = 0x32;  // levels aren't tracked; 50 everywhere (matches GSC_CHARACTER_LIST)
	// send_response chunks big payloads, so this is a UI sanity bound: the
	// Team menu list scrolls, the standalone search menu shows 16 (paging
	// natives are no-op stubs in this client build).
	constexpr size_t   kMaxRows = 50;

	const auto players = InstanceRegistry::GetActiveSearchablePlayers();

	std::vector<uint8_t> rows;
	size_t row_count = 0;
	for (const auto& p : players) {
		if (row_count >= kMaxRows) break;
		const bool teamed = TeamService::IsTeamed(p.session_guid);
		if (name_filter && !contains_ci(p.player_name, *name_filter)) continue;
		// No agency/alliance system yet — a non-empty filter matches nobody.
		if (agency_filter && !agency_filter->empty()) continue;
		if (alliance_filter && !alliance_filter->empty()) continue;
		if (level_min && kLevel < *level_min) continue;
		if (level_max && kLevel > *level_max) continue;
		if (profile_filter && p.profile_id != *profile_filter) continue;
		if (team_filter && teamed != (*team_filter != 0)) continue;
		if (mission_filter && p.in_mission != (*mission_filter == "T")) continue;

		uint32_t map_msg_id = 0;
		if (auto rec = MapGameInfo::LookupByName(p.map_name))
			map_msg_id = rec->friendly_name_msg_id;

		append(rows, 0x0A, 0x00);  // 10 fields per row
		Write4B(rows, GA_T::PLAYER_ID, (uint32_t)p.character_id);
		WriteString(rows, GA_T::PLAYER_NAME, p.player_name);
		WriteString(rows, GA_T::AGENCY_NAME, "");
		WriteString(rows, GA_T::ALLIANCE_NAME, "");
		Write4B(rows, GA_T::LEVEL, kLevel);
		Write4B(rows, GA_T::PROFILE_ID, p.profile_id);
		Write4B(rows, GA_T::MAP_NAME_MSG_ID, map_msg_id);
		// Always ship SKILL_RATING — the Team menu reads it into an
		// uninitialized stack float when absent.
		WriteFloat(rows, GA_T::SKILL_RATING, 5.0f);
		Write1B(rows, GA_T::TEAM_FLAG, teamed ? 1 : 0);
		WriteString(rows, GA_T::MISSION_FLAG, p.in_mission ? "T" : "F");
		++row_count;
	}

	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::QUERY_PLAYERS;
	uint16_t item_count = 2;  // CALLER_ID + DATA_SET

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	Write4B(response, GA_T::CALLER_ID, caller_id);
	append(response, GA_T::DATA_SET & 0xFF, GA_T::DATA_SET >> 8);
	append(response, row_count & 0xFF, (row_count >> 8) & 0xFF);
	WriteNBytesRaw(response, rows);

	Logger::Log("tcp", "[TCP] QUERY_PLAYERS: caller_id=%u online=%d matched=%d\n",
		caller_id, (int)players.size(), (int)row_count);

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
	uint16_t item_count = 6;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	WriteString(response, GA_T::PLAYER_NAME, player_name);
	Write4B(response, GA_T::CHARACTER_ID, static_cast<uint32_t>(selected_character_id_));
	Write4B(response, GA_T::PROFILE_ID, selected_profile_id_);
	Write4B(response, GA_T::CLASS_TYPE_VALUE_ID, GetClassConfig(selected_profile_id_).classTypeValueId);

	Write4B(response, GA_T::CLASS_MSG_ID,
		ResolveClassMsgId(selected_profile_id_, 22976, "ADD_PLAYER_CHARACTER"));
	Write4B(response, GA_T::HOME_MAP_GAME_ID, ResolveHomeMapGameId());

	send_response(response);
}

void TcpSession::send_delete_character_response(uint32_t character_id, uint32_t error_code)
{
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::DELETE_CHARACTER;
	uint16_t item_count = 2;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	Write4B(response, GA_T::ERROR_CODE,   error_code);
	Write4B(response, GA_T::CHARACTER_ID, character_id);

	send_response(response);
}

void TcpSession::send_select_character_response()
{
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::GSC_SELECT_CHARACTER;
	uint16_t item_count = 10;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	// Write4B(response, GA_T::CALLER_ID, 0x0);
	Write4B(response, GA_T::ERROR_CODE, 0x0);
	Write4B(response, GA_T::HOME_MAP_GAME_ID, ResolveHomeMapGameId());
	Write4B(response, GA_T::TASK_FORCE, 0x1);
	Write4B(response, GA_T::CHARACTER_ID, static_cast<uint32_t>(selected_character_id_));
	const uint32_t profile_id = selected_profile_id_ != 0 ? selected_profile_id_ : GA_G::PROFILE_ID_ASSAULT;
	Write4B(response, GA_T::PROFILE_ID, profile_id);
	Write4B(response, GA_T::CLASS_TYPE_VALUE_ID, GetClassConfig(profile_id).classTypeValueId);
	WriteString(response, GA_T::CLASS_MSG_ID,
		std::to_string(ResolveClassMsgId(profile_id, 22976, "GSC_SELECT_CHARACTER")));

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
	send_go_play_to_instance(*instance, home_task_force_);
}

void TcpSession::send_go_play_to_instance(const InstanceInfo& target, int task_force)
{
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::GSC_GO_PLAY;
	uint16_t item_count = 13;

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

	const uint32_t profile_id = selected_profile_id_ != 0 ? selected_profile_id_ : GA_G::PROFILE_ID_ASSAULT;
	const auto& class_config = GetClassConfig(profile_id);
	const uint32_t class_msg_id = ResolveClassMsgId(profile_id, 22976, "GSC_GO_PLAY");
	Write4B(response, GA_T::CLASS_TYPE_VALUE_ID, class_config.classTypeValueId);
	Write4B(response, GA_T::CLASS_MSG_ID, class_msg_id);

	send_response(response);
	Logger::Log("tcp", "[TcpSession] Sent GSC_GO_PLAY → instance=%lld map=%s game_id=%u name_msg=%u bg_res=%u class_type=%u class_msg=%u tf=%d for %s\n",
		(long long)target.instance_id, target.map_name.c_str(),
		map_game_id, friendly_name_msg_id, entry_background_image_res_id,
		class_config.classTypeValueId, class_msg_id, task_force, session_guid_.c_str());
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

	// Collect equipped cosmetics (helmet, suit, dyes, trail) for all characters.
	struct CosmeticEntry { uint32_t char_id; int item_id; int slot_value_id; };
	std::vector<CosmeticEntry> cosmetics;
	for (const auto& c : characters) {
		auto devices = PlayerSessionStore::GetDevicesForCharacter(c.id, c.current_item_profile_id);
		for (const auto& d : devices) {
			if (d.item_id > 0)
				cosmetics.push_back({ static_cast<uint32_t>(c.id), d.item_id, d.slot_value_id });
		}
	}

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
		append(response, 0x0E, 0x00);  // inner item count = 14
		Write4B(response, GA_T::CHARACTER_ID,               static_cast<uint32_t>(c.id));
		Write4B(response, GA_T::CHARACTER_LEVEL,            0x32);
		Write4B(response, GA_T::CURRENT_LEVEL,              0x32);
		Write4B(response, GA_T::FORCED_CHARACTER_LEVEL,     0x32);
		Write4B(response, GA_T::PROFILE_ID,                 c.profile_id);
		Write4B(response, GA_T::CLASS_TYPE_VALUE_ID,        GetClassConfig(c.profile_id).classTypeValueId);
		Write4B(response, GA_T::HEAD_ASM_ID,                c.head_asm_id);
		Write4B(response, GA_T::HAIR_ASM_ID,                c.hair_asm_id);
		Write4B(response, GA_T::GENDER_TYPE_VALUE_ID,       c.gender_type_value_id);
		WriteString(response, GA_T::MAP_NAME,               "Scramble: Tetra Pier");
		Write4B(response, GA_T::XP_VALUE,                   0xbbddc);
		Write4B(response, GA_T::SKILL_GROUP_SET_ID,         GetClassConfig(c.profile_id).skillGroupSetId);
		Write4B(response, GA_T::SKIN_MATERIAL_PARAMETER_ID, c.skin_mat_param_id);
		Write4B(response, GA_T::EYE_MATERIAL_PARAMETER_ID,  c.eye_mat_param_id);
	}

	append(response, GA_T::DATA_SET_PLAYER_INVENTORY & 0xFF, GA_T::DATA_SET_PLAYER_INVENTORY >> 8);
	append(response, static_cast<uint8_t>(cosmetics.size() & 0xFF),
	                 static_cast<uint8_t>(cosmetics.size() >> 8));
	for (const auto& entry : cosmetics) {
		append(response, 0x03, 0x00);  // 3 fields per record: CHARACTER_ID, ITEM_ID, EQUIPPED_SLOT_VALUE_ID
		Write4B(response, GA_T::CHARACTER_ID,           entry.char_id);
		Write4B(response, GA_T::ITEM_ID,                static_cast<uint32_t>(entry.item_id));
		Write4B(response, GA_T::EQUIPPED_SLOT_VALUE_ID, static_cast<uint32_t>(entry.slot_value_id));
	}

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
	append(response, 0x0C, 0x00);  // inner item count
	Write4B(response, GA_T::CHARACTER_ID, 0x2AC950);
	Write4B(response, GA_T::CHARACTER_LEVEL, 0x32);
	Write4B(response, GA_T::CURRENT_LEVEL, 0x32);
	Write4B(response, GA_T::FORCED_CHARACTER_LEVEL, 0x32);
	Write4B(response, GA_T::PROFILE_ID, GA_G::PROFILE_ID_ASSAULT);
	Write4B(response, GA_T::CLASS_TYPE_VALUE_ID, GetClassConfig(GA_G::PROFILE_ID_ASSAULT).classTypeValueId);
	Write4B(response, GA_T::HEAD_ASM_ID, GA_G::HEAD_ASM_ID_TROLL);
	Write4B(response, GA_T::HAIR_ASM_ID, 0x85D);
	Write4B(response, GA_T::GENDER_TYPE_VALUE_ID, 0x355);
	WriteString(response, GA_T::MAP_NAME, "Scramble: Tetra Pier");
	Write4B(response, GA_T::XP_VALUE, 0xbbddc);
	Write4B(response, GA_T::SKILL_GROUP_SET_ID, GA_G::SKILL_GROUP_SET_ID_ASSAULT);

	// MEDIC
	append(response, 0x0C, 0x00);  // inner item count
	Write4B(response, GA_T::CHARACTER_ID, 0x0000f69c);
	Write4B(response, GA_T::CHARACTER_LEVEL, 0x32);
	Write4B(response, GA_T::CURRENT_LEVEL, 0x32);
	Write4B(response, GA_T::FORCED_CHARACTER_LEVEL, 0x32);
	Write4B(response, GA_T::PROFILE_ID, GA_G::PROFILE_ID_MEDIC);
	Write4B(response, GA_T::CLASS_TYPE_VALUE_ID, GetClassConfig(GA_G::PROFILE_ID_MEDIC).classTypeValueId);
	Write4B(response, GA_T::HEAD_ASM_ID, GA_G::HEAD_ASM_ID_TROLL);
	Write4B(response, GA_T::HAIR_ASM_ID, 0x85D);
	Write4B(response, GA_T::GENDER_TYPE_VALUE_ID, 0x355);
	WriteString(response, GA_T::MAP_NAME, "Commonwealth HQ");
	Write4B(response, GA_T::XP_VALUE, 0xbbddc);
	Write4B(response, GA_T::SKILL_GROUP_SET_ID, GA_G::SKILL_GROUP_SET_ID_MEDIC);

	// RECON
	append(response, 0x0C, 0x00);  // inner item count
	Write4B(response, GA_T::CHARACTER_ID, 0x2AC950);
	Write4B(response, GA_T::CHARACTER_LEVEL, 0x32);
	Write4B(response, GA_T::CURRENT_LEVEL, 0x32);
	Write4B(response, GA_T::FORCED_CHARACTER_LEVEL, 0x32);
	Write4B(response, GA_T::PROFILE_ID, GA_G::PROFILE_ID_RECON);
	Write4B(response, GA_T::CLASS_TYPE_VALUE_ID, GetClassConfig(GA_G::PROFILE_ID_RECON).classTypeValueId);
	Write4B(response, GA_T::HEAD_ASM_ID, GA_G::HEAD_ASM_ID_TROLL);
	Write4B(response, GA_T::HAIR_ASM_ID, 0x85D);
	Write4B(response, GA_T::GENDER_TYPE_VALUE_ID, 0x355);
	WriteString(response, GA_T::MAP_NAME, "Scramble: Tetra Pier");
	Write4B(response, GA_T::XP_VALUE, 0xbbddc);
	Write4B(response, GA_T::SKILL_GROUP_SET_ID, GA_G::SKILL_GROUP_SET_ID_RECON);

	// ROBO
	append(response, 0x0C, 0x00);  // inner item count
	Write4B(response, GA_T::CHARACTER_ID, 0x2AC950);
	Write4B(response, GA_T::CHARACTER_LEVEL, 0x32);
	Write4B(response, GA_T::CURRENT_LEVEL, 0x32);
	Write4B(response, GA_T::FORCED_CHARACTER_LEVEL, 0x32);
	Write4B(response, GA_T::PROFILE_ID, GA_G::PROFILE_ID_ROBOTIC);
	Write4B(response, GA_T::CLASS_TYPE_VALUE_ID, GetClassConfig(GA_G::PROFILE_ID_ROBOTIC).classTypeValueId);
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


void TcpSession::send_login_response_for(const std::string& response_player_name,
                                         const std::string& response_session_guid,
                                         bool success,
                                         const char* error_text,
                                         uint32_t error_msg_id)
{
	std::vector<uint8_t> response;

	// MSG_ID is what the client actually displays on failure (FinishLogin's
	// error path Translates it); ERROR_TEXT/DESC are not shown by the login menu.
	const bool write_msg_id = error_text && error_msg_id != 0;

	uint16_t packet_type = GA_U::GSC_USER_LOGIN_RESPONSE;
	uint16_t item_count = error_text ? 12 : 9;
	if (write_msg_id) item_count += 1;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	Write1B(response,     GA_T::SUCCESS, success ? 1 : 0);
	WriteNBytes(response, GA_T::SESSION_GUID, GuidHexToBytes(response_session_guid));
	Write4B(response,     GA_T::GAME_BITS, 0x0000000F);
	Write2B(response,     GA_T::NET_ACCESS_FLAGS, 0xF3F8);
	Write4B(response,     GA_T::AFK_TIMEOUT_SEC, 0x00000384);
	WriteNBytes(response, GA_T::DISPLAY_EULA_FLAG, {0x01, 0x00, 0x6E});
	WriteString(response, GA_T::PLAYER_NAME, response_player_name.c_str());

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

	if (error_text) {
		Write4B(response, GA_T::ERROR_CODE, 1);
		WriteString(response, GA_T::ERROR_TEXT, error_text);
		WriteString(response, GA_T::ERROR_DESC, error_text);
	}
	if (write_msg_id) {
		Write4B(response, GA_T::MSG_ID, error_msg_id);
	}

	send_response(response);
}

void TcpSession::send_login_rejected_response(const char* error_text, uint32_t msg_id)
{
	send_login_response_for("", "00000000000000000000000000000000", false, error_text, msg_id);
}

void TcpSession::send_login_response()
{
	send_login_response_for(player_name, session_guid_);
}

// ===========================================================================
// User moderation — login-bug spoof, live-kick, online snapshot.
// ===========================================================================

void TcpSession::ApplyBanSpoof() {
	if (s_ban_spoof_mode_ == "garbage") {
		std::vector<uint8_t> garbage(32);
		for (auto& b : garbage) b = static_cast<uint8_t>(std::rand() & 0xFF);
		std::error_code ec;
		asio::write(socket_, asio::buffer(garbage), ec);
		close_after_login_rejection();
		return;
	}
	// "silent" (default): do nothing. The TcpSession sits idle in do_read.
	// The client's own ~10s read timeout fires and surfaces "Unable to
	// connect to server" — matches the historical login-bug surface.
	if (s_ban_spoof_fallback_close_sec_ > 0) {
		auto self = shared_from_this();
		auto t = std::make_shared<asio::steady_timer>(io_ctx_);
		t->expires_after(std::chrono::seconds(s_ban_spoof_fallback_close_sec_));
		t->async_wait([self, t](std::error_code ec) {
			if (!ec) self->close_after_login_rejection();
		});
	}
}

void TcpSession::send_kick_to_bogus_map() {
	// Slim sibling of send_go_play_response() — same opcode + the minimum
	// set of fields the client expects on a GO_PLAY frame, but the map
	// name is one no client will resolve. Client tries to switch maps,
	// fails, and falls into its local disconnect path (which closes the
	// control TCP socket → existing do_read error branch sweeps the rest).
	std::vector<uint8_t> response;

	uint16_t packet_type = GA_U::GSC_GO_PLAY;
	uint16_t item_count = 2;

	append(response, packet_type & 0xFF, packet_type >> 8);
	append(response, item_count & 0xFF, item_count >> 8);

	WriteIP(response, GA_T::HOST_NET_ADDR, "999.999.999.999", 0);
	WriteString(response, GA_T::MAP_FILENAME, "Login_FreeAgent");


	send_response(response);

	Logger::Log("ban", "[ban] kick guid=%s user_id=%lld name=%s ip=%s\n",
		session_guid_.c_str(), (long long)user_id_,
		player_name.c_str(), ip_address_.c_str());
}

void TcpSession::arm_kick_fallback_timer() {
	if (s_kick_fallback_close_sec_ <= 0) return;
	auto self = shared_from_this();
	auto t = std::make_shared<asio::steady_timer>(io_ctx_);
	t->expires_after(std::chrono::seconds(s_kick_fallback_close_sec_));
	t->async_wait([self, t](std::error_code ec) {
		if (!ec) self->close_after_login_rejection();
	});
}

int TcpSession::KickSessionsForUser(int64_t user_id) {
	if (user_id <= 0) return 0;
	// Snapshot under lock — we'll mutate session state through
	// MatchmakingService / write the socket outside the lock.
	std::vector<std::shared_ptr<TcpSession>> targets;
	{
		std::lock_guard<std::mutex> lock(sessions_mutex_);
		for (auto& [guid, weak] : g_sessions_) {
			auto s = weak.lock();
			if (s && s->user_id_ == user_id) targets.push_back(s);
		}
	}
	for (auto& s : targets) {
		MatchmakingService::RemovePlayer(s->session_guid_);
		s->send_kick_to_bogus_map();
		s->arm_kick_fallback_timer();
	}
	return (int)targets.size();
}

int TcpSession::KickSessionsForIp(const std::string& ip) {
	if (ip.empty()) return 0;
	std::vector<std::shared_ptr<TcpSession>> targets;
	{
		std::lock_guard<std::mutex> lock(sessions_mutex_);
		for (auto& [guid, weak] : g_sessions_) {
			auto s = weak.lock();
			if (!s) continue;
			// ip_address_ is "1.2.3.4:port" — match the host octet quad,
			// not the port.
			if (s->ip_address_.rfind(ip + ":", 0) == 0) targets.push_back(s);
		}
	}
	for (auto& s : targets) {
		MatchmakingService::RemovePlayer(s->session_guid_);
		s->send_kick_to_bogus_map();
		s->arm_kick_fallback_timer();
	}
	return (int)targets.size();
}

void TcpSession::ForEachLiveSession(const std::function<void(const OnlineSnapshot&)>& visit) {
	std::vector<OnlineSnapshot> snaps;
	{
		std::lock_guard<std::mutex> lock(sessions_mutex_);
		for (auto& [guid, weak] : g_sessions_) {
			auto s = weak.lock();
			if (!s) continue;
			OnlineSnapshot snap;
			snap.session_guid = s->session_guid_;
			snap.user_id      = s->user_id_;
			snap.username     = s->player_name;
			const std::string& ipp = s->ip_address_;
			auto colon = ipp.find(':');
			snap.ip = (colon == std::string::npos) ? ipp : ipp.substr(0, colon);
			snap.instance_id  = s->assigned_instance_id_;
			snap.login_at     = s->session_login_at_;
			snaps.push_back(std::move(snap));
		}
	}
	for (const auto& s : snaps) visit(s);
}
