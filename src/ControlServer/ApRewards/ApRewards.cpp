#include "src/ControlServer/ApRewards/ApRewards.hpp"
#include "src/ControlServer/Database/Database.hpp"
#include "src/ControlServer/Logger.hpp"
#include "src/ControlServer/ServiceHooks/ServiceHooks.hpp"
#include "sqlite3.h"
#include <algorithm>
#include <ctime>

namespace ApRewards {

namespace {
Settings g_settings;
}

void Configure(const Settings& s) { g_settings = s; }
const Settings& Get() { return g_settings; }

void EnsureSchema() {
    sqlite3* db = Database::GetConnection();
    if (!db) return;
    sqlite3_exec(db,
        "CREATE TABLE IF NOT EXISTS ga_ap_grants ("
        " id INTEGER PRIMARY KEY AUTOINCREMENT, user_id INTEGER NOT NULL,"
        " character_id INTEGER NOT NULL, source TEXT NOT NULL, base INTEGER NOT NULL,"
        " amount INTEGER NOT NULL, capped INTEGER NOT NULL DEFAULT 0,"
        " day INTEGER NOT NULL, at INTEGER NOT NULL);"
        "CREATE INDEX IF NOT EXISTS ga_ap_grants_char_day ON ga_ap_grants(character_id, day);"
        // one raid-clear award per agent per instance; one season-end payout per season
        "CREATE TABLE IF NOT EXISTS ga_ap_raid_clears (instance_id INTEGER NOT NULL,"
        " character_id INTEGER NOT NULL, PRIMARY KEY (instance_id, character_id));"
        "CREATE TABLE IF NOT EXISTS ga_ava_season_payouts (season INTEGER PRIMARY KEY,"
        " paid_at INTEGER NOT NULL, agents INTEGER NOT NULL DEFAULT 0);",
        nullptr, nullptr, nullptr);
}

int64_t Grant(int64_t character_id, int64_t level, const std::string& source,
              int64_t base, bool scaled, bool capped) {
    sqlite3* db = Database::GetConnection();
    if (!db || character_id <= 0 || base <= 0) return 0;
    const Settings& s = g_settings;
    const int64_t now = (int64_t)std::time(nullptr);
    const int64_t day = now / 86400;

    int64_t user_id = 0;
    sqlite3_stmt* st = nullptr;
    if (sqlite3_prepare_v2(db, "SELECT user_id FROM ga_characters WHERE id = ?",
                           -1, &st, nullptr) == SQLITE_OK) {
        sqlite3_bind_int64(st, 1, character_id);
        if (sqlite3_step(st) == SQLITE_ROW) user_id = sqlite3_column_int64(st, 0);
        sqlite3_finalize(st);
    }
    if (user_id <= 0) return 0;

    int64_t today = 0;
    if (capped) {
        if (sqlite3_prepare_v2(db,
                "SELECT COALESCE(SUM(amount),0) FROM ga_ap_grants "
                "WHERE character_id = ? AND day = ? AND capped = 1",
                -1, &st, nullptr) == SQLITE_OK) {
            sqlite3_bind_int64(st, 1, character_id);
            sqlite3_bind_int64(st, 2, day);
            if (sqlite3_step(st) == SQLITE_ROW) today = sqlite3_column_int64(st, 0);
            sqlite3_finalize(st);
        }
    }
    int64_t amount = base;
    if (capped && s.daily_cap > 0)
        amount = std::min(amount, std::max<int64_t>(0, s.daily_cap - today));
    if (ServiceHooks::Enabled("ap_grant")) {
        auto d = ServiceHooks::Call("ap_grant", {
            {"user_id", user_id}, {"char_id", character_id}, {"level", level},
            {"source", source}, {"base", base}, {"scaled", scaled}, {"capped", capped},
            {"today", today}, {"daily_cap", s.daily_cap}});
        if (d) amount = ServiceHooks::ClampInt(*d, "amount", amount, 0, 10000);
    }
    if (amount <= 0) return 0;

    if (sqlite3_prepare_v2(db,
            "UPDATE ga_users SET agenda_points = agenda_points + ?1 WHERE id = ?2",
            -1, &st, nullptr) != SQLITE_OK) return 0;
    sqlite3_bind_int64(st, 1, amount);
    sqlite3_bind_int64(st, 2, user_id);
    sqlite3_step(st);
    sqlite3_finalize(st);

    if (sqlite3_prepare_v2(db,
            "INSERT INTO ga_ap_grants (user_id, character_id, source, base, amount, capped, day, at) "
            "VALUES (?,?,?,?,?,?,?,?)", -1, &st, nullptr) == SQLITE_OK) {
        sqlite3_bind_int64(st, 1, user_id);
        sqlite3_bind_int64(st, 2, character_id);
        sqlite3_bind_text(st, 3, source.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(st, 4, base);
        sqlite3_bind_int64(st, 5, amount);
        sqlite3_bind_int(st, 6, capped ? 1 : 0);
        sqlite3_bind_int64(st, 7, day);
        sqlite3_bind_int64(st, 8, now);
        sqlite3_step(st);
        sqlite3_finalize(st);
    }
    Logger::Log("rewards", "[AP] user=%lld char=%lld source=%s +%lld (base %lld, level %lld)\n",
                (long long)user_id, (long long)character_id, source.c_str(),
                (long long)amount, (long long)base, (long long)level);
    return amount;
}

}  // namespace ApRewards
