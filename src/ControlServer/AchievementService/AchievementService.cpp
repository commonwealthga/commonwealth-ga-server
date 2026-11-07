#include "src/ControlServer/CombatRating/CombatRating.hpp"
#include "src/ControlServer/AchievementService/AchievementService.hpp"
#include "src/ControlServer/EconParity/EconParity.hpp"
#include "src/ControlServer/Database/Database.hpp"
#include "src/ControlServer/PlayerSessionStore/PlayerSessionStore.hpp"
#include "src/ControlServer/Logger.hpp"
#include "sqlite3.h"
#include <array>
#include <map>
#include <mutex>
#include <set>

namespace AchievementService {
namespace {

struct Req {
    int achievement_id = 0, metric = 0, map_game_id = 0, gameplay = 0,
        class_value = 0, difficulty = 0;
    double value = 0;
};

std::mutex g_mu;
std::vector<Req> g_reqs;
std::map<int, int> g_sort;  // valid value id -> sort_order (difficulties)
bool g_loaded = false;

void LoadLocked(sqlite3* db) {
    if (g_loaded) return;
    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(db,
            "SELECT achievement_id, metric_value_id, map_game_id, gameplay_type_value_id, "
            "       class_value_id, difficulty_value_id, requirement_value "
            "FROM asm_data_set_achievement_reqs", -1, &s, nullptr) != SQLITE_OK) return;
    while (sqlite3_step(s) == SQLITE_ROW) {
        Req r;
        r.achievement_id = sqlite3_column_int(s, 0);
        r.metric         = sqlite3_column_int(s, 1);
        r.map_game_id    = sqlite3_column_int(s, 2);
        r.gameplay       = sqlite3_column_int(s, 3);
        r.class_value    = sqlite3_column_int(s, 4);
        r.difficulty     = sqlite3_column_int(s, 5);
        r.value          = sqlite3_column_double(s, 6);
        g_reqs.push_back(r);
    }
    sqlite3_finalize(s);
    if (sqlite3_prepare_v2(db,
            "SELECT value_id, sort_order FROM asm_data_set_valid_values "
            "WHERE valid_value_group_id = 116", -1, &s, nullptr) == SQLITE_OK) {
        while (sqlite3_step(s) == SQLITE_ROW)
            g_sort[sqlite3_column_int(s, 0)] = sqlite3_column_int(s, 1);
        sqlite3_finalize(s);
    }
    g_sort.emplace(1029, 4);  // Medium (not in group 116) sits below High
    g_loaded = !g_reqs.empty();
    Logger::Log("db", "[Achievements] loaded %zu requirement rows\n", g_reqs.size());
}

int ClassValueForProfile(int profile_id) {
    switch (profile_id) {
        case 567: return 941;  // Medic
        case 681: return 942;  // Recon
        case 680: return 943;  // Assault
        case 679: return 944;  // Robotics
        default:  return 0;
    }
}

struct CharInfo { int64_t user_id = 0; int class_value = 0; };
CharInfo LoadChar(sqlite3* db, int64_t cid) {
    CharInfo ci;
    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(db, "SELECT user_id, profile_id FROM ga_characters WHERE id = ?",
                           -1, &s, nullptr) != SQLITE_OK) return ci;
    sqlite3_bind_int64(s, 1, cid);
    if (sqlite3_step(s) == SQLITE_ROW) {
        ci.user_id = sqlite3_column_int64(s, 0);
        ci.class_value = ClassValueForProfile(sqlite3_column_int(s, 1));
    }
    sqlite3_finalize(s);
    return ci;
}

