// Agency bank / tax / bank log + agency chat lines.
// Request + receiver layouts decoded from GlobalAgenda.exe:
//   0xC9 AGENCY_UPDATE_TAX      TAX_PERCENT(float) + ROSTER_FLAGS
//   0xCC AGENCY_ADD_INVENTORY   INVENTORY_ID + INSTANCE_COUNT
//   0xCD AGENCY_MAIL_INVENTORY  INVENTORY_ID + NAME_MSG_ID + INSTANCE_COUNT + PLAYER_NAME
//   0xCE AGENCY_REMOVE_INVENTORY INVENTORY_ID + ITEM_ID + INSTANCE_COUNT
//   0xCF AGENCY_UPDATE_CURRENCY CURRENCY_VALUE (signed)
//   0x1BE GET_AGENCY_INV_LOG    AGENCY_ID + DATE_START + DATE_END + CHANGE_TYPE_VALUE_ID + PLAYER_NAME
//   roster 0xC2: ROSTER_FLAGS bit 0x10 gates CURRENCY_VALUE + DATA_SET_AGENCY_INVENTORY(+INVENTORY_STATE);
//   TAX_PERCENT is read in the bit0 block.
//   log reply: DATA_SET rows {CHANGE_DATETIME (days since 1900), CHANGE_TYPE_VALUE_ID, PLAYER_NAME,
//   ITEM_ID, CURRENCY_VALUE, INSTANCE_COUNT}.
#include "src/ControlServer/Social/AgencyBank.hpp"
#include "src/ControlServer/Database/Database.hpp"
#include "src/ControlServer/PlayerSessionStore/PlayerSessionStore.hpp"
#include "src/ControlServer/ChatSession/ChatSession.hpp"
#include "src/ControlServer/MailService/MailService.hpp"
#include "src/ControlServer/TcpSession/TcpSession.hpp"
#include "src/ControlServer/Constants/TcpFunctions.h"
#include "src/ControlServer/Constants/TcpTypes.h"
#include "src/ControlServer/Logger.hpp"
#include "sqlite3.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <ctime>
#include <map>
#include <mutex>
#include <set>

namespace {

struct Stmt {
    sqlite3_stmt* s = nullptr;
    Stmt(sqlite3* db, const char* sql) {
        if (sqlite3_prepare_v2(db, sql, -1, &s, nullptr) != SQLITE_OK) {
            Logger::Log("agency", "[Bank] prepare failed: %s (%s)\n", sqlite3_errmsg(db), sql);
            s = nullptr;
        }
    }
    ~Stmt() { if (s) sqlite3_finalize(s); }
    explicit operator bool() const { return s != nullptr; }
};

bool Exec(sqlite3* db, const char* sql) {
    char* err = nullptr;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &err) != SQLITE_OK) {
        Logger::Log("agency", "[Bank] exec failed: %s (%s)\n", err ? err : "?", sql);
        if (err) sqlite3_free(err);
        return false;
    }
    return true;
}

bool Step(sqlite3* db, Stmt& st, bool need_change = true) {
    return st && sqlite3_step(st.s) == SQLITE_DONE && (!need_change || sqlite3_changes(db) > 0);
}

int64_t Scalar(sqlite3* db, const char* sql, int64_t a, int64_t b = 0) {
    Stmt st(db, sql);
    if (!st) return 0;
    sqlite3_bind_int64(st.s, 1, a);
    if (sqlite3_bind_parameter_count(st.s) > 1) sqlite3_bind_int64(st.s, 2, b);
    return sqlite3_step(st.s) == SQLITE_ROW ? sqlite3_column_int64(st.s, 0) : 0;
}

std::string ColText(sqlite3_stmt* s, int i) {
    const unsigned char* t = sqlite3_column_text(s, i);
    return t ? reinterpret_cast<const char*>(t) : "";
}

std::once_flag g_schema_once;
std::mutex g_ver_mutex;
std::map<int64_t, uint64_t> g_versions;   // agency -> bank version
std::atomic<uint64_t> g_ver_seq{1};

void Bump(int64_t agency_id) {
    std::lock_guard<std::mutex> l(g_ver_mutex);
    g_versions[agency_id] = ++g_ver_seq;
}

std::string NameOf(sqlite3* db, int64_t character_id) {
    Stmt st(db, "SELECT u.username FROM ga_characters c JOIN ga_users u ON u.id = c.user_id WHERE c.id = ?");
    if (!st) return "";
    sqlite3_bind_int64(st.s, 1, character_id);
    return sqlite3_step(st.s) == SQLITE_ROW ? ColText(st.s, 0) : "";
}

bool AddLog(sqlite3* db, int64_t agency_id, uint32_t type, int64_t character_id,
            int item_id, int64_t currency, int64_t count) {
    Stmt st(db,
        "INSERT INTO ga_agency_bank_log (agency_id, at, change_type, character_id, player_name, item_id, currency, count) "
        "VALUES (?, strftime('%s','now'), ?, ?, ?, ?, ?, ?)");
    if (!st) return false;
    const std::string name = NameOf(db, character_id);
    sqlite3_bind_int64(st.s, 1, agency_id);
    sqlite3_bind_int(st.s, 2, (int)type);
    sqlite3_bind_int64(st.s, 3, character_id);
    sqlite3_bind_text(st.s, 4, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(st.s, 5, item_id);
    sqlite3_bind_int64(st.s, 6, currency);
    sqlite3_bind_int64(st.s, 7, count);
    return Step(db, st);
}

// gear rows: the asm item id the client names (device rows carry item_id 0)
int ItemIdOfRow(sqlite3* db, int64_t inventory_id) {
    return (int)Scalar(db,
        "SELECT COALESCE(NULLIF(p.item_id,0), (SELECT MIN(i.item_id) FROM asm_data_set_items i "
        " WHERE p.device_id > 0 AND i.ref_device_id = p.device_id), 0) FROM ga_players_inventory p WHERE p.id = ?",
        inventory_id);
}

int64_t UserOf(sqlite3* db, int64_t character_id) {
    return Scalar(db, "SELECT user_id FROM ga_characters WHERE id = ? AND deleted_at = 0", character_id);
}

bool HasColumn(sqlite3* db, const char* table, const char* col) {
    std::string sql = std::string("PRAGMA table_info(") + table + ")";
    Stmt st(db, sql.c_str());
    if (!st) return false;
    while (sqlite3_step(st.s) == SQLITE_ROW)
        if (ColText(st.s, 1) == col) return true;
    return false;
}

} // namespace

