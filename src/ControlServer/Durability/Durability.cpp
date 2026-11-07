#include "src/ControlServer/Durability/Durability.hpp"
#include "src/ControlServer/Database/Database.hpp"
#include "src/ControlServer/Logger.hpp"
#include "src/ControlServer/Loadouts/ArmorLoadouts.hpp"
#include "src/ControlServer/PlayerSessionStore/PlayerSessionStore.hpp"
#include "sqlite3.h"
#include <algorithm>
#include <map>
#include <string>
#include <mutex>
#include <set>

// See Durability.hpp for the rules and their sources.
namespace Durability {
namespace {

constexpr int kRepairKitType = 1369;   // CONST_TGITC_REPAIR_KIT
constexpr int kDefaultMax    = 100;

int SettingInt(sqlite3* db, const char* key, int def) {
    sqlite3_stmt* s = nullptr;
    int v = def;
    if (sqlite3_prepare_v2(db, "SELECT value FROM cs_settings WHERE key = ?", -1, &s, nullptr) != SQLITE_OK) return def;
    sqlite3_bind_text(s, 1, key, -1, SQLITE_TRANSIENT);
    if (sqlite3_step(s) == SQLITE_ROW && sqlite3_column_type(s, 0) != SQLITE_NULL) v = sqlite3_column_int(s, 0);
    sqlite3_finalize(s);
    return v;
}

bool Enabled(sqlite3* db) { return SettingInt(db, "durability_enabled", 1) != 0; }

int BlueprintDurability(sqlite3* db, int blueprint_id) {
    if (blueprint_id <= 0) return 0;
    static std::mutex m;
    static std::map<int, int> cache;
    {
        std::lock_guard<std::mutex> lk(m);
        auto it = cache.find(blueprint_id);
        if (it != cache.end()) return it->second;
    }
    int d = 0;
    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(db, "SELECT COALESCE(durability, 0) FROM asm_data_set_blueprints WHERE blueprint_id = ? LIMIT 1",
                           -1, &s, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(s, 1, blueprint_id);
        if (sqlite3_step(s) == SQLITE_ROW) d = sqlite3_column_int(s, 0);
        sqlite3_finalize(s);
    }
    std::lock_guard<std::mutex> lk(m);
    cache[blueprint_id] = d;
    return d;
}

struct Row {
    bool found = false;
    int64_t id = 0;
    int item_id = 0, device_id = 0, quality = 0, durability = -1;
    bool oc = false, has_mods = false;
};

// Same rows/columns for every reader. Quality: the row's crafted quality, else
// the item's own (what the client compares the kit against).
const char* kRowCols =
    "p.id, p.item_id, p.device_id, "
    "  CASE WHEN p.quality IN (1162,1163,1164,1165) THEN p.quality ELSE COALESCE(it.quality_value_id, 0) END, "
    "  COALESCE(p.durability, -1), p.oc, (COALESCE(p.mod_effect_group_ids, '') NOT IN ('', '0')) ";
const char* kRowJoin =
    "LEFT JOIN asm_data_set_items it ON it.item_id = (CASE WHEN p.item_id > 0 THEN p.item_id ELSE p.device_id END) ";

Row ReadRow(sqlite3_stmt* s) {
    Row r;
    r.found = true;
    r.id = sqlite3_column_int64(s, 0);
    r.item_id = sqlite3_column_int(s, 1);
    r.device_id = sqlite3_column_int(s, 2);
    r.quality = sqlite3_column_int(s, 3);
    r.durability = sqlite3_column_int(s, 4);
    r.oc = sqlite3_column_int(s, 5) != 0;
    r.has_mods = sqlite3_column_int(s, 6) != 0;
    return r;
}

// The blueprint TcpSession sends for this row (same rule as the inventory
// records) -> its durability is the max the client shows.
int MaxFor(sqlite3* db, const Row& r) {
    if (!r.has_mods) return 0;
    const int bp = r.item_id > 0 ? PlayerSessionStore::GetBlueprintIdForStatItem(r.item_id)
                                 : PlayerSessionStore::GetBlueprintIdForDevice(r.device_id, r.oc);
    return BlueprintDurability(db, bp);
}

bool WearQuality(int q) { return q == 1162 || q == 1163 || q == 1164; }

bool Wearable(sqlite3* db, const Row& r, int* max_out) {
    const int mx = MaxFor(db, r);
    if (max_out) *max_out = mx;
    return WearQuality(r.quality) && r.has_mods && mx > 0;
}

bool SetDurability(sqlite3* db, int64_t id, int value) {
    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(db, "UPDATE ga_players_inventory SET durability = ? WHERE id = ?", -1, &s, nullptr) != SQLITE_OK)
        return false;
    sqlite3_bind_int(s, 1, value);
    sqlite3_bind_int64(s, 2, id);
    const bool ok = sqlite3_step(s) == SQLITE_DONE && sqlite3_changes(db) == 1;
    sqlite3_finalize(s);
    return ok;
}

}  // namespace