bool Grant(sqlite3* db, int64_t cid, int achievement_id, std::vector<Unlock>& out) {
    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(db,
            "INSERT OR IGNORE INTO ga_character_achievements (character_id, achievement_id, achieved_at) "
            "VALUES (?, ?, strftime('%s','now'))", -1, &s, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int64(s, 1, cid);
    sqlite3_bind_int(s, 2, achievement_id);
    sqlite3_step(s);
    sqlite3_finalize(s);
    if (sqlite3_changes(db) != 1) return false;
    out.push_back({cid, achievement_id});
    Logger::Log("rewards", "[Achievements] char=%lld earned achievement %d\n",
                (long long)cid, achievement_id);
    return true;
}

bool Has(sqlite3* db, int64_t cid, int achievement_id) {
    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(db, "SELECT 1 FROM ga_character_achievements "
            "WHERE character_id = ? AND achievement_id = ?", -1, &s, nullptr) != SQLITE_OK) return true;
    sqlite3_bind_int64(s, 1, cid);
    sqlite3_bind_int(s, 2, achievement_id);
    const bool has = sqlite3_step(s) == SQLITE_ROW;
    sqlite3_finalize(s);
    return has;
}

int64_t Bump(sqlite3* db, int64_t cid, int achievement_id) {
    sqlite3_stmt* s = nullptr;
    int64_t n = 0;
    if (sqlite3_prepare_v2(db,
            "INSERT INTO ga_achievement_progress (character_id, achievement_id, count) VALUES (?1, ?2, 1) "
            "ON CONFLICT(character_id, achievement_id) DO UPDATE SET count = count + 1 "
            "RETURNING count", -1, &s, nullptr) != SQLITE_OK) return 0;
    sqlite3_bind_int64(s, 1, cid);
    sqlite3_bind_int(s, 2, achievement_id);
    if (sqlite3_step(s) == SQLITE_ROW) n = sqlite3_column_int64(s, 0);
    sqlite3_finalize(s);
    return n;
}

// "across ALL classes": every class has an agent on this account whose
// progress (or star rating) reached the value.
bool AllClassesReached(sqlite3* db, int64_t user_id, int achievement_id, double value, bool stars) {
    std::set<int> done;
    sqlite3_stmt* s = nullptr;
    const char* sql = stars
        ? "SELECT id, profile_id, 0 FROM ga_characters WHERE user_id = ?1 AND deleted_at = 0"
        : "SELECT c.id, c.profile_id, p.count FROM ga_characters c JOIN ga_achievement_progress p "
          "  ON p.character_id = c.id AND p.achievement_id = ?2 WHERE c.user_id = ?1 AND c.deleted_at = 0";
    if (sqlite3_prepare_v2(db, sql, -1, &s, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int64(s, 1, user_id);
    if (!stars) sqlite3_bind_int(s, 2, achievement_id);
    std::vector<std::pair<int64_t, std::pair<int, double>>> rows;
    while (sqlite3_step(s) == SQLITE_ROW)
        rows.push_back({sqlite3_column_int64(s, 0),
                        {sqlite3_column_int(s, 1), sqlite3_column_double(s, 2)}});
    sqlite3_finalize(s);
    for (const auto& [cid, pc] : rows) {
        const double have = stars
            ? (double)CombatRating::Stars(cid, PlayerSessionStore::GetCharacterLevelXp(cid).level) : pc.second;
        const int cls = ClassValueForProfile(pc.first);
        if (cls && have >= value) done.insert(cls);
    }
    return done.size() >= 4;
}

}  // namespace

void EnsureSchema() {
    sqlite3* db = Database::GetConnection();
    if (!db) return;
    sqlite3_exec(db,
        "CREATE TABLE IF NOT EXISTS ga_character_achievements ("
        "  character_id INTEGER NOT NULL, achievement_id INTEGER NOT NULL,"
        "  achieved_at INTEGER NOT NULL, PRIMARY KEY (character_id, achievement_id));"
        "CREATE TABLE IF NOT EXISTS ga_achievement_progress ("
        "  character_id INTEGER NOT NULL, achievement_id INTEGER NOT NULL,"
        "  count INTEGER NOT NULL DEFAULT 0, PRIMARY KEY (character_id, achievement_id));",
        nullptr, nullptr, nullptr);
    // reward_granted: the achievement's loot_table reward went into the bag
    // (fails harmlessly once the column exists).
    sqlite3_exec(db, "ALTER TABLE ga_character_achievements "
                     "ADD COLUMN reward_granted INTEGER NOT NULL DEFAULT 0",
                 nullptr, nullptr, nullptr);
}

int GrantPendingRewards(int64_t character_id) {
    sqlite3* db = Database::GetConnection();
    if (!db || character_id == 0) return 0;
    const auto ci = PlayerSessionStore::GetCharacterById(character_id);
    if (!ci || ci->user_id == 0) return 0;
    std::vector<std::pair<int, int>> pending;  // achievement id, loot table id
    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(db,
            "SELECT c.achievement_id, COALESCE(a.loot_table_id, 0) FROM ga_character_achievements c "
            "LEFT JOIN asm_data_set_achievements a ON a.achievement_id = c.achievement_id "
            "WHERE c.character_id = ? AND c.reward_granted = 0", -1, &s, nullptr) != SQLITE_OK) return 0;
    sqlite3_bind_int64(s, 1, character_id);
    while (sqlite3_step(s) == SQLITE_ROW)
        pending.push_back({sqlite3_column_int(s, 0), sqlite3_column_int(s, 1)});
    sqlite3_finalize(s);

    int granted = 0;
    for (const auto& [aid, loot] : pending) {
        // Claim the row first: a crash between claim and grant loses one
        // reward, it can never hand out two.
        if (sqlite3_prepare_v2(db, "UPDATE ga_character_achievements SET reward_granted = 1 "
                "WHERE character_id = ? AND achievement_id = ? AND reward_granted = 0",
                -1, &s, nullptr) != SQLITE_OK) continue;
        sqlite3_bind_int64(s, 1, character_id);
        sqlite3_bind_int(s, 2, aid);
        sqlite3_step(s);
        sqlite3_finalize(s);
        if (sqlite3_changes(db) != 1 || loot <= 0) continue;
        std::map<int, int> bag;
        std::vector<Database::AutoGear> autoGear;  // 0957
        const auto comps = Database::RollLootTable(loot, bag, 0, &autoGear, ci->profile_id);  // 0959
        if (!bag.empty()) EconParity::ApplyAutoGear(Database::GrantLootItems(ci->user_id, bag), autoGear);
        if (!comps.empty()) Database::GrantComponents(ci->user_id, comps);
        for (const auto& kv : bag)
            Logger::Log("rewards", "[Achievements] char=%lld achievement %d reward table %d -> bag item %d x%d\n",
                        (long long)character_id, aid, loot, kv.first, kv.second);
        granted += (int)(bag.size() + comps.size());
    }
    return granted;
}

