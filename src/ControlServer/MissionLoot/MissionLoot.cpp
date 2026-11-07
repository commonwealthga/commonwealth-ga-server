#include "src/ControlServer/MissionLoot/MissionLoot.hpp"
#include "src/ControlServer/EconParity/EconParity.hpp"
#include "src/ControlServer/ApRewards/ApRewards.hpp"
#include "src/ControlServer/Database/Database.hpp"
#include "src/ControlServer/PlayerSessionStore/PlayerSessionStore.hpp"
#include "src/ControlServer/Logger.hpp"
#include "sqlite3.h"
#include <algorithm>
#include <map>
#include <random>
#include <utility>
#include <vector>

namespace MissionLoot {
namespace {

std::mt19937& Rng() { static thread_local std::mt19937 r{std::random_device{}()}; return r; }

// One row of a pick-one table (drop_chance = weight; the reward tables and the
// Master Armor and Weapons sub-tables sum to 100). Recurses into sub-tables.
// auto_create_flag rows give the blueprint's crafted item (as the original did).
// max_level > 0: rows whose item (or crafted item) needs a higher level, and
// sub-tables with no such row left, are not candidates.
struct Row { double w; int item; int qty; int sub; int type; int bp = 0; int maxq = 0; };  // bp = creating blueprint, maxq = row max quality

std::vector<Row> LoadRows(sqlite3* db, int table, int depth, int max_level) {
    std::vector<Row> rows;
    if (table <= 0 || depth > 4) return rows;
    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(db,
            "SELECT CASE WHEN COALESCE(li.auto_create_flag,0) = 1 AND COALESCE(b.created_item_id,0) > 0 "
            "            THEN b.created_item_id ELSE li.item_id END, "
            "li.quantity, COALESCE(li.sub_loot_table_id,0), li.drop_chance, "
            "CASE WHEN COALESCE(li.auto_create_flag,0) = 1 AND COALESCE(b.created_item_id,0) > 0 "
            "     THEN -1 ELSE COALESCE(i.item_type_value_id,0) END, "
            "MAX(COALESCE(i.required_level,0), COALESCE(c.required_level,0)), "
            "CASE WHEN COALESCE(li.auto_create_flag,0) = 1 AND COALESCE(b.created_item_id,0) > 0 THEN li.item_id ELSE 0 END, "
            "COALESCE(li.max_quality_value_id,0) "
            "FROM asm_data_set_loot_table_items li "
            "LEFT JOIN asm_data_set_items i ON i.item_id = li.item_id "
            "LEFT JOIN asm_data_set_blueprints b ON b.blueprint_id = li.item_id "
            "LEFT JOIN asm_data_set_items c ON c.item_id = b.created_item_id "
            "WHERE li.loot_table_id = ? ORDER BY li.sort_order, li.id", -1, &s, nullptr) != SQLITE_OK)
        return rows;
    sqlite3_bind_int(s, 1, table);
    while (sqlite3_step(s) == SQLITE_ROW) {
        Row r{std::max(0.0, sqlite3_column_double(s, 3)), sqlite3_column_int(s, 0),
              std::max(1, sqlite3_column_int(s, 1)), sqlite3_column_int(s, 2), sqlite3_column_int(s, 4),
              sqlite3_column_int(s, 6), sqlite3_column_int(s, 7)};
        if (r.item <= 0 && r.sub <= 0) continue;
        if (max_level > 0 && r.sub <= 0 && sqlite3_column_int(s, 5) > max_level) continue;
        rows.push_back(r);
    }
    sqlite3_finalize(s);
    if (max_level > 0)
        rows.erase(std::remove_if(rows.begin(), rows.end(), [&](const Row& r) {
            return r.sub > 0 && LoadRows(db, r.sub, depth + 1, max_level).empty(); }), rows.end());
    return rows;
}

void PickInto(sqlite3* db, int table, int depth, std::map<int, int>& bag,
              std::map<int, int>& comps, int max_level = 0,
              std::vector<EconParity::GearDrop>* gear = nullptr) {
    std::vector<Row> rows = LoadRows(db, table, depth, max_level);
    if (rows.empty()) return;
    double total = 0;
    for (const auto& r : rows) total += r.w;
    if (total <= 0) { for (auto& r : rows) r.w = 1; total = (double)rows.size(); }
    double x = std::uniform_real_distribution<double>(0.0, total)(Rng());
    const Row* pick = &rows.back();
    for (const auto& r : rows) { if (x < r.w) { pick = &r; break; } x -= r.w; }
    if (pick->sub > 0) { PickInto(db, pick->sub, depth + 1, bag, comps, max_level, gear); return; }
    if (gear && pick->type == -1 && pick->bp > 0 && pick->maxq > 0)
        for (int k = 0; k < pick->qty; ++k) gear->push_back({pick->item, pick->bp, pick->maxq});
    if (pick->type == 1171) comps[pick->item] += pick->qty;
    // Also 1020 dyes (UMax Rainbow 388, Dome Defense 475, PvP 30+ Tier-5 477).
    else if (pick->type == -1 || pick->type == 686 || pick->type == 950 || pick->type == 1612 ||
             pick->type == 1020)
        bag[pick->item] += pick->qty;
}

}  // namespace

