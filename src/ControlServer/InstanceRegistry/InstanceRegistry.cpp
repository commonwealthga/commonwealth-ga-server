#include "src/ControlServer/InstanceRegistry/InstanceRegistry.hpp"
#include "src/ControlServer/Database/Database.hpp"
#include "src/ControlServer/Logger.hpp"
#include "sqlite3.h"
#include <set>

std::mutex InstanceRegistry::mutex_;
std::string InstanceRegistry::s_host_ = "127.0.0.1";

void InstanceRegistry::Init() {
    // No-op: mutex_ is default-constructed.
    // Kept for startup symmetry with PlayerSessionStore::Init().
}

void InstanceRegistry::SetHost(const std::string& host) {
    s_host_ = host;
}

std::optional<InstanceInfo> InstanceRegistry::GetReadyInstance(const std::string& map_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    sqlite3* db = Database::GetConnection();
    if (!db) return std::nullopt;

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db,
        "SELECT id, map_name, state, pid, udp_port, ip_address, player_count, "
        "       started_at, COALESCE(sealed_at, 0), "
        "       COALESCE(instance_id, 0), COALESCE(is_home_map, 0), COALESCE(max_players, 0), "
        "       COALESCE(game_mode, '') "
        "FROM ga_instances "
        "WHERE map_name = ? AND state = 'READY' "
        "LIMIT 1",
        -1, &stmt, nullptr);
    if (rc != SQLITE_OK || !stmt) {
        Logger::Log("db", "[InstanceRegistry] GetReadyInstance prepare failed: %s\n",
            sqlite3_errmsg(db));
        return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, map_name.c_str(), -1, SQLITE_TRANSIENT);

    std::optional<InstanceInfo> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        InstanceInfo info;
        info.id           = sqlite3_column_int64(stmt, 0);
        const char* mn    = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        info.map_name     = mn ? mn : "";
        const char* st    = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        info.state        = st ? st : "";
        info.pid          = sqlite3_column_int(stmt, 3);
        info.udp_port     = static_cast<uint16_t>(sqlite3_column_int(stmt, 4));
        const char* ip    = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        info.ip_address   = (ip && *ip) ? ip : "127.0.0.1";
        info.player_count = sqlite3_column_int(stmt, 6);
        info.started_at   = sqlite3_column_int64(stmt, 7);
        info.sealed_at    = sqlite3_column_int64(stmt, 8);
        info.instance_id  = sqlite3_column_int64(stmt, 9);
        info.is_home_map  = sqlite3_column_int(stmt, 10) != 0;
        info.max_players  = sqlite3_column_int(stmt, 11);
        const char* gm    = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 12));
        info.game_mode    = gm ? gm : "";
        result = std::move(info);
    }
    sqlite3_finalize(stmt);
    return result;
}

void InstanceRegistry::SeedHomeMapInstance(const std::string& map_name, uint16_t udp_port) {
    std::lock_guard<std::mutex> lock(mutex_);
    sqlite3* db = Database::GetConnection();
    if (!db) return;

    // Mark all stale (non-STOPPED) instances from a previous run as STOPPED.
    char* err = nullptr;
    int rc = sqlite3_exec(db,
        "UPDATE ga_instances SET state='STOPPED', sealed_at=strftime('%s','now') "
        "WHERE state != 'STOPPED'",
        nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        Logger::Log("db", "[InstanceRegistry] Failed to clear stale instances: %s\n", err);
        sqlite3_free(err);
        return;
    }

    // Insert a fresh READY row for the home map.
    sqlite3_stmt* stmt = nullptr;
    rc = sqlite3_prepare_v2(db,
        "INSERT INTO ga_instances "
        "(map_name, state, pid, udp_port, ip_address, player_count, started_at) "
        "VALUES (?, 'READY', 0, ?, ?, 0, strftime('%s','now'))",
        -1, &stmt, nullptr);
    if (rc != SQLITE_OK || !stmt) {
        Logger::Log("db", "[InstanceRegistry] SeedHomeMapInstance prepare failed: %s\n",
            sqlite3_errmsg(db));
        return;
    }

    sqlite3_bind_text(stmt, 1, map_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt,  2, static_cast<int>(udp_port));
    sqlite3_bind_text(stmt, 3, s_host_.c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        Logger::Log("db", "[InstanceRegistry] SeedHomeMapInstance insert failed: %s\n",
            sqlite3_errmsg(db));
        return;
    }

    Logger::Log("db", "[InstanceRegistry] Seeded instance: map=%s udp_port=%d\n",
        map_name.c_str(), (int)udp_port);
}