void EnsureSchema() {
    sqlite3* db = Database::GetConnection();
    if (!db) return;
    bool have = false;
    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(db, "PRAGMA table_info(ga_players_inventory)", -1, &s, nullptr) == SQLITE_OK) {
        while (sqlite3_step(s) == SQLITE_ROW) {
            const unsigned char* n = sqlite3_column_text(s, 1);
            if (n && std::string((const char*)n) == "durability") have = true;
        }
        sqlite3_finalize(s);
    }
    // -1 = full: every item that exists before durability tracking starts at full durability.
    if (!have) {
        char* err = nullptr;
        if (sqlite3_exec(db, "ALTER TABLE ga_players_inventory ADD COLUMN durability INTEGER NOT NULL DEFAULT -1",
                         nullptr, nullptr, &err) != SQLITE_OK) {
            Logger::Log("db", "[Durability] add column failed: %s\n", err ? err : "?");
            sqlite3_free(err);
            return;
        }
        Logger::Log("db", "[Durability] ga_players_inventory.durability added (all rows full)\n");
    }
    sqlite3_exec(db, "INSERT OR IGNORE INTO cs_settings(key, value) VALUES ('durability_enabled', '1');"
                     "INSERT OR IGNORE INTO cs_settings(key, value) VALUES ('durability_wear_per_mission', '1');",
                 nullptr, nullptr, nullptr);
}

int ForWire(int64_t inventory_id, int blueprint_id) {
    sqlite3* db = Database::GetConnection();
    if (!db) return kDefaultMax;
    int mx = BlueprintDurability(db, blueprint_id);
    if (mx <= 0) mx = kDefaultMax;   // no durability on the client side anyway
    if (!Enabled(db)) return mx;
    int cur = -1;
    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(db, "SELECT COALESCE(durability, -1) FROM ga_players_inventory WHERE id = ?", -1, &s, nullptr) == SQLITE_OK) {
        sqlite3_bind_int64(s, 1, inventory_id);
        if (sqlite3_step(s) == SQLITE_ROW) cur = sqlite3_column_int(s, 0);
        sqlite3_finalize(s);
    }
    return cur < 0 ? mx : std::min(cur, mx);
}

int WearForInstance(int64_t instance_id) {
    sqlite3* db = Database::GetConnection();
    if (!db || instance_id <= 0 || !Enabled(db)) return 0;
    const int wear = std::max(0, SettingInt(db, "durability_wear_per_mission", 1));
    if (wear == 0) return 0;

    std::set<int64_t> chars;
    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(db, "SELECT DISTINCT character_id FROM ga_instance_players "
                               "WHERE instance_id = ? AND character_id > 0", -1, &s, nullptr) == SQLITE_OK) {
        sqlite3_bind_int64(s, 1, instance_id);
        while (sqlite3_step(s) == SQLITE_ROW) chars.insert(sqlite3_column_int64(s, 0));
        sqlite3_finalize(s);
    }
    int changed = 0;
    const std::string sql = std::string("SELECT DISTINCT ") + kRowCols +
        "FROM ga_character_devices d "
        "JOIN ga_characters c ON c.id = d.character_id AND d.item_profile_id = c.current_item_profile_id "
        "JOIN ga_players_inventory p ON p.id = d.inventory_id " + kRowJoin +
        "WHERE d.character_id = ?";
    for (int64_t cid : chars) {
        std::vector<std::pair<Row, int>> rows;
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &s, nullptr) != SQLITE_OK) {
            Logger::Log("db", "[Durability] wear query failed: %s\n", sqlite3_errmsg(db));
            return changed;
        }
        sqlite3_bind_int64(s, 1, cid);
        while (sqlite3_step(s) == SQLITE_ROW) {
            Row r = ReadRow(s);
            int mx = 0;
            if (Wearable(db, r, &mx)) rows.emplace_back(r, mx);
        }
        sqlite3_finalize(s);
        std::string line;
        for (const auto& [r, mx] : rows) {
            const int cur = r.durability < 0 ? mx : std::min(r.durability, mx);
            const int next = std::max(0, cur - wear);
            if (next == cur || !SetDurability(db, r.id, next)) continue;
            ++changed;
            line += " " + std::to_string(r.id) + ":" + std::to_string(cur) + "->" + std::to_string(next);
        }
        if (!line.empty())
            Logger::Log("loot", "[Durability] wear inst=%lld char=%lld%s\n", (long long)instance_id, (long long)cid, line.c_str());
    }
    return changed;
}