void EnsureSchema() {
    sqlite3* db = Database::GetConnection();
    if (!db) return;
    sqlite3_exec(db,
        "CREATE TABLE IF NOT EXISTS ga_pending_mission_loot ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT, character_id INTEGER NOT NULL,"
        "  instance_id INTEGER NOT NULL, loot_table_id INTEGER NOT NULL,"
        "  created_at INTEGER NOT NULL, granted INTEGER NOT NULL DEFAULT 0);"
        "CREATE INDEX IF NOT EXISTS ix_pending_mission_loot ON ga_pending_mission_loot(character_id, granted);"
        "CREATE TABLE IF NOT EXISTS ga_currency_grants ("
        "  character_id INTEGER NOT NULL, ref INTEGER NOT NULL, xp INTEGER NOT NULL,"
        "  credits INTEGER NOT NULL, tokens INTEGER NOT NULL, created_at INTEGER NOT NULL,"
        "  PRIMARY KEY (character_id, ref));",
        nullptr, nullptr, nullptr);
    // 0 = no level cap (fails harmlessly once the column exists).
    sqlite3_exec(db, "ALTER TABLE ga_pending_mission_loot ADD COLUMN max_level INTEGER NOT NULL DEFAULT 0",
                 nullptr, nullptr, nullptr);
}

int TableFor(uint32_t difficulty, int players, const std::string& queue_name,
             uint32_t queue_type) {
    if (queue_name == "ddr") return 474;                       // *PVE GROUP DEFENSE DOME RAID
    if (queue_type == 1454) return 341;                        // *PVE GROUP DEFENSE RAID
    if (queue_name.rfind("double_agent", 0) == 0 || difficulty == 1260) return 349;  // DOUBLEAGENT NEW
    // {full group, 3P, 2P (also solo)} per security level.
    auto grp = [&](int full, int p3, int p2) { return players >= 4 ? full : (players == 3 ? p3 : p2); };
    switch (difficulty) {
        case 1467: return 234;  // Novice   -> *PVE SOLO NOVICE
        case 1468: return 235;  // Adept    -> *PVE SOLO ROOKIE
        case 1469: return 236;  // Advanced -> *PVE SOLO ADVANCED
        case 1470: return 237;  // Expert   -> *PVE SOLO EXPERT
        case 1028: return grp(208, 242, 243);  // Low Security
        case 1029: return grp(230, 244, 245);  // Medium
        case 1030: return grp(204, 246, 247);  // High
        case 1259: return grp(228, 248, 249);  // Maximum
        case 1471: return grp(233, 250, 251);  // Ultra-Max
        default: return 0;
    }
}