int64_t InstanceRegistry::InsertStarting(const std::string& map_name, const std::string& game_mode,
                                          uint16_t udp_port, int pid, bool is_home_map) {
    std::lock_guard<std::mutex> lock(mutex_);
    sqlite3* db = Database::GetConnection();
    if (!db) return 0;

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db,
        "INSERT INTO ga_instances "
        "(map_name, game_mode, state, pid, udp_port, ip_address, player_count, started_at, instance_id, is_home_map) "
        "VALUES (?, ?, 'STARTING', ?, ?, ?, 0, strftime('%s','now'), 0, ?)",
        -1, &stmt, nullptr);
    if (rc != SQLITE_OK || !stmt) {
        Logger::Log("db", "[InstanceRegistry] InsertStarting prepare failed: %s\n",
            sqlite3_errmsg(db));
        return 0;
    }

    sqlite3_bind_text(stmt, 1, map_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, game_mode.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt,  3, pid);
    sqlite3_bind_int(stmt,  4, static_cast<int>(udp_port));
    sqlite3_bind_text(stmt, 5, s_host_.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt,  6, is_home_map ? 1 : 0);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        Logger::Log("db", "[InstanceRegistry] InsertStarting insert failed: %s\n",
            sqlite3_errmsg(db));
        return 0;
    }

    int64_t rowid = sqlite3_last_insert_rowid(db);

    // Use rowid as instance_id
    char* err = nullptr;
    char sql[128];
    snprintf(sql, sizeof(sql),
        "UPDATE ga_instances SET instance_id = %lld WHERE id = %lld",
        (long long)rowid, (long long)rowid);
    rc = sqlite3_exec(db, sql, nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        Logger::Log("db", "[InstanceRegistry] InsertStarting update instance_id failed: %s\n", err);
        sqlite3_free(err);
    }

    Logger::Log("db", "[InstanceRegistry] InsertStarting: map=%s udp_port=%d pid=%d is_home_map=%d -> instance_id=%lld\n",
        map_name.c_str(), (int)udp_port, pid, (int)is_home_map, (long long)rowid);
    return rowid;
}

void InstanceRegistry::UpdatePid(int64_t instance_id, int pid) {
    std::lock_guard<std::mutex> lock(mutex_);
    sqlite3* db = Database::GetConnection();
    if (!db) return;

    char* err = nullptr;
    char sql[128];
    snprintf(sql, sizeof(sql),
        "UPDATE ga_instances SET pid = %d WHERE instance_id = %lld",
        pid, (long long)instance_id);
    int rc = sqlite3_exec(db, sql, nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        Logger::Log("db", "[InstanceRegistry] UpdatePid failed: %s\n", err);
        sqlite3_free(err);
    }
}

void InstanceRegistry::MarkReady(int64_t instance_id, int max_players) {
    std::lock_guard<std::mutex> lock(mutex_);
    sqlite3* db = Database::GetConnection();
    if (!db) return;

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db,
        "UPDATE ga_instances SET state='READY', max_players=?, last_empty_at=strftime('%s','now') "
        "WHERE instance_id=? AND state='STARTING'",
        -1, &stmt, nullptr);
    if (rc != SQLITE_OK || !stmt) {
        Logger::Log("db", "[InstanceRegistry] MarkReady prepare failed: %s\n",
            sqlite3_errmsg(db));
        return;
    }

    sqlite3_bind_int(stmt,  1, max_players);
    sqlite3_bind_int64(stmt, 2, instance_id);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        Logger::Log("db", "[InstanceRegistry] MarkReady step failed: %s\n", sqlite3_errmsg(db));
    } else {
        Logger::Log("db", "[InstanceRegistry] MarkReady: instance_id=%lld max_players=%d\n",
            (long long)instance_id, max_players);
    }
}

