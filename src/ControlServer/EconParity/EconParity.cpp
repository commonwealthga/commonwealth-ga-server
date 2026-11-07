#include "src/ControlServer/EconParity/EconParity.hpp"
#include "src/ControlServer/Database/Database.hpp"
#include "src/ControlServer/Logger.hpp"
#include "src/ControlServer/MissionLoot/MissionLoot.hpp"
#include "src/ControlServer/PlayerSessionStore/PlayerSessionStore.hpp"
#include <algorithm>
#include <ctime>
#include <mutex>
#include <random>
#include "sqlite3.h"
#include <map>
#include <sstream>

namespace EconParity {
namespace {

// Mesh type of an asm id (0 = unknown / no asm table yet).
int MeshType(sqlite3* db, uint32_t asm_id) {
    sqlite3_stmt* s = nullptr;
    int t = 0;
    if (sqlite3_prepare_v2(db, "SELECT asm_mesh_type_value_id FROM asm_data_set_assembly_meshes "
                               "WHERE asm_id = ? LIMIT 1", -1, &s, nullptr) != SQLITE_OK) return 0;
    sqlite3_bind_int64(s, 1, asm_id);
    if (sqlite3_step(s) == SQLITE_ROW) t = sqlite3_column_int(s, 0);
    sqlite3_finalize(s);
    return t;
}

bool HaveMeshTable(sqlite3* db) {
    sqlite3_stmt* s = nullptr;
    bool ok = false;
    if (sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM asm_data_set_assembly_meshes", -1, &s, nullptr) == SQLITE_OK) {
        ok = sqlite3_step(s) == SQLITE_ROW && sqlite3_column_int(s, 0) > 0;
        sqlite3_finalize(s);
    }
    return ok;
}

}  // namespace

void SanitizeCreateLooks(uint32_t& head_asm_id, uint32_t& hair_asm_id) {
    constexpr uint32_t kDefaultHead = 1605, kDefaultHair = 1974, kBald = 403;
    constexpr int kHeadType = 595, kHairBuilderType = 596, kHairPawnType = 850;
    sqlite3* db = Database::GetConnection();
    const uint32_t head0 = head_asm_id, hair0 = hair_asm_id;
    if (hair_asm_id == 0 || hair_asm_id == kBald) hair_asm_id = kDefaultHair;
    if (db && HaveMeshTable(db)) {
        const int ht = MeshType(db, hair_asm_id);
        if (ht != kHairBuilderType && ht != kHairPawnType) hair_asm_id = kDefaultHair;
        if (MeshType(db, head_asm_id) != kHeadType) head_asm_id = kDefaultHead;
    }
    if (head0 != head_asm_id || hair0 != hair_asm_id)
        Logger::Log("tcp", "[EconParity] create looks fixed: head %u->%u hair %u->%u\n",
                    head0, head_asm_id, hair0, hair_asm_id);
}

namespace {

constexpr int kEpic = 1162, kRare = 1163, kUncommon = 1164, kCommon = 1165;
int Rank(int q) { return q == kEpic ? 3 : q == kRare ? 2 : q == kUncommon ? 1 : 0; }

// quality -> egid of one mod blueprint
std::map<int, int> BlueprintTiers(sqlite3* db, int blueprint_id) {
    std::map<int, int> out;
    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(db,
            "SELECT bim.quality_value_id, MIN(me.effect_group_id) FROM asm_data_set_blueprint_item_mods bim "
            "JOIN asm_data_set_blueprint_mod_effect_groups me ON me.blueprint_mod_id = bim.blueprint_mod_id "
            "WHERE bim.blueprint_id = ? GROUP BY bim.quality_value_id", -1, &s, nullptr) != SQLITE_OK) return out;
    sqlite3_bind_int(s, 1, blueprint_id);
    while (sqlite3_step(s) == SQLITE_ROW) out[sqlite3_column_int(s, 0)] = sqlite3_column_int(s, 1);
    sqlite3_finalize(s);
    return out;
}

std::string TiersToText(const std::map<int, int>& t) {
    std::string out;
    for (const auto& [q, e] : t) out += (out.empty() ? "" : ",") + std::to_string(q) + ":" + std::to_string(e);
    return out;
}

std::map<int, int> TextToTiers(const std::string& txt) {
    std::map<int, int> out;
    std::stringstream ss(txt);
    std::string part;
    while (std::getline(ss, part, ',')) {
        const auto c = part.find(':');
        if (c == std::string::npos) continue;
        const int q = std::atoi(part.substr(0, c).c_str()), e = std::atoi(part.substr(c + 1).c_str());
        if (Rank(q) > 0 && e > 0) out[q] = e;
    }
    return out;
}

// Unstamped mod rows (made before the mod tiers were stamped, market-bot stock): the item's richest blueprint.
std::map<int, int> FallbackTiers(sqlite3* db, int mod_item_id) {
    std::map<int, int> best;
    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(db, "SELECT blueprint_id FROM asm_data_set_blueprints WHERE created_item_id = ?",
                           -1, &s, nullptr) != SQLITE_OK) return best;
    sqlite3_bind_int(s, 1, mod_item_id);
    std::vector<int> bps;
    while (sqlite3_step(s) == SQLITE_ROW) bps.push_back(sqlite3_column_int(s, 0));
    sqlite3_finalize(s);
    for (int bp : bps) { auto t = BlueprintTiers(db, bp); if (t.size() > best.size()) best = t; }
    return best;
}