int QueueForInstance(int64_t instance_id, int players_tf, int loot_table_id,
                     const std::vector<int64_t>& exclude) {
    sqlite3* db = Database::GetConnection();
    if (!db || instance_id == 0 || loot_table_id <= 0) return 0;
    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(db,
            "INSERT INTO ga_pending_mission_loot (character_id, instance_id, loot_table_id, created_at) "
            "SELECT DISTINCT character_id, instance_id, ?3, strftime('%s','now') FROM ga_instance_players "
            "WHERE instance_id = ?1 AND task_force_number = ?2 AND left_at IS NULL AND character_id > 0 "
            "AND character_id NOT IN (SELECT character_id FROM ga_pending_mission_loot "
            "                         WHERE instance_id = ?1 AND loot_table_id = ?3)",
            -1, &s, nullptr) != SQLITE_OK) return 0;
    sqlite3_bind_int64(s, 1, instance_id);
    sqlite3_bind_int(s, 2, players_tf);
    sqlite3_bind_int(s, 3, loot_table_id);
    sqlite3_step(s);
    sqlite3_finalize(s);
    int n = sqlite3_changes(db);
    for (int64_t cid : exclude) {
        sqlite3_stmt* d = nullptr;
        if (sqlite3_prepare_v2(db, "DELETE FROM ga_pending_mission_loot "
                "WHERE instance_id = ? AND character_id = ? AND granted = 0",
                -1, &d, nullptr) != SQLITE_OK) continue;
        sqlite3_bind_int64(d, 1, instance_id);
        sqlite3_bind_int64(d, 2, cid);
        sqlite3_step(d);
        n -= sqlite3_changes(db);
        sqlite3_finalize(d);
    }
    Logger::Log("rewards", "[MissionLoot] instance=%lld tf=%d table=%d -> %d roll(s) queued\n",
                (long long)instance_id, players_tf, loot_table_id, n);
    return n;
}

int QueueForCharacter(int64_t character_id, int64_t ref, int loot_table_id, int max_level) {
    sqlite3* db = Database::GetConnection();
    if (!db || character_id <= 0 || ref == 0 || loot_table_id <= 0) return 0;
    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(db,
            "INSERT INTO ga_pending_mission_loot (character_id, instance_id, loot_table_id, created_at, max_level) "
            "SELECT ?1, ?2, ?3, strftime('%s','now'), ?4 "
            "WHERE EXISTS (SELECT 1 FROM ga_characters WHERE id = ?1) "
            "AND NOT EXISTS (SELECT 1 FROM ga_pending_mission_loot "
            "                WHERE character_id = ?1 AND instance_id = ?2 AND loot_table_id = ?3)",
            -1, &s, nullptr) != SQLITE_OK) return 0;
    sqlite3_bind_int64(s, 1, character_id);
    sqlite3_bind_int64(s, 2, ref);
    sqlite3_bind_int(s, 3, loot_table_id);
    sqlite3_bind_int(s, 4, std::max(0, max_level));
    sqlite3_step(s);
    sqlite3_finalize(s);
    const int n = sqlite3_changes(db);
    Logger::Log("rewards", "[MissionLoot] char=%lld ref=%lld table=%d max_level=%d -> %d roll queued\n",
                (long long)character_id, (long long)ref, loot_table_id, max_level, n);
    return n;
}