std::optional<InstanceInfo> InstanceRegistry::GetReadyHomeInstance() {
    std::lock_guard<std::mutex> lock(mutex_);
    sqlite3* db = Database::GetConnection();
    if (!db) return std::nullopt;

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db,
        "SELECT id, map_name, state, pid, udp_port, ip_address, player_count, "
        "       started_at, COALESCE(sealed_at, 0), "
        "       COALESCE(instance_id, 0), COALESCE(is_home_map, 0), COALESCE(max_players, 0), "
        "       COALESCE(game_mode, '') "
        "FROM ga_instances "
        "WHERE is_home_map=1 AND state='READY' "
        "LIMIT 1",
        -1, &stmt, nullptr);
    if (rc != SQLITE_OK || !stmt) {
        Logger::Log("db", "[InstanceRegistry] GetReadyHomeInstance prepare failed: %s\n",
            sqlite3_errmsg(db));
        return std::nullopt;
    }

    std::optional<InstanceInfo> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        InstanceInfo info;
        info.id           = sqlite3_column_int64(stmt, 0);
        const char* mn    = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        info.map_name     = mn ? mn : "";
        const char* st    = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        info.state        = st ? st : "";
        info.pid          = sqlite3_column_int(stmt, 3);
        info.udp_port     = static_cast<uint16_t>(sqlite3_column_int(stmt, 4));
        const char* ip    = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        info.ip_address   = (ip && *ip) ? ip : "127.0.0.1";
        info.player_count = sqlite3_column_int(stmt, 6);
        info.started_at   = sqlite3_column_int64(stmt, 7);
        info.sealed_at    = sqlite3_column_int64(stmt, 8);
        info.instance_id  = sqlite3_column_int64(stmt, 9);
        info.is_home_map  = sqlite3_column_int(stmt, 10) != 0;
        info.max_players  = sqlite3_column_int(stmt, 11);
        const char* gm    = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 12));
        info.game_mode    = gm ? gm : "";
        result = std::move(info);
    }
    sqlite3_finalize(stmt);
    return result;
}

std::optional<InstanceInfo> InstanceRegistry::GetInstanceById(int64_t instance_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    sqlite3* db = Database::GetConnection();
    if (!db) return std::nullopt;

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db,
        "SELECT id, map_name, state, pid, udp_port, ip_address, player_count, "
        "       started_at, COALESCE(sealed_at, 0), "
        "       COALESCE(instance_id, 0), COALESCE(is_home_map, 0), COALESCE(max_players, 0), "
        "       COALESCE(game_mode, '') "
        "FROM ga_instances "
        "WHERE instance_id = ? "
        "LIMIT 1",
        -1, &stmt, nullptr);
    if (rc != SQLITE_OK || !stmt) {
        Logger::Log("db", "[InstanceRegistry] GetInstanceById prepare failed: %s\n",
            sqlite3_errmsg(db));
        return std::nullopt;
    }

    sqlite3_bind_int64(stmt, 1, instance_id);

    std::optional<InstanceInfo> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        InstanceInfo info;
        info.id           = sqlite3_column_int64(stmt, 0);
        const char* mn    = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        info.map_name     = mn ? mn : "";
        const char* st    = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        info.state        = st ? st : "";
        info.pid          = sqlite3_column_int(stmt, 3);
        info.udp_port     = static_cast<uint16_t>(sqlite3_column_int(stmt, 4));
        const char* ip    = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        info.ip_address   = (ip && *ip) ? ip : "127.0.0.1";
        info.player_count = sqlite3_column_int(stmt, 6);
        info.started_at   = sqlite3_column_int64(stmt, 7);
        info.sealed_at    = sqlite3_column_int64(stmt, 8);
        info.instance_id  = sqlite3_column_int64(stmt, 9);
        info.is_home_map  = sqlite3_column_int(stmt, 10) != 0;
        info.max_players  = sqlite3_column_int(stmt, 11);
        const char* gm    = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 12));
        info.game_mode    = gm ? gm : "";
        result = std::move(info);
    }
    sqlite3_finalize(stmt);
    return result;
}

