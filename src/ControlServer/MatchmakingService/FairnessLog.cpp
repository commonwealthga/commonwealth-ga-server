#include "src/ControlServer/MatchmakingService/FairnessLog.hpp"
#include "src/ControlServer/Database/Database.hpp"
#include "src/ControlServer/Logger.hpp"
#include "sqlite3.h"

#include <ctime>

namespace FairnessLog {

namespace {

const char* Name(Event e) {
    switch (e) {
        case Event::Excluded: return "excluded";
        case Event::Played:   return "played";
        case Event::Defender: return "defender";
    }
    return "excluded";
}

// One (scope, user, type) read over idx_mm_fair_lookup, returning up to two
// aggregate columns. `extra` binds ?4 when >= 0. False on any failure.
bool Query2(const char* sql, const std::string& scope, int64_t user_id,
            const char* type, int64_t extra, int64_t* a, int64_t* b) {
    sqlite3* db = Database::GetConnection();
    if (!db) return false;
    sqlite3_stmt* st = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &st, nullptr) != SQLITE_OK || !st) {
        Logger::Log("db", "FairnessLog prepare failed: %s\n", sqlite3_errmsg(db));
        if (st) sqlite3_finalize(st);
        return false;
    }
    sqlite3_bind_text (st, 1, scope.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(st, 2, user_id);
    sqlite3_bind_text (st, 3, type, -1, SQLITE_STATIC);
    if (extra >= 0) sqlite3_bind_int64(st, 4, extra);
    bool ok = false;
    if (sqlite3_step(st) == SQLITE_ROW) {
        if (a) *a = sqlite3_column_int64(st, 0);
        if (b) *b = sqlite3_column_int64(st, 1);
        ok = true;
    }
    sqlite3_finalize(st);
    return ok;
}

}  // namespace

void Record(const std::string& scope, uint32_t queue_id, int64_t user_id,
            Event e, uint32_t profile_id, int64_t instance_id) {
    if (scope.empty() || user_id <= 0) return;
    sqlite3* db = Database::GetConnection();
    if (!db) return;
    sqlite3_stmt* st = nullptr;
    const char* sql =
        "INSERT INTO ga_matchmaking_fairness_events"
        " (user_id, scope, queue_id, event_type, profile_id, instance_id, created_at)"
        " VALUES (?, ?, ?, ?, ?, ?, ?)";
    if (sqlite3_prepare_v2(db, sql, -1, &st, nullptr) != SQLITE_OK || !st) {
        Logger::Log("db", "FairnessLog insert prepare failed: %s\n", sqlite3_errmsg(db));
        if (st) sqlite3_finalize(st);
        return;
    }
    sqlite3_bind_int64(st, 1, user_id);
    sqlite3_bind_text (st, 2, scope.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int  (st, 3, (int)queue_id);
    sqlite3_bind_text (st, 4, Name(e), -1, SQLITE_STATIC);
    sqlite3_bind_int  (st, 5, (int)profile_id);
    if (instance_id > 0) sqlite3_bind_int64(st, 6, instance_id);
    else                 sqlite3_bind_null (st, 6);
    sqlite3_bind_int64(st, 7, (int64_t)std::time(nullptr));
    if (sqlite3_step(st) != SQLITE_DONE)
        Logger::Log("db", "FairnessLog insert failed: %s\n", sqlite3_errmsg(db));
    sqlite3_finalize(st);
}

FairnessStats Load(const std::string& scope, int64_t user_id) {
    FairnessStats s;
    if (scope.empty() || user_id <= 0) return s;

    // Q1: watermark — id of this user's most recent 'played' row.
    int64_t last_played = 0, played = 0;
    Query2("SELECT COALESCE(MAX(id),0), COUNT(*) FROM ga_matchmaking_fairness_events"
           " WHERE scope = ? AND user_id = ? AND event_type = ?",
           scope, user_id, "played", -1, &last_played, &played);
    s.played_count = (uint32_t)played;

    // Q2: exclusions since the watermark ('recently excluded'), and all-time.
    int64_t recent = 0, lifetime = 0;
    Query2("SELECT COALESCE(SUM(id > ?4),0), COUNT(*) FROM ga_matchmaking_fairness_events"
           " WHERE scope = ?1 AND user_id = ?2 AND event_type = ?3",
           scope, user_id, "excluded", last_played, &recent, &lifetime);
    s.exclusion_count     = (uint32_t)recent;
    s.lifetime_exclusions = (uint32_t)lifetime;

    // Q3: defender recency + count.
    int64_t last_def = 0, def_count = 0;
    Query2("SELECT COALESCE(MAX(id),0), COUNT(*) FROM ga_matchmaking_fairness_events"
           " WHERE scope = ? AND user_id = ? AND event_type = ?",
           scope, user_id, "defender", -1, &last_def, &def_count);
    s.last_defender_id = last_def;
    s.defender_count   = (uint32_t)def_count;
    return s;
}

}  // namespace FairnessLog