RepairResult Repair(int64_t user_id, int64_t character_id, int64_t target_row) {
    RepairResult r;
    r.target_row = target_row;
    sqlite3* db = Database::GetConnection();
    if (!db || user_id <= 0 || target_row <= 0) { r.reason = "no item"; return r; }

    Row t;
    sqlite3_stmt* s = nullptr;
    const std::string q = std::string("SELECT ") + kRowCols + "FROM ga_players_inventory p " + kRowJoin +
                          "WHERE p.id = ? AND p.user_id = ?";
    if (sqlite3_prepare_v2(db, q.c_str(), -1, &s, nullptr) == SQLITE_OK) {
        sqlite3_bind_int64(s, 1, target_row);
        sqlite3_bind_int64(s, 2, user_id);
        if (sqlite3_step(s) == SQLITE_ROW) t = ReadRow(s);
        sqlite3_finalize(s);
    }
    int mx = 0;
    if (!t.found) r.reason = "item not found";
    else if (!Wearable(db, t, &mx)) r.reason = "this item has no durability";
    else if (t.durability < 0 || t.durability >= mx) r.reason = "this item is already at full durability";
    if (!r.reason.empty()) {
        Logger::Log("loot", "[Durability] repair user=%lld char=%lld inv=%lld -> REFUSED (%s)\n",
            (long long)user_id, (long long)character_id, (long long)target_row, r.reason.c_str());
        return r;
    }

    // One kit of the same quality from the bag (not equipped anywhere).
    if (sqlite3_prepare_v2(db,
            "SELECT p.id FROM ga_players_inventory p JOIN asm_data_set_items i ON i.item_id = p.item_id "
            "WHERE p.user_id = ? AND i.item_type_value_id = ? AND i.quality_value_id = ? "
            "  AND NOT EXISTS (SELECT 1 FROM ga_character_devices d WHERE d.inventory_id = p.id) "
            "ORDER BY p.id LIMIT 1", -1, &s, nullptr) == SQLITE_OK) {
        sqlite3_bind_int64(s, 1, user_id);
        sqlite3_bind_int(s, 2, kRepairKitType);
        sqlite3_bind_int(s, 3, t.quality);
        if (sqlite3_step(s) == SQLITE_ROW) r.kit_row = sqlite3_column_int64(s, 0);
        sqlite3_finalize(s);
    }
    if (r.kit_row == 0) {
        r.reason = "you don't have a valid repair kit";
        Logger::Log("loot", "[Durability] repair user=%lld char=%lld inv=%lld q=%d -> REFUSED (%s)\n",
            (long long)user_id, (long long)character_id, (long long)target_row, t.quality, r.reason.c_str());
        return r;
    }

    sqlite3_exec(db, "BEGIN IMMEDIATE;", nullptr, nullptr, nullptr);
    bool ok = false;
    if (sqlite3_prepare_v2(db, "DELETE FROM ga_players_inventory WHERE id = ? AND user_id = ?", -1, &s, nullptr) == SQLITE_OK) {
        sqlite3_bind_int64(s, 1, r.kit_row);
        sqlite3_bind_int64(s, 2, user_id);
        ok = sqlite3_step(s) == SQLITE_DONE && sqlite3_changes(db) == 1;
        sqlite3_finalize(s);
    }
    ok = ok && SetDurability(db, target_row, -1);
    sqlite3_exec(db, ok ? "COMMIT;" : "ROLLBACK;", nullptr, nullptr, nullptr);
    r.ok = ok;
    if (!ok) r.reason = "db write failed";
    Logger::Log("loot", "[Durability] repair user=%lld char=%lld inv=%lld q=%d %d/%d kit=%lld -> %s\n",
        (long long)user_id, (long long)character_id, (long long)target_row, t.quality, t.durability, mx,
        (long long)r.kit_row, ok ? "OK (full)" : "FAILED");
    return r;
}

}  // namespace Durability