void InstanceRegistry::MarkStopped(int64_t instance_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    sqlite3* db = Database::GetConnection();
    if (!db) return;

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db,
        "UPDATE ga_instances SET state='STOPPED', sealed_at=strftime('%s','now') "
        "WHERE instance_id=?",
        -1, &stmt, nullptr);
    if (rc != SQLITE_OK || !stmt) {
        Logger::Log("db", "[InstanceRegistry] MarkStopped prepare failed: %s\n",
            sqlite3_errmsg(db));
        return;
    }

    sqlite3_bind_int64(stmt, 1, instance_id);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        Logger::Log("db", "[InstanceRegistry] MarkStopped step failed: %s\n", sqlite3_errmsg(db));
    } else {
        Logger::Log("db", "[InstanceRegistry] MarkStopped: instance_id=%lld\n",
            (long long)instance_id);

        // Also mark all active players as left for this instance
        {
            char player_sql[256];
            snprintf(player_sql, sizeof(player_sql),
                "UPDATE ga_instance_players SET left_at = strftime('%%s','now') "
                "WHERE instance_id = %lld AND left_at IS NULL",
                (long long)instance_id);
            char* perr = nullptr;
            sqlite3_exec(db, player_sql, nullptr, nullptr, &perr);
            if (perr) { sqlite3_free(perr); }
        }
    }
}

std::optional<uint16_t> InstanceRegistry::AllocatePort(uint16_t lo, uint16_t hi) {
    std::lock_guard<std::mutex> lock(mutex_);
    sqlite3* db = Database::GetConnection();
    if (!db) return std::nullopt;

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db,
        "SELECT udp_port FROM ga_instances WHERE state != 'STOPPED'",
        -1, &stmt, nullptr);
    if (rc != SQLITE_OK || !stmt) {
        Logger::Log("db", "[InstanceRegistry] AllocatePort prepare failed: %s\n",
            sqlite3_errmsg(db));
        return std::nullopt;
    }

    std::set<uint16_t> in_use;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        in_use.insert(static_cast<uint16_t>(sqlite3_column_int(stmt, 0)));
    }
    sqlite3_finalize(stmt);

    for (uint16_t port = lo; port <= hi; ++port) {
        if (in_use.find(port) == in_use.end()) {
            Logger::Log("db", "[InstanceRegistry] AllocatePort: allocated port %d\n", (int)port);
            return port;
        }
    }

    Logger::Log("db", "[InstanceRegistry] AllocatePort: port pool exhausted (range %d-%d)\n",
        (int)lo, (int)hi);
    return std::nullopt;
}

void InstanceRegistry::ClearStaleInstances() {
    std::lock_guard<std::mutex> lock(mutex_);
    sqlite3* db = Database::GetConnection();
    if (!db) return;

    char* err = nullptr;
    int rc = sqlite3_exec(db,
        "UPDATE ga_instances SET state='STOPPED', sealed_at=strftime('%s','now') "
        "WHERE state != 'STOPPED'",
        nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        Logger::Log("db", "[InstanceRegistry] ClearStaleInstances failed: %s\n", err);
        sqlite3_free(err);
        return;
    }

    int changed = sqlite3_changes(db);
    Logger::Log("db", "[InstanceRegistry] ClearStaleInstances: cleared %d stale instance(s)\n", changed);
}

