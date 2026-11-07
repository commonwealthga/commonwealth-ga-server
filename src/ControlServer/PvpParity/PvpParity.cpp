#include "src/ControlServer/PvpParity/PvpParity.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <mutex>
#include <set>
#include <string>
#include <vector>

#include "sqlite3.h"
#include "src/ControlServer/AchievementService/AchievementService.hpp"
#include "src/ControlServer/ChatSession/ChatSession.hpp"
#include "src/ControlServer/Database/Database.hpp"
#include "src/ControlServer/Logger.hpp"
#include "src/ControlServer/MatchmakingService/Domain.hpp"
#include "src/ControlServer/MatchmakingService/MatchmakingService.hpp"
#include "src/ControlServer/PlayerSessionStore/PlayerSessionStore.hpp"
#include "src/ControlServer/TcpSession/TcpSession.hpp"
#include "src/ControlServer/TeamService/TeamService.hpp"

namespace PvpParity {

namespace {

constexpr uint32_t kMercQueue = 2;
constexpr uint32_t kMerc30Queue = 73;  // hidden level 30+ bracket

void Exec(sqlite3* db, const char* sql) {
    char* err = nullptr;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &err) != SQLITE_OK) {
        Logger::Log("db", "[PvpParity] migration step failed: %s\n", err ? err : "?");
        if (err) sqlite3_free(err);
    }
}