struct Row { bool found = false; int item_id = 0, device_id = 0, item_type = 0, quality = 0;
             int64_t character_id = 0; std::string mods, slots; };

Row LoadRow(sqlite3* db, int64_t user_id, int64_t id) {
    Row r;
    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(db,
            "SELECT p.item_id, p.device_id, COALESCE(i.item_type_value_id, 0), "
            "  CASE WHEN p.quality IN (1162,1163,1164,1165) THEN p.quality ELSE COALESCE(i.quality_value_id, 0) END, "
            "  p.character_id, p.mod_effect_group_ids, p.allowed_slots "
            "FROM ga_players_inventory p LEFT JOIN asm_data_set_items i ON i.item_id = p.item_id "
            "WHERE p.id = ? AND p.user_id = ?", -1, &s, nullptr) != SQLITE_OK) return r;
    sqlite3_bind_int64(s, 1, id);
    sqlite3_bind_int64(s, 2, user_id);
    if (sqlite3_step(s) == SQLITE_ROW) {
        r.found = true;
        r.item_id = sqlite3_column_int(s, 0); r.device_id = sqlite3_column_int(s, 1);
        r.item_type = sqlite3_column_int(s, 2); r.quality = sqlite3_column_int(s, 3);
        r.character_id = sqlite3_column_int64(s, 4);
        const unsigned char* m = sqlite3_column_text(s, 5); r.mods = m ? (const char*)m : "";
        const unsigned char* a = sqlite3_column_text(s, 6); r.slots = a ? (const char*)a : "";
    }
    sqlite3_finalize(s);
    return r;
}