void InstanceRegistry::InsertInstancePlayer(int64_t instance_id, const std::string& session_guid,
                                            int64_t character_id, int task_force) {
    std::lock_guard<std::mutex> lock(mutex_);
    sqlite3* db = Database::GetConnection();
    if (!db) return;

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db,
        "INSERT OR REPLACE INTO ga_instance_players "
        "(instance_id, session_guid, character_id, task_force_number, joined_at, left_at) "
        "VALUES (?, ?, ?, ?, strftime('%s','now'), NULL)",
        -1, &stmt, nullptr);
    if (rc != SQLITE_OK || !stmt) {
        Logger::Log("db", "[InstanceRegistry] InsertInstancePlayer prepare failed: %s\n",
            sqlite3_errmsg(db));
        return;
    }

    sqlite3_bind_int64(stmt, 1, instance_id);
    sqlite3_bind_text(stmt, 2, session_guid.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 3, character_id);
    sqlite3_bind_int(stmt, 4, task_force);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        Logger::Log("db", "[InstanceRegistry] InsertInstancePlayer failed: %s\n",
            sqlite3_errmsg(db));
        return;
    }

    // Update player_count and clear last_empty_at
    char sql[256];
    snprintf(sql, sizeof(sql),
        "UPDATE ga_instances SET player_count = ("
        "  SELECT COUNT(*) FROM ga_instance_players WHERE instance_id = %lld AND left_at IS NULL"
        "), last_empty_at = NULL WHERE instance_id = %lld",
        (long long)instance_id, (long long)instance_id);
    char* err = nullptr;
    sqlite3_exec(db, sql, nullptr, nullptr, &err);
    if (err) { sqlite3_free(err); }

    Logger::Log("db", "[InstanceRegistry] InsertInstancePlayer: instance=%lld guid=%s tf=%d\n",
        (long long)instance_id, session_guid.c_str(), task_force);
}

void InstanceRegistry::MarkInstancePlayerLeft(int64_t instance_id, const std::string& session_guid) {
    std::lock_guard<std::mutex> lock(mutex_);
    sqlite3* db = Database::GetConnection();
    if (!db) return;

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db,
        "UPDATE ga_instance_players SET left_at = strftime('%s','now') "
        "WHERE instance_id = ? AND session_guid = ? AND left_at IS NULL",
        -1, &stmt, nullptr);
    if (rc != SQLITE_OK || !stmt) {
        Logger::Log("db", "[InstanceRegistry] MarkInstancePlayerLeft prepare failed: %s\n",
            sqlite3_errmsg(db));
        return;
    }

    sqlite3_bind_int64(stmt, 1, instance_id);
    sqlite3_bind_text(stmt, 2, session_guid.c_str(), -1, SQLITE_TRANSIENT);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    // Update player_count
    char sql[256];
    snprintf(sql, sizeof(sql),
        "UPDATE ga_instances SET player_count = ("
        "  SELECT COUNT(*) FROM ga_instance_players WHERE instance_id = %lld AND left_at IS NULL"
        ") WHERE instance_id = %lld",
        (long long)instance_id, (long long)instance_id);
    char* err = nullptr;
    sqlite3_exec(db, sql, nullptr, nullptr, &err);
    if (err) { sqlite3_free(err); }

    // Set last_empty_at if now empty
    snprintf(sql, sizeof(sql),
        "UPDATE ga_instances SET last_empty_at = strftime('%%s','now') "
        "WHERE instance_id = %lld AND player_count = 0 AND last_empty_at IS NULL",
        (long long)instance_id);
    sqlite3_exec(db, sql, nullptr, nullptr, &err);
    if (err) { sqlite3_free(err); }

    Logger::Log("db", "[InstanceRegistry] MarkInstancePlayerLeft: instance=%lld guid=%s\n",
        (long long)instance_id, session_guid.c_str());
}

void InstanceRegistry::MarkAllInstancePlayersLeft(int64_t instance_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    sqlite3* db = Database::GetConnection();
    if (!db) return;

    char sql[256];
    snprintf(sql, sizeof(sql),
        "UPDATE ga_instance_players SET left_at = strftime('%%s','now') "
        "WHERE instance_id = %lld AND left_at IS NULL",
        (long long)instance_id);
    char* err = nullptr;
    sqlite3_exec(db, sql, nullptr, nullptr, &err);
    if (err) { sqlite3_free(err); }

    snprintf(sql, sizeof(sql),
        "UPDATE ga_instances SET player_count = 0 WHERE instance_id = %lld",
        (long long)instance_id);
    sqlite3_exec(db, sql, nullptr, nullptr, &err);
    if (err) { sqlite3_free(err); }

    Logger::Log("db", "[InstanceRegistry] MarkAllInstancePlayersLeft: instance=%lld\n",
        (long long)instance_id);
}