std::string Setting(const char* key) {
    sqlite3* db = Database::GetConnection();
    std::string v;
    sqlite3_stmt* s = nullptr;
    if (db && sqlite3_prepare_v2(db, "SELECT value FROM cs_settings WHERE key = ?", -1, &s, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(s, 1, key, -1, SQLITE_STATIC);
        if (sqlite3_step(s) == SQLITE_ROW && sqlite3_column_text(s, 0))
            v = reinterpret_cast<const char*>(sqlite3_column_text(s, 0));
    }
    sqlite3_finalize(s);
    return v;
}

std::string Msg(int msg_id, const char* fallback) {
    sqlite3* db = Database::GetConnection();
    std::string v = fallback;
    sqlite3_stmt* s = nullptr;
    if (db && sqlite3_prepare_v2(db, "SELECT message FROM asm_data_set_msg_translations WHERE msg_id = ?",
                                 -1, &s, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(s, 1, msg_id);
        if (sqlite3_step(s) == SQLITE_ROW && sqlite3_column_text(s, 0))
            v = reinterpret_cast<const char*>(sqlite3_column_text(s, 0));
    }
    sqlite3_finalize(s);
    return v;
}

void Replace(std::string& s, const std::string& key, const std::string& val) {
    const auto p = s.find(key);
    if (p != std::string::npos) s.replace(p, key.size(), val);
}

bool IsPvpQueue(uint32_t queue_id) {
    if (queue_id >= 40 && queue_id <= 45) return false;  // AvA has its own rules
    const auto qc = MatchmakingService::GetQueueConfig(queue_id);
    return qc && qc->taskforce_policy == TaskforcePolicy::BalancedPvp && qc->difficulty_value_id == 0;
}

// Seconds of deserter penalty left for a character (0 = none).
int64_t DeserterLeft(int64_t cid, int64_t penalty) {
    sqlite3* db = Database::GetConnection();
    if (!db || cid == 0 || penalty <= 0) return 0;
    const int64_t now = (int64_t)std::time(nullptr);
    int64_t left = 0;
    sqlite3_stmt* s = nullptr;
    // Left before the mission ended (end_mission_at = BeginEndMission time) while a
    // human sat on the other task force (solo pops vs nobody never count).
    if (sqlite3_prepare_v2(db,
            "SELECT i.queue_id, p.left_at FROM ga_instance_players p "
            "JOIN ga_instances i ON i.instance_id = p.instance_id "
            "WHERE p.character_id = ?1 AND p.left_at IS NOT NULL AND p.left_at > ?2 - ?3 "
            "  AND i.queue_id IS NOT NULL "
            "  AND (IFNULL(i.end_mission_at, 0) = 0 OR p.left_at < i.end_mission_at) "
            "  AND EXISTS (SELECT 1 FROM ga_instance_players o WHERE o.instance_id = p.instance_id "
            "              AND o.task_force_number != p.task_force_number "
            "              AND o.character_id IS NOT NULL AND o.character_id != p.character_id) "
            "ORDER BY p.left_at DESC", -1, &s, nullptr) != SQLITE_OK) return 0;
    sqlite3_bind_int64(s, 1, cid);
    sqlite3_bind_int64(s, 2, now);
    sqlite3_bind_int64(s, 3, penalty);
    while (sqlite3_step(s) == SQLITE_ROW) {
        const uint32_t q = (uint32_t)sqlite3_column_int(s, 0);
        if (!IsPvpQueue(q)) continue;
        const auto qc = MatchmakingService::GetQueueConfig(q);
        if (!qc || qc->reward_level_max == 0) continue;  // scrimmages: no penalty
        left = sqlite3_column_int64(s, 1) + penalty - now;
        break;
    }
    sqlite3_finalize(s);
    return left > 0 ? left : 0;
}

std::string CharName(int64_t cid) {
    sqlite3* db = Database::GetConnection();
    std::string n;
    sqlite3_stmt* s = nullptr;
    if (db && sqlite3_prepare_v2(db, "SELECT u.username FROM ga_characters c JOIN ga_users u ON u.id = c.user_id WHERE c.id = ?", -1, &s, nullptr) == SQLITE_OK) {
        sqlite3_bind_int64(s, 1, cid);
        if (sqlite3_step(s) == SQLITE_ROW && sqlite3_column_text(s, 0))
            n = reinterpret_cast<const char*>(sqlite3_column_text(s, 0));
    }
    sqlite3_finalize(s);
    return n;
}

}  // namespace

static void Migrate0513(sqlite3* db);

void Migrate(sqlite3* db) {
    if (!db) return;
    bool done = false;
    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(db, "SELECT 1 FROM cs_migration_markers WHERE name = 'pvp_parity_0510_2026_11_01'",
                           -1, &s, nullptr) == SQLITE_OK)
        done = sqlite3_step(s) == SQLITE_ROW;
    sqlite3_finalize(s);
    if (done) { Migrate0513(db); return; }

    static const char* kCols =
        "(queue_id, name, taskforce_policy, continue_in_queue, enabled,"
        " queue_type_value_id, status_msg_id, name_msg_id, desc_msg_id, icon_id,"
        " max_players_per_side, min_players_per_team, max_players_per_team,"
        " level_min, level_max, tab, map_x, map_y, map_active_flag,"
        " map_icon_texture_res_id, video_res_id, location_value_id,"
        " double_agent_flag, sys_site_id, sort_order, bonus_queue_flag,"
        " difficulty_value_id, access_flags, active_flag, locked_flag,"
        " map_pool_id, min_players_to_pop, max_players_per_instance,"
        " pop_delay_seconds, pop_delay_policy, instant_pop_when_full,"
        " requires_pvp_verification, team_policy, team_side_policy,"
        " max_team_size, victory_bonus_lives, map_recency_divisors, reward_level_max) VALUES ";
    // Arena tab (443) next to 1v1 / Beta Maps; queue type 1421 = arena (win loot 348).
    const std::string queues = std::string("INSERT OR IGNORE INTO ga_queues ") + kCols +
        " (70, 'arena_4v4', 'balanced_pvp', 0, 1, 1421, 0, 41470, 41469, 532, 4, 1, 4, 30, 200, 443, 6.0, 0.0, 1,"
        "  5126, 0, 1477, 1, 0, 3, 0, 0, 0, 1, 0, 70, 2, 8, 0.0, 'halve_on_join', 1, 0, 'mixed', 'preferred', 4, 0, '2', 200),"
        " (71, 'arena_4v4_scrim', 'balanced_pvp', 0, 1, 1421, 0, 61672, 61671, 532, 4, 1, 4, 10, 200, 443, 6.0, 0.0, 1,"
        "  5126, 0, 1477, 1, 0, 4, 0, 0, 0, 1, 0, 70, 2, 8, 0.0, 'halve_on_join', 1, 0, 'mixed', 'preferred', 4, 0, '2', 0),"
        " (72, 'ava_scrimmage', 'balanced_pvp', 0, 1, 1421, 0, 61104, 61103, 532, 10, 1, 10, 10, 200, 443, 6.0, 0.0, 1,"
        "  5126, 0, 1477, 1, 0, 5, 0, 0, 0, 1, 0, 71, 2, 20, 0.0, 'halve_on_join', 1, 0, 'mixed', 'preferred', 10, 0, '2', 0);";
    const char* steps[] = {
        "INSERT OR IGNORE INTO ga_map_pools (map_pool_id, name) VALUES (70, 'arena_4v4'), (71, 'ava_scrimmage');",
        "INSERT OR IGNORE INTO ga_map_pool_entries (map_pool_id, map_name, game_mode, weight, enabled) VALUES"
        " (70, 'Ticket_HimLab_4v4',    'TgGame.TgGame_PointRotation', 1, 1),"
        " (70, 'Ticket_Silo_4v4_P',    'TgGame.TgGame_PointRotation', 1, 1),"
        " (70, 'Ticket_Osprey_4v4_P',  'TgGame.TgGame_PointRotation', 1, 1),"
        " (70, 'MissileComplex_4v4_P', 'TgGame.TgGame_Mission',       1, 1),"
        " (71, 'HEX_AVA_2pt_Theft_Factory1_P', 'TgGame.TgGame_Escort', 1, 1),"
        " (71, 'HEX_AVA_2pt_Theft_Lab1',       'TgGame.TgGame_Escort', 1, 1),"
        " (71, 'HEX_AVA_Push_Factory1_P',      'TgGame.TgGame_Escort', 1, 1),"
        " (71, 'HEX_AVA_Push_Lab1_P',          'TgGame.TgGame_Escort', 1, 1),"
        " (7,  'CTR_Recursive_P', 'TgGame.TgGame_PointRotation', 10, 1);",
        queues.c_str(),
        // Hidden level 30+ merc bracket (used only with pvp_merc_brackets=original).
        "DROP TABLE IF EXISTS temp.pvp_q73;",
        "CREATE TEMP TABLE pvp_q73 AS SELECT * FROM ga_queues WHERE queue_id = 2;",
        "UPDATE temp.pvp_q73 SET queue_id = 73, name = 'merc_30', enabled = 0, level_min = 30;",
        "INSERT OR IGNORE INTO ga_queues SELECT * FROM temp.pvp_q73;",
        "DROP TABLE IF EXISTS temp.pvp_q73;",
        "UPDATE ga_map_pool_entries SET max_players = 8 WHERE map_pool_id = 2 AND map_name IN"
        " ('Ticket_HimLab_4v4', 'Ticket_Silo_4v4_P', 'Ticket_Osprey_4v4_P', 'MissileComplex_4v4_P');",
        "UPDATE ga_map_pool_entries SET enabled = 0 WHERE map_pool_id = 2 AND"
        " (map_name LIKE 'HEX_AVA_%' OR map_name = 'CTR_Recursive_P');",
        "UPDATE ga_queues SET requires_pvp_verification = 0 WHERE queue_id = 18 AND name = 'beta_pvp';",
        "INSERT OR IGNORE INTO cs_settings (key, value) VALUES ('pvp_deserter_seconds', '900'), ('pvp_merc_brackets', 'one');",
        "INSERT OR IGNORE INTO cs_migration_markers (name) VALUES ('pvp_parity_0510_2026_11_01');",
    };
    for (const char* sql : steps) Exec(db, sql);
    Logger::Log("db", "[PvpParity] 0510 applied: arena_4v4/arena_4v4_scrim/ava_scrimmage, Beta Maps = Colony Caverns\n");
    Migrate0513(db);
}