bool IsModEgid(sqlite3* db, int egid) {
    sqlite3_stmt* s = nullptr;
    bool yes = false;
    if (sqlite3_prepare_v2(db,
            "SELECT 1 FROM asm_data_set_blueprint_mod_effect_groups me "
            "JOIN asm_data_set_blueprint_item_mods bim ON bim.blueprint_mod_id = me.blueprint_mod_id "
            "JOIN asm_data_set_blueprints b ON b.blueprint_id = bim.blueprint_id "
            "JOIN asm_data_set_items c ON c.item_id = b.created_item_id AND c.item_type_value_id IN (1434, 1441) "
            "WHERE me.effect_group_id = ? LIMIT 1", -1, &s, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(s, 1, egid);
    yes = sqlite3_step(s) == SQLITE_ROW;
    sqlite3_finalize(s);
    return yes;
}

}  // namespace

static void EnsureSchema(sqlite3* db) {
    static bool done = false;
    if (done || !db) return;
    sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS ga_crafted_mod_tiers ("
                     " inventory_id INTEGER PRIMARY KEY, blueprint_id INTEGER NOT NULL, tiers TEXT NOT NULL,"
                     " created_at INTEGER NOT NULL DEFAULT (strftime('%s','now')))", nullptr, nullptr, nullptr);
    done = true;
}

void InitSchema() {
    sqlite3* db = Database::GetConnection();
    if (!db) return;
    EnsureSchema(db);
    char* err = nullptr;
    if (sqlite3_exec(db,
            "CREATE TRIGGER IF NOT EXISTS trg_lv50_xp_credits AFTER UPDATE OF xp ON ga_characters "
            "WHEN NEW.xp > (SELECT MAX(xp_value) FROM asm_data_set_xp_levels) BEGIN "
            " UPDATE ga_characters SET credits = COALESCE(credits,0) + CAST((NEW.xp - MAX(COALESCE(OLD.xp,0), "
            "   (SELECT MAX(xp_value) FROM asm_data_set_xp_levels))) * COALESCE((SELECT CAST(value AS REAL) "
            "   FROM cs_settings WHERE key = 'lv50_xp_credit_pct'), 33) / 100 AS INTEGER), "
            "  xp = (SELECT MAX(xp_value) FROM asm_data_set_xp_levels) WHERE id = NEW.id; END",
            nullptr, nullptr, &err) != SQLITE_OK) {
        Logger::Log("db", "[Econ] lv50 xp trigger failed: %s\n", err ? err : "?");
        sqlite3_free(err);
    }
}

void StampCraftedMod(int blueprint_id, const std::vector<int64_t>& inventory_ids) {
    sqlite3* db = Database::GetConnection();
    if (!db || blueprint_id <= 0 || inventory_ids.empty()) return;
    const auto tiers = BlueprintTiers(db, blueprint_id);
    if (tiers.empty()) return;  // not a mod blueprint
    EnsureSchema(db);
    const std::string txt = TiersToText(tiers);
    for (int64_t id : inventory_ids) {
        sqlite3_stmt* s = nullptr;
        if (sqlite3_prepare_v2(db,
                "INSERT OR REPLACE INTO ga_crafted_mod_tiers (inventory_id, blueprint_id, tiers) "
                "SELECT ?2, ?3, ?1 FROM ga_players_inventory WHERE id = ?2 AND item_id IN "
                "(SELECT item_id FROM asm_data_set_items WHERE item_type_value_id IN (1434, 1441))",
                -1, &s, nullptr) != SQLITE_OK) continue;
        sqlite3_bind_text(s, 1, txt.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(s, 2, id);
        sqlite3_bind_int(s, 3, blueprint_id);
        sqlite3_step(s);
        sqlite3_finalize(s);
    }
    Logger::Log("loot", "[Mod] crafted mod rows=%zu blueprint=%d tiers=%s\n", inventory_ids.size(), blueprint_id, txt.c_str());
}

static CombineResult CombineImpl(int64_t user_id, int64_t character_id, int64_t target_row, int64_t mod_row) {
    CombineResult r;
    r.target_row = target_row; r.mod_row = mod_row;
    sqlite3* db = Database::GetConnection();
    if (!db || user_id <= 0) { r.reason = "no account"; return r; }
    if (target_row <= 0 || mod_row <= 0 || target_row == mod_row) { r.reason = "bad rows"; return r; }
    const Row t = LoadRow(db, user_id, target_row), m = LoadRow(db, user_id, mod_row);
    auto mine = [&](const Row& x) { return x.found && (x.character_id == 0 || character_id == 0 || x.character_id == character_id); };
    if (!mine(t) || !mine(m)) { r.reason = "row not owned"; return r; }
    if (m.item_type != 1434 && m.item_type != 1441) { r.reason = "not a mod (type " + std::to_string(m.item_type) + ")"; return r; }
    const bool armor = t.item_type == 1106;
    const bool weapon = t.device_id > 0 && !armor && t.item_type != 1412 && t.item_type != 1441 && t.item_type != 1434
                        && (t.item_type == 217 || t.item_id == 0) && t.slots != "501";
    if (m.item_type == 1441 && !armor) { r.reason = "armor mod needs armor"; return r; }
    if (m.item_type == 1434 && !weapon) { r.reason = "weapon mod needs a weapon"; return r; }
    const int rank = Rank(t.quality);
    if (rank == 0) { r.reason = "gear must be Uncommon or better (quality " + std::to_string(t.quality) + ")"; return r; }
    EnsureSchema(db);
    std::map<int, int> tiers;
    {
        sqlite3_stmt* q = nullptr;
        if (sqlite3_prepare_v2(db, "SELECT tiers FROM ga_crafted_mod_tiers WHERE inventory_id = ?", -1, &q, nullptr) == SQLITE_OK) {
            sqlite3_bind_int64(q, 1, mod_row);
            if (sqlite3_step(q) == SQLITE_ROW && sqlite3_column_text(q, 0))
                tiers = TextToTiers((const char*)sqlite3_column_text(q, 0));
            sqlite3_finalize(q);
        }
    }
    if (tiers.empty()) tiers = FallbackTiers(db, m.item_id);
    int best_rank = 0;
    for (const auto& [q, e] : tiers)
        if (Rank(q) <= rank && Rank(q) > best_rank) { best_rank = Rank(q); r.egid = e; }
    if (r.egid <= 0) { r.reason = "mod has no tier for this quality"; return r; }

    // Keep the gear's own letters, drop an earlier mod (one mod per item).
    std::string kept;
    {
        std::stringstream ss(t.mods);
        std::string part;
        while (std::getline(ss, part, ',')) {
            const int e = std::atoi(part.c_str());
            if (e <= 0 || IsModEgid(db, e)) continue;
            kept += (kept.empty() ? "" : ",") + std::to_string(e);
        }
    }
    kept += (kept.empty() ? "" : ",") + std::to_string(r.egid);

    sqlite3_exec(db, "BEGIN IMMEDIATE;", nullptr, nullptr, nullptr);
    bool ok = false;
    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(db, "DELETE FROM ga_players_inventory WHERE id = ? AND user_id = ?", -1, &s, nullptr) == SQLITE_OK) {
        sqlite3_bind_int64(s, 1, mod_row); sqlite3_bind_int64(s, 2, user_id);
        ok = sqlite3_step(s) == SQLITE_DONE && sqlite3_changes(db) == 1;
        sqlite3_finalize(s);
    }
    if (ok && sqlite3_prepare_v2(db, "DELETE FROM ga_crafted_mod_tiers WHERE inventory_id = ?", -1, &s, nullptr) == SQLITE_OK) {
        sqlite3_bind_int64(s, 1, mod_row); sqlite3_step(s); sqlite3_finalize(s);
    }
    if (ok && sqlite3_prepare_v2(db, "UPDATE ga_players_inventory SET mod_effect_group_ids = ? WHERE id = ? AND user_id = ?",
                                 -1, &s, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(s, 1, kept.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(s, 2, target_row); sqlite3_bind_int64(s, 3, user_id);
        ok = sqlite3_step(s) == SQLITE_DONE && sqlite3_changes(db) == 1;
        sqlite3_finalize(s);
    } else ok = false;
    sqlite3_exec(db, ok ? "COMMIT;" : "ROLLBACK;", nullptr, nullptr, nullptr);
    r.ok = ok;
    if (!ok) r.reason = "db write failed";
    Logger::Log("loot", "[Mod] combine user=%lld char=%lld target=%lld (type %d q %d) mod=%lld (item %d) -> %s egid=%d mods=%s (%s)\n",
        (long long)user_id, (long long)character_id, (long long)target_row, t.item_type, t.quality,
        (long long)mod_row, m.item_id, ok ? "OK" : "REFUSED", r.egid, ok ? kept.c_str() : t.mods.c_str(), r.reason.c_str());
    return r;
}

CombineResult CombineItems(int64_t user_id, int64_t character_id, int64_t target_row, int64_t mod_row) {
    CombineResult r = CombineImpl(user_id, character_id, target_row, mod_row);
    if (!r.ok)
        Logger::Log("loot", "[Mod] combine user=%lld char=%lld target=%lld mod=%lld -> REFUSED (%s)\n",
            (long long)user_id, (long long)character_id, (long long)target_row, (long long)mod_row, r.reason.c_str());
    return r;
}

namespace {
constexpr int64_t kBoosterNightlyTokens = 2000;  // wiki Booster / Steam 10-01-2010: 2,000 tokens at midnight GMT
int64_t UserOf(sqlite3* db, int64_t cid) {
    sqlite3_stmt* s = nullptr; int64_t u = 0;
    if (sqlite3_prepare_v2(db, "SELECT user_id FROM ga_characters WHERE id = ? AND deleted_at = 0", -1, &s, nullptr) == SQLITE_OK) {
        sqlite3_bind_int64(s, 1, cid);
        if (sqlite3_step(s) == SQLITE_ROW) u = sqlite3_column_int64(s, 0);
        sqlite3_finalize(s);
    }
    return u;
}
int64_t BoosterUntil(sqlite3* db, int64_t uid) {
    sqlite3_stmt* s = nullptr; int64_t t = 0;
    if (sqlite3_prepare_v2(db, "SELECT booster_until FROM ga_users WHERE id = ?", -1, &s, nullptr) == SQLITE_OK) {
        sqlite3_bind_int64(s, 1, uid);
        if (sqlite3_step(s) == SQLITE_ROW) t = sqlite3_column_int64(s, 0);
        sqlite3_finalize(s);
    }
    return t;
}
}  // namespace

bool BoosterActive(int64_t character_id) {
    sqlite3* db = Database::GetConnection();
    if (!db || character_id <= 0) return false;
    const int64_t uid = UserOf(db, character_id);
    return uid > 0 && BoosterUntil(db, uid) > (int64_t)time(nullptr);
}

int64_t BoosterNightlyTokens(int64_t character_id) {
    static std::mutex mu;
    static std::map<int64_t, int64_t> checked;  // character -> UTC day already handled
    const int64_t now = (int64_t)time(nullptr), today = now / 86400;
    {
        std::lock_guard<std::mutex> l(mu);
        auto it = checked.find(character_id);
        if (it != checked.end() && it->second == today) return 0;
        checked[character_id] = today;
    }
    sqlite3* db = Database::GetConnection();
    if (!db || character_id <= 0) return 0;
    const int64_t uid = UserOf(db, character_id);
    if (uid <= 0) return 0;
    sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS ga_booster_token_days (user_id INTEGER PRIMARY KEY, last_day INTEGER NOT NULL)",
                 nullptr, nullptr, nullptr);
    int64_t last = -1;
    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(db, "SELECT last_day FROM ga_booster_token_days WHERE user_id = ?", -1, &s, nullptr) == SQLITE_OK) {
        sqlite3_bind_int64(s, 1, uid);
        if (sqlite3_step(s) == SQLITE_ROW) last = sqlite3_column_int64(s, 0);
        sqlite3_finalize(s);
    }
    // Midnights d*86400 in (last, today] that fell inside the booster (d*86400 < booster_until).
    // First sight of an account: start counting from today (nothing owed yet).
    int64_t nights = 0;
    if (last >= 0 && last < today) {
        const int64_t until_day = (BoosterUntil(db, uid) - 1) / 86400;  // last midnight before it ends
        const int64_t hi = std::min(today, until_day);
        nights = std::clamp<int64_t>(hi - last, 0, 90);
    }
    if (sqlite3_prepare_v2(db, "INSERT INTO ga_booster_token_days (user_id, last_day) VALUES (?1, ?2) "
                               "ON CONFLICT(user_id) DO UPDATE SET last_day = MAX(last_day, ?2)", -1, &s, nullptr) == SQLITE_OK) {
        sqlite3_bind_int64(s, 1, uid); sqlite3_bind_int64(s, 2, today);
        sqlite3_step(s); sqlite3_finalize(s);
        if (sqlite3_changes(db) != 1) nights = 0;
    } else nights = 0;
    if (nights <= 0) return 0;
    const int64_t paid = PlayerSessionStore::LimitTokenGain(character_id, nights * kBoosterNightlyTokens);
    if (paid > 0 && sqlite3_prepare_v2(db, "UPDATE ga_characters SET tokens = tokens + ? WHERE id = ? AND deleted_at = 0",
                                       -1, &s, nullptr) == SQLITE_OK) {
        sqlite3_bind_int64(s, 1, paid); sqlite3_bind_int64(s, 2, character_id);
        sqlite3_step(s); sqlite3_finalize(s);
    }
    Logger::Log("rewards", "[Booster] nightly tokens user=%lld char=%lld nights=%lld -> +%lld tokens (caps applied)\n",
                (long long)uid, (long long)character_id, (long long)nights, (long long)paid);
    return paid;
}

int QueueBoosterLoot(int64_t instance_id, int players_tf, int loot_table_id, const std::vector<int64_t>& exclude) {
    sqlite3* db = Database::GetConnection();
    if (!db || instance_id == 0 || loot_table_id <= 0) return 0;
    std::vector<int64_t> boosted;
    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(db,
            "SELECT DISTINCT p.character_id FROM ga_instance_players p JOIN ga_characters c ON c.id = p.character_id "
            "JOIN ga_users u ON u.id = c.user_id WHERE p.instance_id = ? AND p.task_force_number = ? "
            "AND p.left_at IS NULL AND p.character_id > 0 AND u.booster_until > strftime('%s','now')",
            -1, &s, nullptr) != SQLITE_OK) return 0;
    sqlite3_bind_int64(s, 1, instance_id); sqlite3_bind_int(s, 2, players_tf);
    while (sqlite3_step(s) == SQLITE_ROW) boosted.push_back(sqlite3_column_int64(s, 0));
    sqlite3_finalize(s);
    int n = 0;
    for (int64_t cid : boosted)
        if (std::find(exclude.begin(), exclude.end(), cid) == exclude.end())
            n += MissionLoot::QueueForCharacter(cid, -instance_id, loot_table_id, 0);  // ref -inst = the 2nd roll
    if (n) Logger::Log("rewards", "[Booster] instance=%lld table=%d -> %d extra roll(s)\n", (long long)instance_id, loot_table_id, n);
    return n;
}