int GrantCurrency(int64_t character_id, int64_t ref, int64_t xp, int64_t credits, int64_t tokens,
                  int64_t ap) {
    sqlite3* db = Database::GetConnection();
    if (!db || character_id <= 0 || ref == 0) return 0;
    xp = std::clamp<int64_t>(xp, 0, 100000);
    credits = std::clamp<int64_t>(credits, 0, 100000);
    tokens = std::clamp<int64_t>(tokens, 0, 100000);
    ap = std::clamp<int64_t>(ap, 0, 1000);
    sqlite3_stmt* s = nullptr;
    // Dedupe row first (live agents only); the add runs only if it was new.
    if (sqlite3_prepare_v2(db,
            "INSERT OR IGNORE INTO ga_currency_grants (character_id, ref, xp, credits, tokens, created_at) "
            "SELECT ?1, ?2, ?3, ?4, ?5, strftime('%s','now') "
            "WHERE EXISTS (SELECT 1 FROM ga_characters WHERE id = ?1 AND deleted_at = 0)",
            -1, &s, nullptr) != SQLITE_OK) return 0;
    sqlite3_bind_int64(s, 1, character_id);
    sqlite3_bind_int64(s, 2, ref);
    sqlite3_bind_int64(s, 3, xp);
    sqlite3_bind_int64(s, 4, credits);
    sqlite3_bind_int64(s, 5, tokens);
    sqlite3_step(s);
    sqlite3_finalize(s);
    if (sqlite3_changes(db) != 1) return 0;
    // Relative add, xp capped at the top level (same as mission-end rewards).
    if (sqlite3_prepare_v2(db,
            "UPDATE ga_characters SET "
            "  xp = xp + ?1, "  // cap + lv50 credits = trigger trg_lv50_xp_credits
            ""
            "  credits = credits + ?2, tokens = tokens + ?3 WHERE id = ?4",
            -1, &s, nullptr) != SQLITE_OK) return 0;
    sqlite3_bind_int64(s, 1, xp);
    sqlite3_bind_int64(s, 2, credits);
    sqlite3_bind_int64(s, 3, tokens);
    sqlite3_bind_int64(s, 4, character_id);
    sqlite3_step(s);
    sqlite3_finalize(s);
    const int n = sqlite3_changes(db);
    // Account-wide AP through the AP ledger (uncapped, same dedupe as above).
    if (n == 1 && ap > 0)
        ApRewards::Grant(character_id, PlayerSessionStore::GetCharacterLevelXp(character_id).level,
                         "currency_grant", ap, false, false);
    Logger::Log("rewards", "[MissionLoot] char=%lld ref=%lld +xp %lld +cr %lld +tok %lld -> %d\n",
                (long long)character_id, (long long)ref, (long long)xp, (long long)credits,
                (long long)tokens, n);
    return n;
}

int QueuePvpWin(int64_t instance_id, int winning_task_force, bool arena,
                const std::vector<int64_t>& paid) {
    sqlite3* db = Database::GetConnection();
    if (!db || instance_id == 0 || (winning_task_force != 1 && winning_task_force != 2)) return 0;
    int n = 0;
    for (int64_t cid : paid) {
        sqlite3_stmt* s = nullptr;
        if (sqlite3_prepare_v2(db,
                "SELECT 1 FROM ga_instance_players WHERE instance_id = ? AND character_id = ? "
                "AND left_at IS NULL AND task_force_number = ? LIMIT 1", -1, &s, nullptr) != SQLITE_OK) continue;
        sqlite3_bind_int64(s, 1, instance_id);
        sqlite3_bind_int64(s, 2, cid);
        sqlite3_bind_int(s, 3, winning_task_force);
        const bool winner = sqlite3_step(s) == SQLITE_ROW;
        sqlite3_finalize(s);
        if (!winner) continue;
        const int level = PlayerSessionStore::GetCharacterLevelXp(cid).level;
        // Final data: *PVP ARENA 348, *PVP LOOT TABLE (1-29) 231, (30+) 473 (2% Tier-5 dye 477).
        n += QueueForCharacter(cid, instance_id, arena ? 348 : (level >= 30 ? 473 : 231), 0);
    }
    Logger::Log("rewards", "[MissionLoot] pvp instance=%lld tf=%d arena=%d -> %d roll(s) queued\n",
                (long long)instance_id, winning_task_force, arena ? 1 : 0, n);
    return n;
}