namespace AgencyBank {

void EnsureSchema() {
    std::call_once(g_schema_once, [] {
        sqlite3* db = Database::GetConnection();
        Exec(db,
            "CREATE TABLE IF NOT EXISTS ga_agency_bank ("
            "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "  agency_id INTEGER NOT NULL,"
            "  inventory_id INTEGER NOT NULL DEFAULT 0,"   // gear row parked at user_id 0, 0 = component stack
            "  item_id INTEGER NOT NULL DEFAULT 0,"
            "  quantity INTEGER NOT NULL DEFAULT 1,"
            "  deposited_by INTEGER NOT NULL DEFAULT 0,"
            "  deposited_at INTEGER NOT NULL DEFAULT 0);"
            "CREATE INDEX IF NOT EXISTS idx_ga_agency_bank_agency ON ga_agency_bank(agency_id);"
            "CREATE TABLE IF NOT EXISTS ga_agency_bank_log ("
            "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "  agency_id INTEGER NOT NULL,"
            "  at INTEGER NOT NULL,"
            "  change_type INTEGER NOT NULL,"
            "  character_id INTEGER NOT NULL DEFAULT 0,"
            "  player_name TEXT NOT NULL DEFAULT '',"
            "  item_id INTEGER NOT NULL DEFAULT 0,"
            "  currency INTEGER NOT NULL DEFAULT 0,"
            "  count INTEGER NOT NULL DEFAULT 0);"
            "CREATE INDEX IF NOT EXISTS idx_ga_agency_bank_log_agency ON ga_agency_bank_log(agency_id, at);");
        if (!HasColumn(db, "ga_agencies", "bank_credits"))
            Exec(db, "ALTER TABLE ga_agencies ADD COLUMN bank_credits INTEGER NOT NULL DEFAULT 0");
        if (!HasColumn(db, "ga_agencies", "tax_percent"))
            Exec(db, "ALTER TABLE ga_agencies ADD COLUMN tax_percent REAL NOT NULL DEFAULT 0");
        Logger::Log("agency", "[Bank] schema ready\n");
    });
}

uint64_t Version(int64_t agency_id) {
    std::lock_guard<std::mutex> l(g_ver_mutex);
    auto it = g_versions.find(agency_id);
    if (it == g_versions.end()) it = g_versions.emplace(agency_id, ++g_ver_seq).first;
    return it->second;
}

Bank Load(int64_t agency_id) {
    EnsureSchema();
    Bank b;
    sqlite3* db = Database::GetConnection();
    {
        Stmt st(db, "SELECT bank_credits, tax_percent FROM ga_agencies WHERE id = ?");
        if (st) {
            sqlite3_bind_int64(st.s, 1, agency_id);
            if (sqlite3_step(st.s) == SQLITE_ROW) {
                b.credits = sqlite3_column_int64(st.s, 0);
                b.tax_percent = (float)sqlite3_column_double(st.s, 1);
            }
        }
    }
    Stmt st(db,
        "SELECT b.id, b.inventory_id, b.item_id, b.quantity, COALESCE(p.device_id,0), "
        "  CASE WHEN b.inventory_id > 0 THEN COALESCE(p.quality,0) "
        "       ELSE COALESCE((SELECT quality_value_id FROM asm_data_set_items WHERE item_id = b.item_id),0) END, "
        "  COALESCE(p.mod_effect_group_ids,''), COALESCE(p.oc,0), COALESCE(p.profile_id,0), COALESCE(p.item_id,0) "
        "FROM ga_agency_bank b LEFT JOIN ga_players_inventory p ON p.id = b.inventory_id AND p.user_id = 0 "
        "WHERE b.agency_id = ? ORDER BY b.id");
    if (!st) return b;
    sqlite3_bind_int64(st.s, 1, agency_id);
    while (sqlite3_step(st.s) == SQLITE_ROW) {
        Item it;
        it.bank_id      = sqlite3_column_int64(st.s, 0);
        it.inventory_id = sqlite3_column_int64(st.s, 1);
        it.item_id      = it.inventory_id > 0 ? sqlite3_column_int(st.s, 9) : sqlite3_column_int(st.s, 2);
        it.quantity     = sqlite3_column_int64(st.s, 3);
        it.device_id    = sqlite3_column_int(st.s, 4);
        it.quality      = sqlite3_column_int(st.s, 5);
        it.mods         = ColText(st.s, 6);
        it.oc           = sqlite3_column_int(st.s, 7);
        it.profile_id   = sqlite3_column_int(st.s, 8);
        b.items.push_back(it);
    }
    return b;
}

bool IsEmpty(int64_t agency_id) {
    EnsureSchema();
    sqlite3* db = Database::GetConnection();
    return Scalar(db, "SELECT COUNT(*) FROM ga_agency_bank WHERE agency_id = ?", agency_id) == 0 &&
           Scalar(db, "SELECT COALESCE(bank_credits,0) FROM ga_agencies WHERE id = ?", agency_id) <= 0;
}

Result DepositItem(int64_t agency_id, int64_t character_id, int64_t inventory_id, int64_t count) {
    EnsureSchema();
    Result r;
    sqlite3* db = Database::GetConnection();
    const int64_t user_id = UserOf(db, character_id);
    if (user_id <= 0 || agency_id <= 0 || inventory_id <= 0) return r;
    if (!Exec(db, "BEGIN IMMEDIATE")) return r;
    auto fail = [&]() { Exec(db, "ROLLBACK"); r = Result{}; return r; };

    if (inventory_id >= (int64_t)MailService::COMPONENT_INV_BASE) {
        const int item_id = (int)(inventory_id - MailService::COMPONENT_INV_BASE);
        if (item_id <= 0 || count <= 0) return fail();
        if (Scalar(db, "SELECT COALESCE(item_bind_type_value_id,0) FROM asm_data_set_items WHERE item_id = ?", item_id)
                == MailService::BIND_ON_ACQUIRE) return fail();
        {
            Stmt st(db, "UPDATE ga_user_components SET quantity = quantity - ?1, updated_at = strftime('%s','now') "
                        "WHERE user_id = ?2 AND item_id = ?3 AND quantity >= ?1");
            if (!st) return fail();
            sqlite3_bind_int64(st.s, 1, count); sqlite3_bind_int64(st.s, 2, user_id); sqlite3_bind_int(st.s, 3, item_id);
            if (!Step(db, st)) return fail();
        }
        {
            Stmt st(db, "DELETE FROM ga_user_components WHERE user_id = ? AND item_id = ? AND quantity <= 0");
            if (st) {
                sqlite3_bind_int64(st.s, 1, user_id); sqlite3_bind_int(st.s, 2, item_id);
                if (Step(db, st)) r.depleted_components.push_back(item_id);
            }
        }
        // one bank stack per component item
        const int64_t have = Scalar(db, "SELECT id FROM ga_agency_bank WHERE agency_id = ? AND inventory_id = 0 AND item_id = ?",
                                    agency_id, item_id);
        if (have > 0) {
            Stmt st(db, "UPDATE ga_agency_bank SET quantity = quantity + ? WHERE id = ?");
            if (!st) return fail();
            sqlite3_bind_int64(st.s, 1, count); sqlite3_bind_int64(st.s, 2, have);
            if (!Step(db, st)) return fail();
        } else {
            Stmt st(db, "INSERT INTO ga_agency_bank (agency_id, inventory_id, item_id, quantity, deposited_by, deposited_at) "
                        "VALUES (?, 0, ?, ?, ?, strftime('%s','now'))");
            if (!st) return fail();
            sqlite3_bind_int64(st.s, 1, agency_id); sqlite3_bind_int(st.s, 2, item_id);
            sqlite3_bind_int64(st.s, 3, count); sqlite3_bind_int64(st.s, 4, character_id);
            if (!Step(db, st)) return fail();
        }
        if (!AddLog(db, agency_id, EV_ITEM, character_id, item_id, 0, count)) return fail();
        r.components_changed = true;
    } else {
        int profile = 0, equipped = 0, item_id = 0;
        int64_t sell_value = 0, sell_cur = 0, bind = 0;
        {
            Stmt st(db,
                "SELECT p.item_id, p.profile_id, COALESCE(p.sell_value,0), COALESCE(p.sell_currency,0), "
                "  (SELECT COUNT(*) FROM ga_character_devices d JOIN ga_characters c ON c.id = d.character_id "
                "   WHERE d.inventory_id = p.id AND c.user_id = p.user_id), "
                "  COALESCE((SELECT MAX(item_bind_type_value_id) FROM asm_data_set_items i "
                "            WHERE (p.item_id > 0 AND i.item_id = p.item_id) "
                "               OR (p.item_id = 0 AND p.device_id > 0 AND i.ref_device_id = p.device_id)), 0) "
                "FROM ga_players_inventory p WHERE p.id = ? AND p.user_id = ?");
            if (!st) return fail();
            sqlite3_bind_int64(st.s, 1, inventory_id); sqlite3_bind_int64(st.s, 2, user_id);
            if (sqlite3_step(st.s) != SQLITE_ROW) return fail();
            item_id = sqlite3_column_int(st.s, 0); profile = sqlite3_column_int(st.s, 1);
            sell_value = sqlite3_column_int64(st.s, 2); sell_cur = sqlite3_column_int64(st.s, 3);
            equipped = sqlite3_column_int(st.s, 4); bind = sqlite3_column_int64(st.s, 5);
        }
        (void)profile;
        if (equipped > 0) return fail();
        if (item_id == 0 && sell_value <= 0 && sell_cur == 0) return fail();   // starter kit
        if (bind == MailService::BIND_ON_ACQUIRE) return fail();              // bound-to-agency gear may go in
        const int log_item = ItemIdOfRow(db, inventory_id);
        Stmt ins(db, "INSERT INTO ga_agency_bank (agency_id, inventory_id, item_id, quantity, deposited_by, deposited_at) "
                     "VALUES (?, ?, ?, 1, ?, strftime('%s','now'))");
        if (!ins) return fail();
        sqlite3_bind_int64(ins.s, 1, agency_id); sqlite3_bind_int64(ins.s, 2, inventory_id);
        sqlite3_bind_int(ins.s, 3, item_id); sqlite3_bind_int64(ins.s, 4, character_id);
        if (!Step(db, ins)) return fail();
        const int64_t bank_id = sqlite3_last_insert_rowid(db);
        Stmt park(db, "UPDATE ga_players_inventory SET user_id = 0, stock_n = ? WHERE id = ? AND user_id = ?");
        if (!park) return fail();
        sqlite3_bind_int64(park.s, 1, -(1000000000LL + bank_id));
        sqlite3_bind_int64(park.s, 2, inventory_id); sqlite3_bind_int64(park.s, 3, user_id);
        if (!Step(db, park)) return fail();
        if (!AddLog(db, agency_id, EV_ITEM, character_id, log_item, 0, 1)) return fail();
        r.removed_rows.push_back(inventory_id);
    }
    if (!Exec(db, "COMMIT")) return fail();
    r.ok = true;
    Bump(agency_id);
    Logger::Log("agency", "[Bank] agency %lld: char %lld deposited inv %lld x%lld\n",
        (long long)agency_id, (long long)character_id, (long long)inventory_id, (long long)count);
    return r;
}

Result WithdrawItem(int64_t agency_id, int64_t character_id, int64_t client_inventory_id, int64_t count) {
    EnsureSchema();
    Result r;
    sqlite3* db = Database::GetConnection();
    const int64_t user_id = UserOf(db, character_id);
    if (user_id <= 0 || agency_id <= 0 || client_inventory_id <= 0) return r;
    const int my_profile = (int)Scalar(db, "SELECT profile_id FROM ga_characters WHERE id = ?", character_id);
    if (!Exec(db, "BEGIN IMMEDIATE")) return r;
    auto fail = [&]() { Exec(db, "ROLLBACK"); r = Result{}; return r; };

    if (client_inventory_id >= (int64_t)BANK_COMPONENT_BASE) {
        const int64_t bank_id = client_inventory_id - BANK_COMPONENT_BASE;
        int item_id = 0; int64_t have = 0;
        {
            Stmt st(db, "SELECT item_id, quantity FROM ga_agency_bank WHERE id = ? AND agency_id = ? AND inventory_id = 0");
            if (!st) return fail();
            sqlite3_bind_int64(st.s, 1, bank_id); sqlite3_bind_int64(st.s, 2, agency_id);
            if (sqlite3_step(st.s) != SQLITE_ROW) return fail();
            item_id = sqlite3_column_int(st.s, 0); have = sqlite3_column_int64(st.s, 1);
        }
        if (count <= 0 || count > have) count = have;
        {
            Stmt st(db, "UPDATE ga_agency_bank SET quantity = quantity - ?1 WHERE id = ?2 AND quantity >= ?1");
            if (!st) return fail();
            sqlite3_bind_int64(st.s, 1, count); sqlite3_bind_int64(st.s, 2, bank_id);
            if (!Step(db, st)) return fail();
        }
        {
            Stmt st(db, "DELETE FROM ga_agency_bank WHERE id = ? AND quantity <= 0");
            if (st) { sqlite3_bind_int64(st.s, 1, bank_id); sqlite3_step(st.s); }
        }
        Stmt st(db,
            "INSERT INTO ga_user_components (user_id, item_id, quantity, updated_at) VALUES (?, ?, ?, strftime('%s','now')) "
            "ON CONFLICT(user_id, item_id) DO UPDATE SET quantity = quantity + excluded.quantity, updated_at = excluded.updated_at");
        if (!st) return fail();
        sqlite3_bind_int64(st.s, 1, user_id); sqlite3_bind_int(st.s, 2, item_id); sqlite3_bind_int64(st.s, 3, count);
        if (!Step(db, st)) return fail();
        if (!AddLog(db, agency_id, EV_ITEM, character_id, item_id, 0, -count)) return fail();
        r.components_changed = true;
        r.item_id = item_id;
        r.count = count;
    } else {
        const int64_t inventory_id = client_inventory_id;
        int64_t bank_id = 0; int profile = 0;
        {
            Stmt st(db,
                "SELECT b.id, COALESCE(p.profile_id,0) FROM ga_agency_bank b JOIN ga_players_inventory p "
                "  ON p.id = b.inventory_id AND p.user_id = 0 AND p.stock_n = -(1000000000 + b.id) "
                "WHERE b.inventory_id = ? AND b.agency_id = ?");
            if (!st) return fail();
            sqlite3_bind_int64(st.s, 1, inventory_id); sqlite3_bind_int64(st.s, 2, agency_id);
            if (sqlite3_step(st.s) != SQLITE_ROW) return fail();
            bank_id = sqlite3_column_int64(st.s, 0); profile = sqlite3_column_int(st.s, 1);
        }
        if (profile != 0 && my_profile != 0 && profile != my_profile) return fail();   // class gear of another class
        const int log_item = ItemIdOfRow(db, inventory_id);
        {
            Stmt st(db, "DELETE FROM ga_agency_bank WHERE id = ? AND agency_id = ?");
            if (!st) return fail();
            sqlite3_bind_int64(st.s, 1, bank_id); sqlite3_bind_int64(st.s, 2, agency_id);
            if (!Step(db, st)) return fail();
        }
        Stmt st(db,
            "UPDATE ga_players_inventory SET user_id = ?1, character_id = ?3, stock_n = "
            " (SELECT COALESCE(MAX(p2.stock_n), -1) + 1 FROM ga_players_inventory p2 "
            "  WHERE p2.user_id = ?1 AND p2.item_id = ga_players_inventory.item_id) "
            "WHERE id = ?2 AND user_id = 0 AND stock_n = ?4");
        if (!st) return fail();
        sqlite3_bind_int64(st.s, 1, user_id); sqlite3_bind_int64(st.s, 2, inventory_id);
        sqlite3_bind_int64(st.s, 3, character_id); sqlite3_bind_int64(st.s, 4, -(1000000000LL + bank_id));
        if (!Step(db, st)) return fail();
        if (!AddLog(db, agency_id, EV_ITEM, character_id, log_item, 0, -1)) return fail();
        r.gained_rows.push_back(inventory_id);
        count = 1;
    }
    if (!Exec(db, "COMMIT")) return fail();
    r.ok = true;
    Bump(agency_id);
    Logger::Log("agency", "[Bank] agency %lld: char %lld withdrew %lld x%lld\n",
        (long long)agency_id, (long long)character_id, (long long)client_inventory_id, (long long)count);
    return r;
}

static Result MoveCredits(int64_t agency_id, int64_t character_id, int64_t amount, bool deposit) {
    EnsureSchema();
    Result r;
    sqlite3* db = Database::GetConnection();
    if (agency_id <= 0 || character_id <= 0 || amount <= 0 || amount > MAX_CREDITS) return r;
    if (!Exec(db, "BEGIN IMMEDIATE")) return r;
    auto fail = [&]() { Exec(db, "ROLLBACK"); r = Result{}; return r; };
    {
        Stmt st(db, deposit
            ? "UPDATE ga_characters SET credits = credits - ?1 WHERE id = ?2 AND deleted_at = 0 AND credits >= ?1"
            : "UPDATE ga_agencies SET bank_credits = bank_credits - ?1 WHERE id = ?2 AND bank_credits >= ?1");
        if (!st) return fail();
        sqlite3_bind_int64(st.s, 1, amount); sqlite3_bind_int64(st.s, 2, deposit ? character_id : agency_id);
        if (!Step(db, st)) return fail();
    }
    {
        Stmt st(db, deposit
            ? "UPDATE ga_agencies SET bank_credits = bank_credits + ?1 WHERE id = ?2"
            : "UPDATE ga_characters SET credits = credits + ?1 WHERE id = ?2 AND deleted_at = 0");
        if (!st) return fail();
        sqlite3_bind_int64(st.s, 1, amount); sqlite3_bind_int64(st.s, 2, deposit ? agency_id : character_id);
        if (!Step(db, st)) return fail();
    }
    if (!AddLog(db, agency_id, EV_CURRENCY, character_id, 0, deposit ? amount : -amount, 0)) return fail();
    if (!Exec(db, "COMMIT")) return fail();
    r.ok = true;
    r.credits = deposit ? -amount : amount;
    Bump(agency_id);
    Logger::Log("agency", "[Bank] agency %lld: char %lld %s %lld credits\n",
        (long long)agency_id, (long long)character_id, deposit ? "deposited" : "withdrew", (long long)amount);
    return r;
}

Result DepositCredits(int64_t agency_id, int64_t character_id, int64_t amount) {
    return MoveCredits(agency_id, character_id, amount, true);
}
Result WithdrawCredits(int64_t agency_id, int64_t character_id, int64_t amount) {
    return MoveCredits(agency_id, character_id, amount, false);
}

bool SetTax(int64_t agency_id, int64_t character_id, float percent) {
    EnsureSchema();
    if (!std::isfinite(percent)) return false;
    if (percent < 0) percent = 0;
    if (percent > MAX_TAX_PERCENT) percent = MAX_TAX_PERCENT;
    sqlite3* db = Database::GetConnection();
    Stmt st(db, "UPDATE ga_agencies SET tax_percent = ? WHERE id = ?");
    if (!st) return false;
    sqlite3_bind_double(st.s, 1, percent); sqlite3_bind_int64(st.s, 2, agency_id);
    if (!Step(db, st)) return false;
    Bump(agency_id);
    Logger::Log("agency", "[Bank] agency %lld: char %lld set tax %.1f%%\n",
        (long long)agency_id, (long long)character_id, percent);
    return true;
}

void NormalizeRanks(std::vector<Database::AgencyRankRow>& ranks) {
    int next = 0;
    for (const auto& r : ranks) next = std::max(next, r.rank_id + 1);
    std::set<int> seen;
    for (auto& r : ranks) {
        const int was = r.rank_id;
        if (!seen.insert(r.rank_id).second) { r.rank_id = next++; seen.insert(r.rank_id); }
        Logger::Log("agency", "[Ranks] row id=%d%s level=%d perm=0x%X name='%s'\n",
            r.rank_id, was != r.rank_id ? " (dup, new id)" : "", r.rank_level, (unsigned)r.permissions, r.rank_name.c_str());
    }
}

const char* BagBoundFlag(int item_id) {
    if (item_id <= 0) return "T";
    static std::mutex mu;
    static std::map<int, int> bind;   // asm item id -> bind type (static data, loaded once)
    std::lock_guard<std::mutex> lk(mu);
    if (bind.empty()) {
        sqlite3* db = Database::GetConnection();
        Stmt st(db, "SELECT item_id, MAX(COALESCE(item_bind_type_value_id,0)) FROM asm_data_set_items GROUP BY item_id");
        if (st) while (sqlite3_step(st.s) == SQLITE_ROW)
            bind[sqlite3_column_int(st.s, 0)] = sqlite3_column_int(st.s, 1);
        if (bind.empty()) bind[0] = 0;
    }
    auto it = bind.find(item_id);
    const int b = it == bind.end() ? 0 : it->second;
    return (b == MailService::BIND_ON_ACQUIRE || b == MailService::BIND_TO_AGENCY) ? "T" : "F";
}

int64_t TaxEarnings(int64_t character_id, int64_t credits) {
    if (character_id <= 0 || credits <= 0) return 0;
    EnsureSchema();
    sqlite3* db = Database::GetConnection();
    const int64_t user_id = UserOf(db, character_id);
    const int64_t agency_id = user_id > 0 ? Database::GetAgencyIdForUser(user_id) : 0;
    if (agency_id <= 0) return 0;
    double pct = 0;
    {
        Stmt st(db, "SELECT tax_percent FROM ga_agencies WHERE id = ?");
        if (!st) return 0;
        sqlite3_bind_int64(st.s, 1, agency_id);
        if (sqlite3_step(st.s) == SQLITE_ROW) pct = sqlite3_column_double(st.s, 0);
    }
    if (pct <= 0) return 0;
    if (pct > MAX_TAX_PERCENT) pct = MAX_TAX_PERCENT;
    const int64_t tax = (int64_t)std::floor((double)credits * pct / 100.0);
    if (tax <= 0) return 0;
    // SAVEPOINT: the reward paths may already sit inside a transaction
    if (!Exec(db, "SAVEPOINT agency_tax")) return 0;
    auto fail = [&]() { Exec(db, "ROLLBACK TO agency_tax"); Exec(db, "RELEASE agency_tax"); return (int64_t)0; };
    {
        Stmt st(db, "UPDATE ga_characters SET credits = credits - ?1 WHERE id = ?2 AND credits >= ?1");
        if (!st) return fail();
        sqlite3_bind_int64(st.s, 1, tax); sqlite3_bind_int64(st.s, 2, character_id);
        if (!Step(db, st)) return fail();
    }
    {
        Stmt st(db, "UPDATE ga_agencies SET bank_credits = bank_credits + ? WHERE id = ?");
        if (!st) return fail();
        sqlite3_bind_int64(st.s, 1, tax); sqlite3_bind_int64(st.s, 2, agency_id);
        if (!Step(db, st)) return fail();
    }
    if (!AddLog(db, agency_id, EV_TAX_INCOME, character_id, 0, tax, 0)) return fail();
    if (!Exec(db, "RELEASE agency_tax")) return fail();
    Bump(agency_id);
    Logger::Log("agency", "[Bank] tax: char %lld earned %lld -> %lld (%.1f%%) to agency %lld\n",
        (long long)character_id, (long long)credits, (long long)tax, pct, (long long)agency_id);
    TcpSession::AgencyTaxPaid(character_id, user_id, tax);
    return tax;
}

std::vector<LogRow> GetLog(int64_t agency_id, uint32_t change_type, const std::string& name,
                           int64_t from_unix, int64_t to_unix, int limit) {
    EnsureSchema();
    std::vector<LogRow> out;
    sqlite3* db = Database::GetConnection();
    Stmt st(db,
        "SELECT at, change_type, player_name, item_id, currency, count FROM ga_agency_bank_log "
        "WHERE agency_id = ?1 AND (?2 = 0 OR change_type = ?2) AND (?3 = '' OR player_name LIKE ?3) "
        "AND (?4 = 0 OR at >= ?4) AND (?5 = 0 OR at < ?5) ORDER BY id DESC LIMIT ?6");
    if (!st) return out;
    sqlite3_bind_int64(st.s, 1, agency_id);
    sqlite3_bind_int(st.s, 2, (int)change_type);
    sqlite3_bind_text(st.s, 3, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(st.s, 4, from_unix);
    sqlite3_bind_int64(st.s, 5, to_unix);
    sqlite3_bind_int(st.s, 6, limit);
    while (sqlite3_step(st.s) == SQLITE_ROW) {
        LogRow r;
        r.at = sqlite3_column_int64(st.s, 0);
        r.change_type = (uint32_t)sqlite3_column_int(st.s, 1);
        r.player_name = ColText(st.s, 2);
        r.item_id = sqlite3_column_int(st.s, 3);
        r.currency = sqlite3_column_int64(st.s, 4);
        r.count = sqlite3_column_int64(st.s, 5);
        out.push_back(r);
    }
    return out;
}

} // namespace AgencyBank

// ── agency chat lines ──────────────────────────────────────────────────────
namespace AgencyChat {
namespace {
constexpr uint32_t kAgencyChannel = 2;
constexpr int64_t  kReconnectSecs = 45;   // chat socket drops on zone travel: no notices for a quick rejoin
std::mutex g_mu;
std::map<int64_t, std::pair<int64_t, std::string>> g_left;   // user -> (left at, name)
std::map<std::string, std::pair<int64_t, std::string>> g_online;   // chat guid -> (user, name); the session store may be gone at teardown

void Put2(std::vector<uint8_t>& b, uint16_t v) { b.push_back(v & 0xFF); b.push_back(v >> 8); }
void Put4(std::vector<uint8_t>& b, uint16_t tag, uint32_t v) {
    Put2(b, tag); for (int i = 0; i < 4; ++i) b.push_back((v >> (8 * i)) & 0xFF);
}
void PutStr(std::vector<uint8_t>& b, uint16_t tag, const std::string& s) {
    Put2(b, tag); Put2(b, (uint16_t)s.size()); b.insert(b.end(), s.begin(), s.end());
}

std::vector<uint8_t> MsgFrame(uint32_t msg_id, const std::string& player_name, const std::string& motd,
                              int64_t currency, bool announcement) {
    std::vector<uint8_t> body;
    uint16_t n = 2;
    Put2(body, GA_U::CHAT_MESSAGE);
    Put2(body, 0);
    Put4(body, GA_T::CHAT_CH_TYPE, kAgencyChannel);
    Put4(body, GA_T::MSG_ID, msg_id);
    if (announcement) { Put2(body, GA_T::CHAT_ANNOUNCEMENT_FLAG); body.push_back(1); ++n; }
    if (!player_name.empty()) { PutStr(body, GA_T::PLAYER_NAME, player_name); ++n; }
    if (!motd.empty()) { PutStr(body, GA_T::AGENCY_MOTD, motd); ++n; }
    if (currency != 0) { Put4(body, GA_T::CURRENCY_VALUE, (uint32_t)currency); ++n; }
    body[2] = n & 0xFF; body[3] = n >> 8;
    std::vector<uint8_t> frame;
    Put2(frame, (uint16_t)body.size());
    frame.insert(frame.end(), body.begin(), body.end());
    return frame;
}

void FlushPendingOffline() {
    std::vector<std::pair<int64_t, std::string>> due;
    const int64_t now = (int64_t)std::time(nullptr);
    {
        std::lock_guard<std::mutex> l(g_mu);
        for (auto it = g_left.begin(); it != g_left.end();) {
            if (now - it->second.first >= kReconnectSecs) {
                bool still_on = false;   // a newer chat session of the same account is up
                for (const auto& [g, un] : g_online) if (un.first == it->first) { still_on = true; break; }
                if (it->second.first > 0 && !still_on) due.push_back({it->first, it->second.second});
                if (now - it->second.first > 600) { it = g_left.erase(it); continue; }
                it->second.first = -it->second.first;   // notice sent; keep the row a while (negative = done)
            }
            ++it;
        }
    }
    for (const auto& [uid, name] : due) {
        const int64_t agency = Database::GetAgencyIdForUser(uid);
        if (agency == 0) continue;
        std::vector<int64_t> others;
        for (int64_t u : Database::GetAgencyMemberUserIds(agency)) if (u != uid) others.push_back(u);
        SendMsgToUsers(others, AgencyBank::MSG_WENT_OFFLINE, name, "", 0, true);
    }
}
} // namespace

void SendMsgToUsers(const std::vector<int64_t>& user_ids, uint32_t msg_id,
                    const std::string& player_name, const std::string& motd, int64_t currency,
                    bool announcement) {
    if (user_ids.empty()) return;
    ChatSession::DeliverFrameToUsers(user_ids, MsgFrame(msg_id, player_name, motd, currency, announcement));
}

void OnChatJoin(const std::string& session_guid) {
    FlushPendingOffline();
    auto info = PlayerSessionStore::GetByGuid(session_guid);
    if (!info || info->user_id <= 0) return;
    bool rejoin = false;
    {
        std::lock_guard<std::mutex> l(g_mu);
        auto it = g_left.find(info->user_id);
        if (it != g_left.end()) {
            rejoin = it->second.first > 0 && (int64_t)std::time(nullptr) - it->second.first < kReconnectSecs;
            g_left.erase(it);
        }
    }
    {
        std::lock_guard<std::mutex> l(g_mu);
        g_online[session_guid] = {info->user_id, info->player_name};
    }
    if (rejoin) return;
    const int64_t agency = Database::GetAgencyIdForUser(info->user_id);
    if (agency == 0) return;
    auto ag = Database::GetAgencyInfo(agency);
    if (!ag) return;
    if (!ag->motd.empty())
        SendMsgToUsers({info->user_id}, AgencyBank::MSG_MOTD, "", ag->motd, 0, false);
    std::vector<int64_t> others;
    for (const auto& m : ag->members) if (m.user_id != info->user_id) others.push_back(m.user_id);
    SendMsgToUsers(others, AgencyBank::MSG_CAME_ONLINE, info->player_name, "", 0, true);
    Logger::Log("agency", "[Chat] login lines: user %lld agency %lld motd=%d notified=%zu\n",
        (long long)info->user_id, (long long)agency, ag->motd.empty() ? 0 : 1, others.size());
}

void OnChatLeave(const std::string& session_guid, const asio::any_io_executor& ex) {
    int64_t uid = 0; std::string name;
    {
        std::lock_guard<std::mutex> l(g_mu);
        auto it = g_online.find(session_guid);
        if (it != g_online.end()) { uid = it->second.first; name = it->second.second; g_online.erase(it); }
    }
    if (uid <= 0) {
        auto info = PlayerSessionStore::GetByGuid(session_guid);
        if (!info || info->user_id <= 0) return;
        uid = info->user_id; name = info->player_name;
    }
    {
        std::lock_guard<std::mutex> l(g_mu);
        g_left[uid] = {(int64_t)std::time(nullptr), name};
    }
    // Send the offline line once the reconnect window has passed (no roster poll needed)
    auto t = std::make_shared<asio::steady_timer>(ex, std::chrono::seconds(kReconnectSecs + 2));
    t->async_wait([t](const asio::error_code& ec) { if (!ec) FlushPendingOffline(); });
}

void Tick() { FlushPendingOffline(); }
} // namespace AgencyChat

// ── TcpSession side ────────────────────────────────────────────────────────
namespace {
std::mutex g_sent_mu;
std::map<std::string, uint64_t> g_sent;   // session guid -> bank version last shipped

bool AgencyPerm(const Database::AgencyInfo& info, int64_t user_id, uint32_t bit) {
    for (const auto& m : info.members) {
        if (m.user_id != user_id) continue;
        for (const auto& rk : info.ranks)
            if (rk.rank_id == m.rank_id) return rk.rank_level == 0 || ((uint32_t)rk.permissions & bit) != 0;
    }
    return false;
}

InventoryRow BankRow(const AgencyBank::Item& it) {
    InventoryRow row{};
    row.inventory_id = (int)it.ClientId();
    row.profile_id = it.profile_id;
    row.device_id = it.device_id;
    row.quality = it.quality;
    row.oc = it.oc != 0;
    row.item_id = it.item_id;
    for (size_t p = 0, q; p < it.mods.size(); p = q + 1) {
        q = it.mods.find(',', p);
        if (q == std::string::npos) q = it.mods.size();
        if (q > p) row.mods.push_back(std::atoi(it.mods.substr(p, q - p).c_str()));
    }
    return row;
}

// client date fields -> unix seconds, 0 = unparsed. The client sends "YYYYMMDD  H:MM:SS"
// (seen from the client); "MM/DD/YYYY" kept as a fallback.
int64_t ParseDate(const std::string& s, bool end_of_day) {
    int m = 0, d = 0, y = 0, ymd = 0;
    if (std::sscanf(s.c_str(), "%8d", &ymd) == 1 && ymd > 20000000) { y = ymd / 10000; m = ymd / 100 % 100; d = ymd % 100; }
    else if (std::sscanf(s.c_str(), "%d/%d/%d", &m, &d, &y) != 3) return 0;
    if (y < 2000 || m < 1 || m > 12 || d < 1 || d > 31) return 0;
    std::tm t{}; t.tm_year = y - 1900; t.tm_mon = m - 1; t.tm_mday = d;
    const int64_t v = (int64_t)timegm(&t);
    return end_of_day ? v + 86400 : v;
}
} // namespace

// Bank block of the roster (ROSTER_FLAGS bit 0x10), only when the bank changed
// since this session last got it. Returns the number of fields appended.
int TcpSession::append_agency_bank_fields(std::vector<uint8_t>& response, int64_t agency_id, bool with_items)
{
    AgencyChat::Tick();
    const auto bank = AgencyBank::Load(agency_id);
    WriteFloat(response, GA_T::TAX_PERCENT, bank.tax_percent);
    if (!with_items) return 1;
    Write4B(response, GA_T::CURRENCY_VALUE, (uint32_t)bank.credits);
    std::vector<InventoryRow> rows;
    append(response, GA_T::DATA_SET_AGENCY_INVENTORY & 0xFF, GA_T::DATA_SET_AGENCY_INVENTORY >> 8);
    append(response, bank.items.size() & 0xFF, (bank.items.size() >> 8) & 0xFF);
    for (const auto& it : bank.items) {
        rows.push_back(BankRow(it));
        AppendBagRecord(response, rows.back(), (uint32_t)it.quantity);
    }
    size_t state_rows = 0;
    for (const auto& row : rows) state_rows += row.mods.empty() ? 1 : row.mods.size();
    append(response, GA_T::DATA_SET_INVENTORY_STATE & 0xFF, GA_T::DATA_SET_INVENTORY_STATE >> 8);
    append(response, state_rows & 0xFF, (state_rows >> 8) & 0xFF);
    for (const auto& row : rows) {
        std::vector<int> eg = row.mods;
        if (eg.empty()) eg.push_back(0);
        for (int e : eg) {
            append(response, 0x02, 0x00);
            Write4B(response, GA_T::INVENTORY_ID, (uint32_t)row.inventory_id);
            Write4B(response, GA_T::EFFECT_GROUP_ID, (uint32_t)e);
        }
    }
    return 4;
}

bool TcpSession::agency_bank_due(int64_t agency_id)
{
    const uint64_t v = AgencyBank::Version(agency_id);
    std::lock_guard<std::mutex> l(g_sent_mu);
    if (g_sent.size() > 2000) g_sent.clear();
    uint64_t& last = g_sent[session_guid_];
    if (last == v) return false;
    last = v;
    return true;
}

void TcpSession::AgencyTaxPaid(int64_t character_id, int64_t user_id, int64_t tax)
{
    std::shared_ptr<TcpSession> s;
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        for (auto& kv : g_sessions_)
            if (auto c = kv.second.lock())
                if (c->selected_character_id_ == character_id) { s = c; break; }
    }
    AgencyChat::SendMsgToUsers({user_id}, AgencyBank::MSG_TAX_EARNED, "", "", tax, false);
    if (s) {
        auto self = s;
        asio::post(s->io_ctx_, [self]() { self->send_mail_notify(0, true); });
    }
}