void BoosterQuestBonus(int64_t character_id, int64_t& xp, int64_t& credits) {
    if ((xp <= 0 && credits <= 0) || !BoosterActive(character_id)) return;
    xp += xp / 2; credits += credits / 2;
}

int RollGearStats(int blueprint_id, int quality) {
    sqlite3* db = Database::GetConnection();
    if (!db || blueprint_id <= 0 || (Rank(quality) == 0 && quality != kCommon)) return 0;
    std::vector<std::pair<int, double>> pool;
    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(db,
            "SELECT me.effect_group_id, me.make_chance FROM asm_data_set_blueprint_item_mods bim "
            "JOIN asm_data_set_blueprint_mod_effect_groups me ON me.blueprint_mod_id = bim.blueprint_mod_id "
            "WHERE bim.blueprint_id = ? AND bim.quality_value_id = ? ORDER BY me.id", -1, &s, nullptr) != SQLITE_OK) return 0;
    sqlite3_bind_int(s, 1, blueprint_id); sqlite3_bind_int(s, 2, quality);
    double total = 0;
    while (sqlite3_step(s) == SQLITE_ROW) {
        const double w = std::max(0.0, sqlite3_column_double(s, 1));
        pool.emplace_back(sqlite3_column_int(s, 0), w); total += w;
    }
    sqlite3_finalize(s);
    if (pool.empty()) return 0;
    static thread_local std::mt19937 rng{std::random_device{}()};
    if (total <= 0) return pool[std::uniform_int_distribution<size_t>(0, pool.size() - 1)(rng)].first;
    double x = std::uniform_real_distribution<double>(0.0, total)(rng);
    for (const auto& [e, w] : pool) { if (x < w) return e; x -= w; }
    return pool.back().first;
}