// Display name of a granted item (game data), "" if unknown.
static std::string ItemDisplayName(sqlite3* db, int item_id) {
    std::string n; sqlite3_stmt* q = nullptr;
    if (sqlite3_prepare_v2(db, "SELECT COALESCE(NULLIF(name_msg_translated,''), (SELECT message FROM asm_data_set_msg_translations m WHERE m.msg_id = name_msg_id), '') FROM asm_data_set_items WHERE item_id = ? LIMIT 1",
            -1, &q, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(q, 1, item_id);
        if (sqlite3_step(q) == SQLITE_ROW && sqlite3_column_text(q, 0))
            n = reinterpret_cast<const char*>(sqlite3_column_text(q, 0));
    }
    sqlite3_finalize(q);
    return n;
}

std::string CraftedQualityName(const std::vector<int64_t>& rows, int item_id) {
    sqlite3* db = Database::GetConnection();
    if (!db) return "Common";
    std::string n;
    sqlite3_stmt* q = nullptr;
    if (sqlite3_prepare_v2(db, "SELECT text_msg_translated FROM asm_data_set_valid_values WHERE value_id = "
            "COALESCE((SELECT quality FROM ga_players_inventory WHERE id = ?1 AND quality IN (1162,1163,1164,1165)), "
            "(SELECT quality_value_id FROM asm_data_set_items WHERE item_id = ?2)) LIMIT 1", -1, &q, nullptr) == SQLITE_OK) {
        sqlite3_bind_int64(q, 1, rows.empty() ? 0 : rows.front());
        sqlite3_bind_int(q, 2, item_id);
        if (sqlite3_step(q) == SQLITE_ROW && sqlite3_column_text(q, 0)) n = (const char*)sqlite3_column_text(q, 0);
    }
    sqlite3_finalize(q);
    return n.empty() ? "Common" : n;
}

std::string ItemName(int item_id) {
    sqlite3* db = Database::GetConnection();
    return db ? ItemDisplayName(db, item_id) : std::string();
}

int GrantPending(int64_t character_id, std::vector<std::pair<int, int>>* items) {
    sqlite3* db = Database::GetConnection();
    if (!db || character_id == 0) return 0;
    const auto ci = PlayerSessionStore::GetCharacterById(character_id);
    if (!ci || ci->user_id == 0) return 0;
    struct Pending { int64_t rid; int table; int max_level; };
    std::vector<Pending> pending;
    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(db, "SELECT id, loot_table_id, max_level FROM ga_pending_mission_loot "
            "WHERE character_id = ? AND granted = 0", -1, &s, nullptr) != SQLITE_OK) return 0;
    sqlite3_bind_int64(s, 1, character_id);
    while (sqlite3_step(s) == SQLITE_ROW)
        pending.push_back({sqlite3_column_int64(s, 0), sqlite3_column_int(s, 1), sqlite3_column_int(s, 2)});
    sqlite3_finalize(s);

    int granted = 0;
    for (const auto& [rid, table, max_level] : pending) {
        // Claim first: a crash between claim and grant loses one roll, never doubles it.
        if (sqlite3_prepare_v2(db, "UPDATE ga_pending_mission_loot SET granted = 1 WHERE id = ? AND granted = 0",
                -1, &s, nullptr) != SQLITE_OK) continue;
        sqlite3_bind_int64(s, 1, rid);
        sqlite3_step(s);
        sqlite3_finalize(s);
        if (sqlite3_changes(db) != 1) continue;
        std::map<int, int> bag, comps;
        std::vector<EconParity::GearDrop> gear;  // Rare/Epic tables -> row quality + rolled stats
        PickInto(db, table, 0, bag, comps, max_level, &gear);
        if (!bag.empty()) EconParity::ApplyGearDrops(Database::GrantLootItems(ci->user_id, bag), gear);
        if (!comps.empty()) Database::GrantComponents(ci->user_id, comps);
        for (const auto& kv : bag)
            Logger::Log("rewards", "[MissionLoot] char=%lld table %d -> bag item %d x%d\n",
                        (long long)character_id, table, kv.first, kv.second);
        if (items)
            for (const auto* m : {&bag, &comps})
                for (const auto& kv : *m) items->emplace_back(kv.first, kv.second);
        granted += (int)(bag.size() + comps.size());
    }
    return granted;
}

}  // namespace MissionLoot