// Challenge queue + pool (own marker, so it also lands on DBs that already ran the earlier migration).
static void Migrate0513(sqlite3* db) {
    bool done = false;
    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(db, "SELECT 1 FROM cs_migration_markers WHERE name = 'pvp_parity_0513_2026_11_01'",
                           -1, &s, nullptr) == SQLITE_OK)
        done = sqlite3_step(s) == SQLITE_ROW;
    sqlite3_finalize(s);
    if (done) return;
    const char* steps[] = {
        "INSERT OR IGNORE INTO ga_map_pools (map_pool_id, name) VALUES (72, 'challenge');",
        // msg 62818 "List of maps used for Team Challenges" (WIKI-Arena + 1.39 Missile Complex).
        "INSERT OR IGNORE INTO ga_map_pool_entries (map_pool_id, map_name, game_mode, weight, enabled)"
        " SELECT 72, map_name, game_class, 1, 1 FROM map_game_info"
        " WHERE map_game_id IN (1131, 1171, 1369, 1419, 1417, 1291, 1307, 1439) AND IFNULL(game_class, '') <> '';",
        "DROP TABLE IF EXISTS temp.pvp_q74;",
        "CREATE TEMP TABLE pvp_q74 AS SELECT * FROM ga_queues WHERE queue_id = 70;",
        "UPDATE temp.pvp_q74 SET queue_id = 74, name = 'challenge', enabled = 0, rule_class = 'VersusSides',"
        " team_policy = 'versus_sides', team_side_policy = 'ignore', name_msg_id = 62819, desc_msg_id = 62819,"
        " level_min = 1, map_pool_id = 72, reward_level_max = 0, fairness_scope = NULL;",
        "INSERT OR IGNORE INTO ga_queues SELECT * FROM temp.pvp_q74;",
        "DROP TABLE IF EXISTS temp.pvp_q74;",
        "INSERT OR IGNORE INTO cs_migration_markers (name) VALUES ('pvp_parity_0513_2026_11_01');",
    };
    for (const char* sql : steps) Exec(db, sql);
    Logger::Log("db", "[PvpParity] 0513 applied: hidden challenge queue 74 + pool 72\n");
}