void TcpSession::handle_agency_bank_packet(uint16_t packet_type, const PacketView& pkt)
{
    const int64_t me = selected_character_id_;
    if (me <= 0) return;
    const int64_t agency_id = Database::GetAgencyIdForUser(user_id_);
    if (agency_id == 0) { send_agency_message(14265, player_name); return; }
    auto info = Database::GetAgencyInfo(agency_id);
    if (!info) return;
    const bool can_remove = AgencyPerm(*info, user_id_, Database::AGENCY_PERM_INVENTORY_REMOVAL);
    auto RefreshAgencyMembers = [](int64_t agency) {
        for (int64_t uid : Database::GetAgencyMemberUserIds(agency)) DeliverAgencyRefresh(uid);
    };

    switch (packet_type) {
    case GA_U::AGENCY_UPDATE_TAX: {
        const float pct = pkt.ReadFloat(GA_T::TAX_PERCENT).value_or(-1.0f);
        Logger::Log("agency", "[Bank] UPDATE_TAX user %lld value %.3f\n", (long long)user_id_, pct);
        if (info->leader_user_id != user_id_) { send_agency_message(AgencyBank::MSG_NO_TAX_PERM, player_name); break; }
        if (pct >= 0 && AgencyBank::SetTax(agency_id, me, pct)) RefreshAgencyMembers(agency_id);
        break;
    }
    case GA_U::AGENCY_ADD_INVENTORY: {
        const int64_t inv = (int64_t)pkt.Read4B(GA_T::INVENTORY_ID).value_or(0);
        const int64_t cnt = (int64_t)pkt.Read4B(GA_T::INSTANCE_COUNT).value_or(1);
        const auto r = AgencyBank::DepositItem(agency_id, me, inv, cnt <= 0 ? 1 : cnt);
        Logger::Log("agency", "[Bank] ADD_INVENTORY user %lld inv %lld cnt %lld -> %s\n",
            (long long)user_id_, (long long)inv, (long long)cnt, r.ok ? "ok" : "refused");
        if (r.ok) {
            push_mail_bag_changes(r.removed_rows, {}, r.components_changed, r.depleted_components);
            RefreshAgencyMembers(agency_id);
        } else {
            send_mail_notify(MailService::MSG_SEND_FAILED, false);
        }
        break;
    }
    case GA_U::AGENCY_REMOVE_INVENTORY:
    case GA_U::AGENCY_MAIL_INVENTORY: {
        const int64_t inv = (int64_t)pkt.Read4B(GA_T::INVENTORY_ID).value_or(0);
        const int64_t cnt = (int64_t)pkt.Read4B(GA_T::INSTANCE_COUNT).value_or(1);
        const std::string to = pkt.ReadString(GA_T::PLAYER_NAME).value_or("");
        Logger::Log("agency", "[Bank] %s user %lld inv %lld cnt %lld to '%s'\n",
            packet_type == GA_U::AGENCY_MAIL_INVENTORY ? "MAIL_INVENTORY" : "REMOVE_INVENTORY",
            (long long)user_id_, (long long)inv, (long long)cnt, to.c_str());
        if (!can_remove) { send_agency_message(AgencyBank::MSG_NO_WITHDRAW_PERM, player_name); break; }
        const auto r = AgencyBank::WithdrawItem(agency_id, me, inv, cnt);
        if (!r.ok) { send_mail_notify(MailService::MSG_ACCEPT_FAILED, false); break; }
        push_mail_bag_changes({}, r.gained_rows, r.components_changed);
        if (packet_type == GA_U::AGENCY_MAIL_INVENTORY && !to.empty()) {
            // "Send To": out of the bank, then mailed from the withdrawer's bag (refused mail = stays in the bag)
            std::vector<MailService::Attachment> att;
            if (!r.gained_rows.empty()) att.push_back({r.gained_rows[0], 1});
            else if (r.item_id > 0) att.push_back({(int64_t)MailService::COMPONENT_INV_BASE + r.item_id, r.count});
            if (!att.empty()) {
                const auto m = MailService::Send(me, to, "Agency " + info->name, "From the agency inventory.", 0,
                                                 MailService::TYPE_NORMAL, att);
                if (m.ok) { push_mail_bag_changes(m.removed_rows, {}, m.components_changed, m.depleted_components);
                            DeliverNewMail(m.to_character_id); }
                else send_mail_notify(m.error_msg_id, false);
            }
        }
        RefreshAgencyMembers(agency_id);
        break;
    }
    case GA_U::AGENCY_UPDATE_CURRENCY: {
        const int64_t v = (int64_t)(int32_t)pkt.Read4B(GA_T::CURRENCY_VALUE).value_or(0);
        Logger::Log("agency", "[Bank] UPDATE_CURRENCY user %lld value %lld\n", (long long)user_id_, (long long)v);
        if (v == 0) break;
        // The value is the change to the PLAYER's wallet - Deposit sends -N (seen on the
        // real client), Withdraw +N.
        if (v > 0 && !can_remove) { send_agency_message(AgencyBank::MSG_NO_WITHDRAW_PERM, player_name); break; }
        const auto r = v < 0 ? AgencyBank::DepositCredits(agency_id, me, -v)
                             : AgencyBank::WithdrawCredits(agency_id, me, v);
        send_mail_notify(r.ok ? 0 : MailService::MSG_SEND_FAILED, true);
        if (r.ok) RefreshAgencyMembers(agency_id);
        break;
    }
    case GA_U::GET_AGENCY_INV_LOG: {
        const uint32_t type = pkt.Read4B(GA_T::CHANGE_TYPE_VALUE_ID).value_or(0);
        const std::string name = pkt.ReadString(GA_T::PLAYER_NAME).value_or("");
        const std::string ds = pkt.ReadString(GA_T::DATE_START).value_or("");
        const std::string de = pkt.ReadString(GA_T::DATE_END).value_or("");
        Logger::Log("agency", "[Bank] INV_LOG user %lld type %u name '%s' dates '%s'..'%s'\n",
            (long long)user_id_, type, name.c_str(), ds.c_str(), de.c_str());
        const auto rows = AgencyBank::GetLog(agency_id, type, name, ParseDate(ds, false), ParseDate(de, true), 100);
        std::vector<uint8_t> response;
        append(response, GA_U::GET_AGENCY_INV_LOG & 0xFF, GA_U::GET_AGENCY_INV_LOG >> 8);
        append(response, 0x02, 0x00);
        Write4B(response, GA_T::COUNT, (uint32_t)rows.size());
        append(response, GA_T::DATA_SET & 0xFF, GA_T::DATA_SET >> 8);
        append(response, rows.size() & 0xFF, (rows.size() >> 8) & 0xFF);
        for (const auto& r : rows) {
            append(response, 0x06, 0x00);
            WriteDouble(response, GA_T::CHANGE_DATETIME, ((double)r.at + 2208988800.0) / 86400.0);
            Write4B(response, GA_T::CHANGE_TYPE_VALUE_ID, r.change_type);
            WriteString(response, GA_T::PLAYER_NAME, r.player_name);
            Write4B(response, GA_T::ITEM_ID, (uint32_t)r.item_id);
            Write4B(response, GA_T::CURRENCY_VALUE, (uint32_t)(int32_t)r.currency);
            Write4B(response, GA_T::INSTANCE_COUNT, (uint32_t)(int32_t)r.count);
        }
        send_response(response);
        break;
    }
    default: break;
    }
}