std::vector<Earned> GetEarned(int64_t character_id) {
    std::vector<Earned> out;
    sqlite3* db = Database::GetConnection();
    if (!db || character_id == 0) return out;
    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(db, "SELECT achievement_id, achieved_at FROM ga_character_achievements "
            "WHERE character_id = ? ORDER BY achieved_at", -1, &s, nullptr) != SQLITE_OK) return out;
    sqlite3_bind_int64(s, 1, character_id);
    while (sqlite3_step(s) == SQLITE_ROW)
        out.push_back({sqlite3_column_int(s, 0), sqlite3_column_int64(s, 1)});
    sqlite3_finalize(s);
    return out;
}

std::vector<Unlock> CreditPveMission(int64_t instance_id, int players_tf,
                                     uint32_t queue_difficulty, bool solo,
                                     bool double_agent, bool won) {
    std::vector<Unlock> out;
    sqlite3* db = Database::GetConnection();
    if (!db || instance_id == 0 || !won) return out;
    std::lock_guard<std::mutex> lk(g_mu);
    LoadLocked(db);

    std::string map_name;
    std::vector<int64_t> chars;
    sqlite3_stmt* s = nullptr;
    bool victory_bonus = false;
    if (sqlite3_prepare_v2(db, "SELECT victory_bonus_lives, count_deaths_attackers, "
            "count_deaths_defenders FROM ga_instances WHERE instance_id = ?", -1, &s, nullptr) == SQLITE_OK) {
        sqlite3_bind_int64(s, 1, instance_id);
        if (sqlite3_step(s) == SQLITE_ROW) {
            const int lives = sqlite3_column_int(s, 0);
            const int deaths = sqlite3_column_int(s, players_tf == 2 ? 2 : 1);
            victory_bonus = lives > 0 && deaths < lives;
        }
        sqlite3_finalize(s);
    }
    if (sqlite3_prepare_v2(db,
            "SELECT i.map_name, p.character_id FROM ga_instances i "
            "JOIN ga_instance_players p ON p.instance_id = i.instance_id "
            "WHERE i.instance_id = ? AND p.left_at IS NULL AND p.character_id > 0 "
            "  AND p.task_force_number = ? GROUP BY p.character_id", -1, &s, nullptr) != SQLITE_OK)
        return out;
    sqlite3_bind_int64(s, 1, instance_id);
    sqlite3_bind_int(s, 2, players_tf);
    while (sqlite3_step(s) == SQLITE_ROW) {
        map_name = reinterpret_cast<const char*>(sqlite3_column_text(s, 0));
        chars.push_back(sqlite3_column_int64(s, 1));
    }
    sqlite3_finalize(s);
    if (chars.empty()) return out;

    // map_game_ids + gameplay type of the map that ran
    std::set<int> map_ids;
    int map_gameplay = 0;
    if (sqlite3_prepare_v2(db, "SELECT map_game_id, gameplay_type_value_id FROM map_game_info "
            "WHERE map_name = ? COLLATE NOCASE", -1, &s, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(s, 1, map_name.c_str(), -1, SQLITE_TRANSIENT);
        while (sqlite3_step(s) == SQLITE_ROW) {
            map_ids.insert(sqlite3_column_int(s, 0));
            if (!map_gameplay) map_gameplay = sqlite3_column_int(s, 1);
        }
        sqlite3_finalize(s);
    }
    const bool defense = map_gameplay == 1550 || players_tf == 2;
    // Solo Spec Ops difficulties are Adept / Advanced / Expert on the client.
    uint32_t diff = queue_difficulty;
    if (solo && (queue_difficulty < 1467 || queue_difficulty > 1470))  // solo tiers 1467-1470 pass through
        diff = queue_difficulty == 1029 ? 1468 : queue_difficulty == 1030 ? 1469 : 1470;
    const int diff_sort = g_sort.count((int)diff) ? g_sort[(int)diff] : 0;

    auto matches = [&](const Req& r, int class_value) {
        if (r.metric != METRIC_WIN && !(r.metric == METRIC_WIN_VB && victory_bonus)) return false;
        if (r.map_game_id && !map_ids.count(r.map_game_id)) return false;
        switch (r.gameplay) {
            case 0:    break;
            case 1550: if (!defense) return false; break;
            case 1551: if (defense) return false; break;
            case 1552: if (!solo || defense) return false; break;
            case 1553: if (solo || defense) return false; break;
            default:   return false;  // PvP merc types
        }
        if (r.class_value && r.class_value != CLASS_ALL && r.class_value != class_value) return false;
        if (r.difficulty == 1260) return double_agent;
        if (r.difficulty) {
            auto it = g_sort.find(r.difficulty);
            if (it == g_sort.end() || diff_sort < it->second) return false;
        }
        return true;
    };

    for (int64_t cid : chars) {
        const CharInfo ci = LoadChar(db, cid);
        for (const Req& r : g_reqs) {
            if (!matches(r, ci.class_value) || Has(db, cid, r.achievement_id)) continue;
            const int64_t n = Bump(db, cid, r.achievement_id);
            const bool reached = r.class_value == CLASS_ALL
                ? AllClassesReached(db, ci.user_id, r.achievement_id, r.value, false)
                : (double)n >= r.value;
            if (reached) Grant(db, cid, r.achievement_id, out);
        }
    }
    Logger::Log("rewards", "[Achievements] instance=%lld map=%s solo=%d da=%d diff=%u vb=%d -> %zu unlock(s)\n",
                (long long)instance_id, map_name.c_str(), solo ? 1 : 0, double_agent ? 1 : 0,
                diff, victory_bonus ? 1 : 0, out.size());
    return out;
}

// "Num Top Stats in Level 30+ Match" (metric 1563, 75 rows): a level 30+ agent who
// finishes a Merc match of that gameplay type (Breach/Control/Acquisition/Payload/
// Scramble) on top of the scoreboard in N stats (kills, assists, damage, buff, healing,
// defense, objectives; ties count, 0 never tops). Class rows = this agent's class; "ALL
// classes" rows = every class reached N on some agent of the account (best N kept per
// agent in ga_achievement_progress). `paid` = agents that passed the PvP reward gate.
std::vector<Unlock> CreditPvpTopStats(int64_t instance_id, const std::vector<int64_t>& paid) {
    std::vector<Unlock> out;
    sqlite3* db = Database::GetConnection();
    if (!db || instance_id == 0 || paid.empty()) return out;
    std::lock_guard<std::mutex> lk(g_mu);
    LoadLocked(db);
    sqlite3_stmt* s = nullptr;
    int gameplay = 0;
    if (sqlite3_prepare_v2(db, "SELECT m.gameplay_type_value_id FROM ga_instances i JOIN map_game_info m "
            "ON m.map_name = i.map_name COLLATE NOCASE WHERE i.instance_id = ? LIMIT 1", -1, &s, nullptr) == SQLITE_OK) {
        sqlite3_bind_int64(s, 1, instance_id);
        if (sqlite3_step(s) == SQLITE_ROW) gameplay = sqlite3_column_int(s, 0);
        sqlite3_finalize(s);
    }
    if (gameplay < 1544 || gameplay > 1548) return out;
    constexpr int kStats = 7;
    std::map<int64_t, std::array<int64_t, kStats>> stats;
    if (sqlite3_prepare_v2(db, "SELECT character_id, SUM(kills), SUM(assists), SUM(damage_dealt), SUM(buff_value), "
            "SUM(healing), SUM(defense), SUM(obj_points) FROM ga_match_player_stats WHERE instance_id = ? "
            "AND character_id > 0 GROUP BY character_id", -1, &s, nullptr) != SQLITE_OK) return out;
    sqlite3_bind_int64(s, 1, instance_id);
    while (sqlite3_step(s) == SQLITE_ROW) {
        auto& a = stats[sqlite3_column_int64(s, 0)];
        for (int k = 0; k < kStats; ++k) a[k] = sqlite3_column_int64(s, k + 1);
    }
    sqlite3_finalize(s);
    std::array<int64_t, kStats> best{};
    for (const auto& [cid, a] : stats) for (int k = 0; k < kStats; ++k) best[k] = std::max(best[k], a[k]);
    for (int64_t cid : std::set<int64_t>(paid.begin(), paid.end())) {
        auto it = stats.find(cid);
        if (it == stats.end() || PlayerSessionStore::GetCharacterLevelXp(cid).level < 30) continue;
        int tops = 0;
        for (int k = 0; k < kStats; ++k) if (best[k] > 0 && it->second[k] >= best[k]) ++tops;
        const CharInfo ci = LoadChar(db, cid);
        Logger::Log("rewards", "[Achievements] pvp top stats instance=%lld char=%lld gameplay=%d tops=%d\n",
                    (long long)instance_id, (long long)cid, gameplay, tops);
        if (tops == 0 || !ci.class_value) continue;
        for (const Req& r : g_reqs) {
            if (r.metric != METRIC_TOP_STATS_30 || r.gameplay != gameplay || Has(db, cid, r.achievement_id)) continue;
            if (r.class_value == CLASS_ALL) {
                if (sqlite3_prepare_v2(db, "INSERT INTO ga_achievement_progress (character_id, achievement_id, count) "
                        "VALUES (?1, ?2, ?3) ON CONFLICT(character_id, achievement_id) DO UPDATE SET "
                        "count = MAX(count, ?3)", -1, &s, nullptr) == SQLITE_OK) {
                    sqlite3_bind_int64(s, 1, cid); sqlite3_bind_int(s, 2, r.achievement_id); sqlite3_bind_int(s, 3, tops);
                    sqlite3_step(s); sqlite3_finalize(s);
                }
                if (AllClassesReached(db, ci.user_id, r.achievement_id, r.value, false)) Grant(db, cid, r.achievement_id, out);
            } else if (r.class_value == ci.class_value && tops >= r.value) {
                Grant(db, cid, r.achievement_id, out);
            }
        }
    }
    return out;
}

// AvA season end - rank 0 = season winner ("Win N AvA seasons", metric 1577) and
// every top-3 rank counts for "Finish Top 3 in AvA (N times)" (metric 1568).
// Generic metric counter, e.g. 1588 Team Challenge Participation.
std::vector<Unlock> CreditMetric(int64_t character_id, int metric) {
    std::vector<Unlock> out;
    sqlite3* db = Database::GetConnection();
    if (!db || character_id <= 0 || metric <= 0) return out;
    std::lock_guard<std::mutex> lk(g_mu);
    LoadLocked(db);
    for (const Req& r : g_reqs) {
        if (r.metric != metric || Has(db, character_id, r.achievement_id)) continue;
        if ((double)Bump(db, character_id, r.achievement_id) >= r.value) Grant(db, character_id, r.achievement_id, out);
    }
    return out;
}

std::vector<Unlock> CreditAvaSeason(int64_t character_id, int rank) {
    std::vector<Unlock> out;
    sqlite3* db = Database::GetConnection();
    if (!db || character_id <= 0 || rank < 0 || rank > 2) return out;
    std::lock_guard<std::mutex> lk(g_mu);
    LoadLocked(db);
    for (const Req& r : g_reqs) {
        if (!(r.metric == METRIC_AVA_TOP3 || (r.metric == METRIC_AVA_FIRST && rank == 0))) continue;
        if (Has(db, character_id, r.achievement_id)) continue;
        if ((double)Bump(db, character_id, r.achievement_id) >= r.value) Grant(db, character_id, r.achievement_id, out);
    }
    return out;
}

std::vector<Unlock> CreditAvaMatch(int64_t instance_id) {
    std::vector<Unlock> out;
    sqlite3* db = Database::GetConnection();
    if (!db || instance_id == 0) return out;
    std::lock_guard<std::mutex> lk(g_mu);
    LoadLocked(db);
    std::vector<int64_t> chars;
    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(db, "SELECT DISTINCT character_id FROM ga_instance_players "
            "WHERE instance_id = ? AND left_at IS NULL AND character_id > 0", -1, &s, nullptr) != SQLITE_OK)
        return out;
    sqlite3_bind_int64(s, 1, instance_id);
    while (sqlite3_step(s) == SQLITE_ROW) chars.push_back(sqlite3_column_int64(s, 0));
    sqlite3_finalize(s);
    for (int64_t cid : chars)
        for (const Req& r : g_reqs)
            if (r.metric == METRIC_AVA_MATCH && !Has(db, cid, r.achievement_id) &&
                (double)Bump(db, cid, r.achievement_id) >= r.value)
                Grant(db, cid, r.achievement_id, out);
    return out;
}

std::vector<Unlock> CheckStarRating(int64_t character_id) {
    std::vector<Unlock> out;
    sqlite3* db = Database::GetConnection();
    if (!db || character_id == 0) return out;
    std::lock_guard<std::mutex> lk(g_mu);
    LoadLocked(db);
    const CharInfo ci = LoadChar(db, character_id);
    const double stars = (double)CombatRating::Stars(character_id, PlayerSessionStore::GetCharacterLevelXp(character_id).level);
    for (const Req& r : g_reqs) {
        if (r.metric != METRIC_STAR_RATING || Has(db, character_id, r.achievement_id)) continue;
        const bool reached = r.class_value == CLASS_ALL
            ? AllClassesReached(db, ci.user_id, r.achievement_id, r.value, true)
            : stars >= r.value;
        if (reached) Grant(db, character_id, r.achievement_id, out);
    }
    return out;
}

}  // namespace AchievementService