std::string RollGearMods(int blueprint_id, int quality) {
    if (blueprint_id <= 0 || (Rank(quality) == 0 && quality != kCommon)) return "";
    std::string out;
    for (int tier : {kCommon, kUncommon, kRare, kEpic}) {
        if (tier != kCommon && Rank(tier) > Rank(quality)) break;
        const int e = RollGearStats(blueprint_id, tier);
        if (e > 0) out += (out.empty() ? "" : ",") + std::to_string(e);
    }
    return out;
}

void ApplyAutoGear(const std::vector<int64_t>& new_rows, const std::vector<Database::AutoGear>& gear) {
    std::vector<GearDrop> drops;
    for (const auto& g : gear) drops.push_back({g.item_id, g.blueprint_id, g.quality});
    ApplyGearDrops(new_rows, drops);
}

void ApplyGearDrops(const std::vector<int64_t>& new_rows, const std::vector<GearDrop>& drops) {
    sqlite3* db = Database::GetConnection();
    if (!db || new_rows.empty() || drops.empty()) return;
    std::vector<bool> used(drops.size(), false);
    for (int64_t id : new_rows) {
        sqlite3_stmt* s = nullptr; int item = 0;
        if (sqlite3_prepare_v2(db, "SELECT item_id FROM ga_players_inventory WHERE id = ?", -1, &s, nullptr) != SQLITE_OK) continue;
        sqlite3_bind_int64(s, 1, id);
        if (sqlite3_step(s) == SQLITE_ROW) item = sqlite3_column_int(s, 0);
        sqlite3_finalize(s);
        for (size_t k = 0; k < drops.size(); ++k) {
            if (used[k] || drops[k].item_id != item) continue;
            used[k] = true;
            const std::string mods = RollGearMods(drops[k].blueprint_id, drops[k].quality);  // 0957b: every tier
            if (sqlite3_prepare_v2(db, "UPDATE ga_players_inventory SET quality = ?1, mod_effect_group_ids = ?2 "
                                       "WHERE id = ?3 AND mod_effect_group_ids = ''", -1, &s, nullptr) == SQLITE_OK) {
                sqlite3_bind_int(s, 1, drops[k].quality);
                sqlite3_bind_text(s, 2, mods.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_int64(s, 3, id);
                sqlite3_step(s); sqlite3_finalize(s);
            }
            Logger::Log("loot", "[Gear] row=%lld item=%d bp=%d quality=%d -> stats egids %s\n",
                        (long long)id, item, drops[k].blueprint_id, drops[k].quality, mods.c_str());
            break;
        }
    }
}

}  // namespace EconParity