namespace {
struct PendingChallenge {
    std::set<std::string> a, b;   // challenger side, challenged side (session guids)
    int a_tf = 1;
    std::string map, mode, a_name, b_name;
    int64_t expires = 0;
};
std::mutex g_ch_mu;
std::vector<PendingChallenge> g_ch;

std::set<std::string> SideOf(const std::string& leader) {
    std::set<std::string> out{leader};
    if (TeamService::IsTeamed(leader))
        for (const auto& g : TeamService::GetTeamMemberGuids(leader)) out.insert(g);
    return out;
}
}  // namespace

void LaunchChallenge(const std::string& from_guid, const std::string& from_name,
                     const std::string& to_guid, const std::string& to_name,
                     uint32_t map_game_id, bool challenger_defends) {
    PendingChallenge p;
    p.a = SideOf(from_guid); p.b = SideOf(to_guid);
    p.a_tf = challenger_defends ? 2 : 1;
    p.a_name = from_name; p.b_name = to_name;
    p.expires = (int64_t)std::time(nullptr) + 180;
    if (sqlite3* db = Database::GetConnection()) {
        sqlite3_stmt* s = nullptr;
        if (sqlite3_prepare_v2(db,
                "SELECT e.map_name, e.game_mode FROM ga_map_pool_entries e JOIN map_game_info m ON m.map_name = e.map_name "
                "WHERE e.map_pool_id = 72 AND e.enabled = 1 AND m.map_game_id = ? LIMIT 1", -1, &s, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(s, 1, (int)map_game_id);
            if (sqlite3_step(s) == SQLITE_ROW) {
                p.map  = reinterpret_cast<const char*>(sqlite3_column_text(s, 0));
                p.mode = reinterpret_cast<const char*>(sqlite3_column_text(s, 1));
            }
        }
        sqlite3_finalize(s);
    }
    {
        std::lock_guard<std::mutex> l(g_ch_mu);
        const int64_t now = (int64_t)std::time(nullptr);
        g_ch.erase(std::remove_if(g_ch.begin(), g_ch.end(), [&](const PendingChallenge& x) {
            return x.expires < now || x.a.count(from_guid) || x.b.count(from_guid)
                || x.a.count(to_guid) || x.b.count(to_guid); }), g_ch.end());
        g_ch.push_back(p);
    }
    Logger::Log("matchmaking", "[PvpParity] challenge %s (%zu, tf %d) vs %s (%zu) map_game_id=%u -> %s; queueing both on %u\n",
                from_name.c_str(), p.a.size(), p.a_tf, to_name.c_str(), p.b.size(), map_game_id,
                p.map.empty() ? "(pool pick)" : p.map.c_str(), kChallengeQueue);
    const std::string line = "*** Team Challenge: " + from_name + " vs " + to_name + " - launching the match... ***";
    ChatSession::SystemMessageToGuid(from_guid, line);
    ChatSession::SystemMessageToGuid(to_guid, line);
    TcpSession::DeliverZoneQueueJoin(from_guid, kChallengeQueue);
    TcpSession::DeliverZoneQueueJoin(to_guid, kChallengeQueue);
}

void ChallengePop(MatchResult& r) {
    std::lock_guard<std::mutex> l(g_ch_mu);
    for (auto it = g_ch.begin(); it != g_ch.end(); ++it) {
        bool has_a = false, has_b = false;
        for (const auto& g : r.session_guids) { has_a |= it->a.count(g) > 0; has_b |= it->b.count(g) > 0; }
        if (!has_a || !has_b) continue;
        if (!it->map.empty() && !r.existing_instance_id) { r.map_name = it->map; r.game_mode = it->mode; }
        for (auto& [g, tf] : r.task_force_assignments) tf = it->a.count(g) ? it->a_tf : 3 - it->a_tf;
        Logger::Log("matchmaking", "[PvpParity] challenge pop %s vs %s map=%s challenger tf=%d\n",
                    it->a_name.c_str(), it->b_name.c_str(), r.map_name.c_str(), it->a_tf);
        return;   // kept until it expires: a pop can retry (no free server slot)
    }
    Logger::Log("matchmaking", "[PvpParity] challenge pop without a matching pending challenge (%zu players)\n",
                r.session_guids.size());
}

void OnMatchEnded(int64_t instance_id, uint32_t queue_id) {
    if (queue_id != kChallengeQueue || instance_id == 0) return;
    sqlite3* db = Database::GetConnection();
    if (!db) return;
    std::vector<int64_t> chars;
    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(db, "SELECT DISTINCT character_id FROM ga_instance_players "
            "WHERE instance_id = ? AND left_at IS NULL AND character_id > 0", -1, &s, nullptr) == SQLITE_OK) {
        sqlite3_bind_int64(s, 1, instance_id);
        while (sqlite3_step(s) == SQLITE_ROW) chars.push_back(sqlite3_column_int64(s, 0));
    }
    sqlite3_finalize(s);
    std::vector<std::pair<int64_t, int>> un;
    for (int64_t cid : chars)   // metric 1588 "Team Challenge Participation (Count)": 367/370/371/372
        for (const auto& u : AchievementService::CreditMetric(cid, 1588)) un.emplace_back(u.character_id, u.achievement_id);
    Logger::Log("matchmaking", "[PvpParity] challenge instance=%lld ended: %zu participant(s) credited, %zu unlock(s)\n",
                (long long)instance_id, chars.size(), un.size());
    if (!un.empty()) TcpSession::PushAchievementUnlocks(un);
}