std::pair<int, int> InstanceRegistry::GetTeamCounts(int64_t instance_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    sqlite3* db = Database::GetConnection();
    if (!db) return {0, 0};

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db,
        "SELECT task_force_number, COUNT(*) FROM ga_instance_players "
        "WHERE instance_id = ? AND left_at IS NULL "
        "GROUP BY task_force_number",
        -1, &stmt, nullptr);
    if (rc != SQLITE_OK || !stmt) return {0, 0};

    sqlite3_bind_int64(stmt, 1, instance_id);

    int team1 = 0, team2 = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int tf = sqlite3_column_int(stmt, 0);
        int cnt = sqlite3_column_int(stmt, 1);
        if (tf == 1) team1 = cnt;
        else if (tf == 2) team2 = cnt;
    }
    sqlite3_finalize(stmt);
    return {team1, team2};
}

int InstanceRegistry::GetActivePlayerCount(int64_t instance_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    sqlite3* db = Database::GetConnection();
    if (!db) return 0;

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db,
        "SELECT COUNT(*) FROM ga_instance_players "
        "WHERE instance_id = ? AND left_at IS NULL",
        -1, &stmt, nullptr);
    if (rc != SQLITE_OK || !stmt) return 0;

    sqlite3_bind_int64(stmt, 1, instance_id);
    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return count;
}

std::vector<InstanceInfo> InstanceRegistry::GetReadyMissionInstances() {
    std::lock_guard<std::mutex> lock(mutex_);
    sqlite3* db = Database::GetConnection();
    std::vector<InstanceInfo> results;
    if (!db) return results;

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db,
        "SELECT id, map_name, state, pid, udp_port, ip_address, player_count, "
        "       started_at, COALESCE(sealed_at, 0), "
        "       COALESCE(instance_id, 0), COALESCE(is_home_map, 0), COALESCE(max_players, 0), "
        "       COALESCE(game_mode, '') "
        "FROM ga_instances "
        "WHERE state = 'READY' AND is_home_map = 0",
        -1, &stmt, nullptr);
    if (rc != SQLITE_OK || !stmt) return results;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        InstanceInfo info;
        info.id           = sqlite3_column_int64(stmt, 0);
        const char* mn    = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        info.map_name     = mn ? mn : "";
        const char* st    = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        info.state        = st ? st : "";
        info.pid          = sqlite3_column_int(stmt, 3);
        info.udp_port     = static_cast<uint16_t>(sqlite3_column_int(stmt, 4));
        const char* ip    = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        info.ip_address   = (ip && *ip) ? ip : "127.0.0.1";
        info.player_count = sqlite3_column_int(stmt, 6);
        info.started_at   = sqlite3_column_int64(stmt, 7);
        info.sealed_at    = sqlite3_column_int64(stmt, 8);
        info.instance_id  = sqlite3_column_int64(stmt, 9);
        info.is_home_map  = sqlite3_column_int(stmt, 10) != 0;
        info.max_players  = sqlite3_column_int(stmt, 11);
        const char* gm    = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 12));
        info.game_mode    = gm ? gm : "";
        results.push_back(std::move(info));
    }
    sqlite3_finalize(stmt);
    return results;
}

