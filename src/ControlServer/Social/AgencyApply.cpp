#include "src/ControlServer/Social/AgencyApply.hpp"
#include "src/ControlServer/Database/Database.hpp"
#include "src/ControlServer/Logger.hpp"
#include "sqlite3.h"

#include <mutex>

namespace AgencyApply {
namespace {
std::once_flag g_once;
sqlite3* Db() {
    sqlite3* db = Database::GetConnection();
    std::call_once(g_once, [db]() {
        if (!db) return;
        sqlite3_exec(db,
            "CREATE TABLE IF NOT EXISTS ga_agency_applications ("
            " id INTEGER PRIMARY KEY AUTOINCREMENT, agency_id INTEGER NOT NULL, user_id INTEGER NOT NULL,"
            " character_id INTEGER NOT NULL, name TEXT NOT NULL DEFAULT '', text TEXT NOT NULL DEFAULT '',"
            " applied_at INTEGER NOT NULL, state INTEGER NOT NULL DEFAULT 2, decided_at INTEGER NOT NULL DEFAULT 0);"
            "CREATE INDEX IF NOT EXISTS ix_agency_app_agency ON ga_agency_applications(agency_id, state);"
            "CREATE INDEX IF NOT EXISTS ix_agency_app_user ON ga_agency_applications(user_id);",
            nullptr, nullptr, nullptr);
    });
    return db;
}
std::string Col(sqlite3_stmt* s, int c) {
    const unsigned char* t = sqlite3_column_text(s, c);
    return t ? std::string((const char*)t) : std::string();
}
const char* kSel =
    "SELECT p.id, p.agency_id, p.user_id, p.character_id, p.applied_at, p.state, p.name, p.text,"
    " COALESCE(a.name, ''), COALESCE(c.profile_id, 0), COALESCE(c.xp, 0)"
    " FROM ga_agency_applications p LEFT JOIN ga_agencies a ON a.id = p.agency_id"
    " LEFT JOIN ga_characters c ON c.id = p.character_id ";
App Row(sqlite3_stmt* s) {
    App a;
    a.id = sqlite3_column_int64(s, 0); a.agency_id = sqlite3_column_int64(s, 1);
    a.user_id = sqlite3_column_int64(s, 2); a.character_id = sqlite3_column_int64(s, 3);
    a.applied_at = sqlite3_column_int64(s, 4); a.state = sqlite3_column_int(s, 5);
    a.name = Col(s, 6); a.text = Col(s, 7); a.agency_name = Col(s, 8);
    a.profile_id = (uint32_t)sqlite3_column_int64(s, 9); a.xp = sqlite3_column_int64(s, 10);
    return a;
}
}  // namespace

std::optional<App> Latest(int64_t user_id) {
    sqlite3* db = Db(); sqlite3_stmt* s = nullptr; std::optional<App> out;
    const std::string q = std::string(kSel) +
        "WHERE p.user_id = ? AND (p.state = 2 OR p.decided_at >= strftime('%s','now') - 259200) ORDER BY p.id DESC LIMIT 1";
    if (db && sqlite3_prepare_v2(db, q.c_str(), -1, &s, nullptr) == SQLITE_OK) {
        sqlite3_bind_int64(s, 1, user_id);
        if (sqlite3_step(s) == SQLITE_ROW) out = Row(s);
        sqlite3_finalize(s);
    }
    return out;
}

bool Apply(int64_t agency_id, int64_t user_id, int64_t character_id, const std::string& name, const std::string& text) {
    sqlite3* db = Db(); if (!db) return false;
    Cancel(user_id);
    sqlite3_stmt* s = nullptr; bool ok = false;
    if (sqlite3_prepare_v2(db, "INSERT INTO ga_agency_applications (agency_id, user_id, character_id, name, text, applied_at, state)"
                               " VALUES (?, ?, ?, ?, ?, strftime('%s','now'), 2)", -1, &s, nullptr) == SQLITE_OK) {
        sqlite3_bind_int64(s, 1, agency_id); sqlite3_bind_int64(s, 2, user_id); sqlite3_bind_int64(s, 3, character_id);
        sqlite3_bind_text(s, 4, name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(s, 5, text.substr(0, 400).c_str(), -1, SQLITE_TRANSIENT);
        ok = sqlite3_step(s) == SQLITE_DONE;
        sqlite3_finalize(s);
    }
    Logger::Log("agency", "[AgencyApply] user %lld char %lld applied to agency %lld ok=%d\n",
        (long long)user_id, (long long)character_id, (long long)agency_id, ok ? 1 : 0);
    return ok;
}

bool Cancel(int64_t user_id) {
    sqlite3* db = Db(); sqlite3_stmt* s = nullptr; bool ok = false;
    if (db && sqlite3_prepare_v2(db, "DELETE FROM ga_agency_applications WHERE user_id = ? AND state = 2", -1, &s, nullptr) == SQLITE_OK) {
        sqlite3_bind_int64(s, 1, user_id);
        ok = sqlite3_step(s) == SQLITE_DONE && sqlite3_changes(db) > 0;
        sqlite3_finalize(s);
    }
    return ok;
}

std::vector<App> Pending(int64_t agency_id) {
    sqlite3* db = Db(); sqlite3_stmt* s = nullptr; std::vector<App> out;
    const std::string q = std::string(kSel) + "WHERE p.agency_id = ? AND p.state = 2 ORDER BY p.id LIMIT 100";
    if (db && sqlite3_prepare_v2(db, q.c_str(), -1, &s, nullptr) == SQLITE_OK) {
        sqlite3_bind_int64(s, 1, agency_id);
        while (sqlite3_step(s) == SQLITE_ROW) out.push_back(Row(s));
        sqlite3_finalize(s);
    }
    return out;
}

std::optional<App> PendingFor(int64_t agency_id, int64_t character_id) {
    for (auto& a : Pending(agency_id)) if (a.character_id == character_id) return a;
    return std::nullopt;
}

void SetState(int64_t app_id, int state) {
    sqlite3* db = Db(); sqlite3_stmt* s = nullptr;
    if (db && sqlite3_prepare_v2(db, "UPDATE ga_agency_applications SET state = ?, decided_at = strftime('%s','now') WHERE id = ?",
                                 -1, &s, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(s, 1, state); sqlite3_bind_int64(s, 2, app_id);
        sqlite3_step(s); sqlite3_finalize(s);
    }
}

}  // namespace AgencyApply