uint32_t GateJoin(const std::string& guid, uint32_t queue_id, uint32_t match_filters) {
    if (!IsPvpQueue(queue_id) && queue_id != kMerc30Queue) return queue_id;
    // Log the queue screen's game-type mask.
    Logger::Log("tcp", "[PvpParity] MATCH_JOIN queue=%u filters=0x%08x guid=%s\n", queue_id, match_filters, guid.c_str());

    // Who joins: the whole team (leader queues it) or just this agent.
    std::vector<std::pair<std::string, int64_t>> who;  // guid, character
    if (TeamService::IsTeamed(guid)) {
        if (auto party = TeamService::BuildParty(guid))
            for (const auto& m : party->members) {
                const auto si = PlayerSessionStore::GetByGuid(m.session_guid);
                who.emplace_back(m.session_guid, si ? si->selected_character_id : 0);
            }
    }
    if (who.empty()) {
        const auto si = PlayerSessionStore::GetByGuid(guid);
        who.emplace_back(guid, si ? si->selected_character_id : 0);
    }

    // Deserter penalty.
    const std::string ps = Setting("pvp_deserter_seconds");
    const int64_t penalty = ps.empty() ? 900 : std::atoll(ps.c_str());
    for (const auto& [g, cid] : who) {
        const int64_t left = DeserterLeft(cid, penalty);
        if (left <= 0) continue;
        char t[16];
        std::snprintf(t, sizeof t, "%lld:%02lld", (long long)(left / 60), (long long)(left % 60));
        std::string remain = Msg(37022, "Deserter time remaining: @@string_value@@");
        Replace(remain, "@@string_value@@", t);
        if (g == guid) {
            ChatSession::SystemMessageToGuid(guid, Msg(36818, "You cannot join the queue yet, you are a deserter."));
        } else {
            std::string m = Msg(36821, "You have a deserter in your team (@@player_name@@) so you cannot join this queue.");
            Replace(m, "@@player_name@@", CharName(cid));
            ChatSession::SystemMessageToGuid(guid, m);
        }
        ChatSession::SystemMessageToGuid(guid, remain);
        Logger::Log("tcp", "[PvpParity] deserter: queue %u refused for %s (char %lld, %llds left)\n",
                    queue_id, guid.c_str(), (long long)cid, (long long)left);
        return 0;
    }

    // Merc brackets.
    if (queue_id == kMercQueue || queue_id == kMerc30Queue) {
        if (Setting("pvp_merc_brackets") != "original") return kMercQueue;
        uint32_t top = 0;
        for (const auto& [g, cid] : who)
            if (cid) top = std::max(top, PlayerSessionStore::GetCharacterLevelXp(cid).level);
        const uint32_t q = top >= 30 ? kMerc30Queue : kMercQueue;
        Logger::Log("tcp", "[PvpParity] merc bracket: %s level %u -> queue %u\n", guid.c_str(), top, q);
        return q;
    }
    return queue_id;
}

}  // namespace PvpParity