std::optional<InstanceInfo> InstanceRegistry::GetHomeInstance() {
    std::lock_guard<std::mutex> lock(mutex_);
    sqlite3* db = Database::GetConnection();
    if (!db) return std::nullopt;

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db,
        "SELECT id, map_name, state, pid, udp_port, ip_address, player_count, "
        "       started_at, COALESCE(sealed_at, 0), "
        "       COALESCE(instance_id, 0), COALESCE(is_home_map, 0), COALESCE(max_players, 0), "
        "       COALESCE(game_mode, '') "
        "FROM ga_instances "
        "WHERE is_home_map = 1 AND state != 'STOPPED' "
        "LIMIT 1",
        -1, &stmt, nullptr);
    if (rc != SQLITE_OK || !stmt) return std::nullopt;

    std::optional<InstanceInfo> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        InstanceInfo info;
        info.id           = sqlite3_column_int64(stmt, 0);
        const char* mn    = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        info.map_name     = mn ? mn : "";
        const char* st    = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        info.state        = st ? st : "";
        info.pid          = sqlite3_column_int(stmt, 3);
        info.udp_port     = static_cast<uint16_t>(sqlite3_column_int(stmt, 4));
        const char* ip    = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        info.ip_address   = (ip && *ip) ? ip : "127.0.0.1";
        info.player_count = sqlite3_column_int(stmt, 6);
        info.started_at   = sqlite3_column_int64(stmt, 7);
        info.sealed_at    = sqlite3_column_int64(stmt, 8);
        info.instance_id  = sqlite3_column_int64(stmt, 9);
        info.is_home_map  = sqlite3_column_int(stmt, 10) != 0;
        info.max_players  = sqlite3_column_int(stmt, 11);
        const char* gm    = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 12));
        info.game_mode    = gm ? gm : "";
        result = std::move(info);
    }
    sqlite3_finalize(stmt);
    return result;
}

std::vector<InstanceInfo> InstanceRegistry::GetIdleInstances(int timeout_seconds) {
    std::lock_guard<std::mutex> lock(mutex_);
    sqlite3* db = Database::GetConnection();
    std::vector<InstanceInfo> results;
    if (!db) return results;

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db,
        "SELECT id, map_name, state, pid, udp_port, ip_address, player_count, "
        "       started_at, COALESCE(sealed_at, 0), "
        "       COALESCE(instance_id, 0), COALESCE(is_home_map, 0), COALESCE(max_players, 0), "
        "       COALESCE(game_mode, '') "
        "FROM ga_instances "
        "WHERE state = 'READY' AND player_count = 0 "
        "  AND last_empty_at IS NOT NULL "
        "  AND (strftime('%s','now') - last_empty_at) >= ?",
        -1, &stmt, nullptr);
    if (rc != SQLITE_OK || !stmt) return results;

    sqlite3_bind_int(stmt, 1, timeout_seconds);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        InstanceInfo info;
        info.id           = sqlite3_column_int64(stmt, 0);
        const char* mn    = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        info.map_name     = mn ? mn : "";
        const char* st    = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        info.state        = st ? st : "";
        info.pid          = sqlite3_column_int(stmt, 3);
        info.udp_port     = static_cast<uint16_t>(sqlite3_column_int(stmt, 4));
        const char* ip    = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        info.ip_address   = (ip && *ip) ? ip : "127.0.0.1";
        info.player_count = sqlite3_column_int(stmt, 6);
        info.started_at   = sqlite3_column_int64(stmt, 7);
        info.sealed_at    = sqlite3_column_int64(stmt, 8);
        info.instance_id  = sqlite3_column_int64(stmt, 9);
        info.is_home_map  = sqlite3_column_int(stmt, 10) != 0;
        info.max_players  = sqlite3_column_int(stmt, 11);
        const char* gm    = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 12));
        info.game_mode    = gm ? gm : "";
        results.push_back(std::move(info));
    }
    sqlite3_finalize(stmt);
    return results;
}

void InstanceRegistry::SetLastEmptyAtIfEmpty(int64_t instance_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    sqlite3* db = Database::GetConnection();
    if (!db) return;

    char sql[256];
    snprintf(sql, sizeof(sql),
        "UPDATE ga_instances SET last_empty_at = strftime('%%s','now') "
        "WHERE instance_id = %lld AND player_count = 0 AND last_empty_at IS NULL",
        (long long)instance_id);
    char* err = nullptr;
    sqlite3_exec(db, sql, nullptr, nullptr, &err);
    if (err) { sqlite3_free(err); }
}
