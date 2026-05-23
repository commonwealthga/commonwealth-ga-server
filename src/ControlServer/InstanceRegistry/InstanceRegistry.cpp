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
        "       COALESCE(game_mode, ''), "
        "       COALESCE(queue_id, 0), COALESCE(predecessor_instance_id, 0), COALESCE(end_mission_at, 0) "
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
        info.queue_id                = static_cast<uint32_t>(sqlite3_column_int64(stmt, 13));
        info.predecessor_instance_id = sqlite3_column_int64(stmt, 14);
        info.end_mission_at          = sqlite3_column_int64(stmt, 15);
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
                                          uint16_t udp_port, int pid, bool is_home_map,
                                          uint32_t queue_id, int64_t predecessor_instance_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    sqlite3* db = Database::GetConnection();
    if (!db) return 0;

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db,
        "INSERT INTO ga_instances "
        "(map_name, game_mode, state, pid, udp_port, ip_address, player_count, started_at, instance_id, is_home_map, queue_id, predecessor_instance_id) "
        "VALUES (?, ?, 'STARTING', ?, ?, ?, 0, strftime('%s','now'), 0, ?, ?, ?)",
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
    if (queue_id != 0) sqlite3_bind_int64(stmt, 7, static_cast<int64_t>(queue_id));
    else               sqlite3_bind_null(stmt,  7);
    if (predecessor_instance_id != 0) sqlite3_bind_int64(stmt, 8, predecessor_instance_id);
    else                              sqlite3_bind_null(stmt,  8);

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

    Logger::Log("db", "[InstanceRegistry] InsertStarting: map=%s udp_port=%d pid=%d is_home_map=%d queue_id=%u predecessor=%lld -> instance_id=%lld\n",
        map_name.c_str(), (int)udp_port, pid, (int)is_home_map,
        queue_id, (long long)predecessor_instance_id, (long long)rowid);
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

    // DRAFTING-aware transition: if this row is a successor whose parent
    // hasn't yet called MarkMissionEnded, hold it in DRAFTING instead of
    // READY so the matchmaker (which filters state='READY') won't pull
    // players into it. MarkMissionEnded(parent) will promote it later.
    // The CASE keeps everything in a single statement so we don't need a
    // separate read-modify-write cycle.
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db,
        "UPDATE ga_instances "
        "SET state = CASE "
        "      WHEN predecessor_instance_id IS NOT NULL "
        "       AND EXISTS ( "
        "             SELECT 1 FROM ga_instances p "
        "             WHERE p.instance_id = ga_instances.predecessor_instance_id "
        "               AND p.end_mission_at IS NULL "
        "               AND p.state != 'STOPPED' "
        "           ) "
        "      THEN 'DRAFTING' ELSE 'READY' END, "
        "    max_players=?, last_empty_at=strftime('%s','now') "
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
        // Read back what state we landed in for the log line.
        const char* finalState = "?";
        sqlite3_stmt* rs = nullptr;
        if (sqlite3_prepare_v2(db,
                "SELECT state FROM ga_instances WHERE instance_id=?",
                -1, &rs, nullptr) == SQLITE_OK && rs) {
            sqlite3_bind_int64(rs, 1, instance_id);
            if (sqlite3_step(rs) == SQLITE_ROW) {
                const char* s = reinterpret_cast<const char*>(sqlite3_column_text(rs, 0));
                if (s) finalState = s;
            }
        }
        Logger::Log("db", "[InstanceRegistry] MarkReady: instance_id=%lld max_players=%d → state=%s\n",
            (long long)instance_id, max_players, finalState);
        if (rs) sqlite3_finalize(rs);
    }
}

int64_t InstanceRegistry::MarkMissionEnded(int64_t parent_instance_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    sqlite3* db = Database::GetConnection();
    if (!db) return 0;

    // Stamp end_mission_at on the parent (idempotent — only writes when NULL).
    {
        sqlite3_stmt* stmt = nullptr;
        int rc = sqlite3_prepare_v2(db,
            "UPDATE ga_instances "
            "SET end_mission_at = strftime('%s','now') "
            "WHERE instance_id = ? AND end_mission_at IS NULL",
            -1, &stmt, nullptr);
        if (rc == SQLITE_OK && stmt) {
            sqlite3_bind_int64(stmt, 1, parent_instance_id);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
    }

    // Promote any DRAFTING successor (matchmaker can now use it). The
    // matching WHERE clauses below allow the same code path to safely
    // handle the case where the successor doesn't exist or already
    // got promoted earlier by PromoteSuccessor (orphan-fallback).
    int64_t successor_id = 0;
    {
        sqlite3_stmt* stmt = nullptr;
        int rc = sqlite3_prepare_v2(db,
            "SELECT instance_id FROM ga_instances "
            "WHERE predecessor_instance_id = ? AND state = 'DRAFTING' LIMIT 1",
            -1, &stmt, nullptr);
        if (rc == SQLITE_OK && stmt) {
            sqlite3_bind_int64(stmt, 1, parent_instance_id);
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                successor_id = sqlite3_column_int64(stmt, 0);
            }
            sqlite3_finalize(stmt);
        }
    }
    if (successor_id != 0) {
        sqlite3_stmt* stmt = nullptr;
        int rc = sqlite3_prepare_v2(db,
            "UPDATE ga_instances SET state='READY', "
            "  last_empty_at=strftime('%s','now') "
            "WHERE instance_id=? AND state='DRAFTING'",
            -1, &stmt, nullptr);
        if (rc == SQLITE_OK && stmt) {
            sqlite3_bind_int64(stmt, 1, successor_id);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
        Logger::Log("db", "[InstanceRegistry] MarkMissionEnded parent=%lld → promoted successor=%lld DRAFTING→READY\n",
            (long long)parent_instance_id, (long long)successor_id);
    } else {
        Logger::Log("db", "[InstanceRegistry] MarkMissionEnded parent=%lld (no DRAFTING successor)\n",
            (long long)parent_instance_id);
    }
    return successor_id;
}

int64_t InstanceRegistry::PromoteSuccessor(int64_t parent_instance_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    sqlite3* db = Database::GetConnection();
    if (!db) return 0;

    int64_t successor_id = 0;
    sqlite3_stmt* find = nullptr;
    if (sqlite3_prepare_v2(db,
            "SELECT instance_id FROM ga_instances "
            "WHERE predecessor_instance_id = ? AND state = 'DRAFTING' LIMIT 1",
            -1, &find, nullptr) == SQLITE_OK && find) {
        sqlite3_bind_int64(find, 1, parent_instance_id);
        if (sqlite3_step(find) == SQLITE_ROW) {
            successor_id = sqlite3_column_int64(find, 0);
        }
        sqlite3_finalize(find);
    }
    if (successor_id == 0) return 0;

    sqlite3_stmt* upd = nullptr;
    if (sqlite3_prepare_v2(db,
            "UPDATE ga_instances SET state='READY', "
            "  last_empty_at=strftime('%s','now') "
            "WHERE instance_id=? AND state='DRAFTING'",
            -1, &upd, nullptr) == SQLITE_OK && upd) {
        sqlite3_bind_int64(upd, 1, successor_id);
        sqlite3_step(upd);
        sqlite3_finalize(upd);
    }
    Logger::Log("db", "[InstanceRegistry] PromoteSuccessor parent=%lld → successor=%lld DRAFTING→READY (orphan fallback)\n",
        (long long)parent_instance_id, (long long)successor_id);
    return successor_id;
}

std::optional<InstanceInfo> InstanceRegistry::GetSuccessor(int64_t parent_instance_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    sqlite3* db = Database::GetConnection();
    if (!db) return std::nullopt;

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db,
        "SELECT id, map_name, state, pid, udp_port, ip_address, player_count, "
        "       started_at, COALESCE(sealed_at, 0), "
        "       COALESCE(instance_id, 0), COALESCE(is_home_map, 0), COALESCE(max_players, 0), "
        "       COALESCE(game_mode, ''), "
        "       COALESCE(queue_id, 0), COALESCE(predecessor_instance_id, 0), COALESCE(end_mission_at, 0) "
        "FROM ga_instances "
        "WHERE predecessor_instance_id = ? AND state != 'STOPPED' "
        "LIMIT 1",
        -1, &stmt, nullptr);
    if (rc != SQLITE_OK || !stmt) return std::nullopt;

    sqlite3_bind_int64(stmt, 1, parent_instance_id);

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
        info.queue_id                = static_cast<uint32_t>(sqlite3_column_int64(stmt, 13));
        info.predecessor_instance_id = sqlite3_column_int64(stmt, 14);
        info.end_mission_at          = sqlite3_column_int64(stmt, 15);
        result = std::move(info);
    }
    sqlite3_finalize(stmt);
    return result;
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
        "       COALESCE(game_mode, ''), "
        "       COALESCE(queue_id, 0), COALESCE(predecessor_instance_id, 0), COALESCE(end_mission_at, 0) "
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
        info.queue_id                = static_cast<uint32_t>(sqlite3_column_int64(stmt, 13));
        info.predecessor_instance_id = sqlite3_column_int64(stmt, 14);
        info.end_mission_at          = sqlite3_column_int64(stmt, 15);
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
        "       COALESCE(game_mode, ''), "
        "       COALESCE(queue_id, 0), COALESCE(predecessor_instance_id, 0), COALESCE(end_mission_at, 0) "
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
        info.queue_id                = static_cast<uint32_t>(sqlite3_column_int64(stmt, 13));
        info.predecessor_instance_id = sqlite3_column_int64(stmt, 14);
        info.end_mission_at          = sqlite3_column_int64(stmt, 15);
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
                                            int64_t character_id, int task_force,
                                            uint32_t profile_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    sqlite3* db = Database::GetConnection();
    if (!db) return;

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db,
        "INSERT OR REPLACE INTO ga_instance_players "
        "(instance_id, session_guid, character_id, task_force_number, profile_id, joined_at, left_at) "
        "VALUES (?, ?, ?, ?, ?, strftime('%s','now'), NULL)",
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
    if (profile_id == 0) {
        sqlite3_bind_null(stmt, 5);
    } else {
        sqlite3_bind_int(stmt, 5, (int)profile_id);
    }

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

    Logger::Log("db", "[InstanceRegistry] InsertInstancePlayer: instance=%lld guid=%s tf=%d profile=%u\n",
        (long long)instance_id, session_guid.c_str(), task_force, profile_id);
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
    // `end_mission_at IS NULL` excludes instances whose mission has already
    // wrapped. After MSG_MISSION_ENDED the parent row is still state='READY'
    // (deliberately, so stragglers clicking "End Mission" can still consult
    // it for the home-vs-successor routing decision in TcpSession's
    // GSC_CHANGE_INSTANCE handler), but its mission is over and we must NOT
    // route fresh queue-pop players into it. For continue_in_queue=true
    // queues, MarkMissionEnded already promoted the DRAFTING successor to
    // READY in the same transaction — that row has end_mission_at=NULL so
    // it appears here as the queue's only candidate, exactly what we want.
    // For continue_in_queue=false queues there's no successor at all, so
    // the matchmaker sees no instance for this queue and falls through to
    // the "spawn new" branch — which is the requested behavior.
    int rc = sqlite3_prepare_v2(db,
        "SELECT id, map_name, state, pid, udp_port, ip_address, player_count, "
        "       started_at, COALESCE(sealed_at, 0), "
        "       COALESCE(instance_id, 0), COALESCE(is_home_map, 0), COALESCE(max_players, 0), "
        "       COALESCE(game_mode, ''), "
        "       COALESCE(queue_id, 0), COALESCE(predecessor_instance_id, 0), COALESCE(end_mission_at, 0) "
        "FROM ga_instances "
        "WHERE state = 'READY' AND is_home_map = 0 AND end_mission_at IS NULL",
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
        info.queue_id                = static_cast<uint32_t>(sqlite3_column_int64(stmt, 13));
        info.predecessor_instance_id = sqlite3_column_int64(stmt, 14);
        info.end_mission_at          = sqlite3_column_int64(stmt, 15);
        results.push_back(std::move(info));
    }
    sqlite3_finalize(stmt);
    return results;
}

std::vector<InstanceInfo> InstanceRegistry::GetAllRunningInstances() {
    std::lock_guard<std::mutex> lock(mutex_);
    sqlite3* db = Database::GetConnection();
    std::vector<InstanceInfo> results;
    if (!db) return results;

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db,
        "SELECT id, map_name, state, pid, udp_port, ip_address, player_count, "
        "       started_at, COALESCE(sealed_at, 0), "
        "       COALESCE(instance_id, 0), COALESCE(is_home_map, 0), COALESCE(max_players, 0), "
        "       COALESCE(game_mode, ''), "
        "       COALESCE(queue_id, 0), COALESCE(predecessor_instance_id, 0), COALESCE(end_mission_at, 0) "
        "FROM ga_instances "
        "WHERE state != 'STOPPED' AND pid IS NOT NULL AND pid > 0",
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
        info.queue_id                = static_cast<uint32_t>(sqlite3_column_int64(stmt, 13));
        info.predecessor_instance_id = sqlite3_column_int64(stmt, 14);
        info.end_mission_at          = sqlite3_column_int64(stmt, 15);
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
        "       COALESCE(game_mode, ''), "
        "       COALESCE(queue_id, 0), COALESCE(predecessor_instance_id, 0), COALESCE(end_mission_at, 0) "
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
        info.queue_id                = static_cast<uint32_t>(sqlite3_column_int64(stmt, 13));
        info.predecessor_instance_id = sqlite3_column_int64(stmt, 14);
        info.end_mission_at          = sqlite3_column_int64(stmt, 15);
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

    // Home-map pin: while any mission instance (non-home) is alive in
    // STARTING/READY, do NOT report the home map as idle even if its
    // last_empty_at is past the timeout. Active servers should always have
    // a warm home so returning from a mission is instantaneous; the 5-min
    // idle rule only applies once the server is truly quiet (no missions).
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db,
        "SELECT id, map_name, state, pid, udp_port, ip_address, player_count, "
        "       started_at, COALESCE(sealed_at, 0), "
        "       COALESCE(instance_id, 0), COALESCE(is_home_map, 0), COALESCE(max_players, 0), "
        "       COALESCE(game_mode, ''), "
        "       COALESCE(queue_id, 0), COALESCE(predecessor_instance_id, 0), COALESCE(end_mission_at, 0) "
        "FROM ga_instances "
        "WHERE state = 'READY' AND player_count = 0 "
        "  AND last_empty_at IS NOT NULL "
        "  AND (strftime('%s','now') - last_empty_at) >= ? "
        "  AND (is_home_map = 0 OR NOT EXISTS ("
        "         SELECT 1 FROM ga_instances mi "
        "         WHERE mi.is_home_map = 0 "
        "           AND mi.state IN ('STARTING','READY')"
        "       ))",
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
        info.queue_id                = static_cast<uint32_t>(sqlite3_column_int64(stmt, 13));
        info.predecessor_instance_id = sqlite3_column_int64(stmt, 14);
        info.end_mission_at          = sqlite3_column_int64(stmt, 15);
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

// ---------------------------------------------------------------------------
// Queue-scoped live counters (RuntimeStats consumers)
// ---------------------------------------------------------------------------

int InstanceRegistry::GetActiveInstanceCountForQueue(uint32_t queue_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    sqlite3* db = Database::GetConnection();
    if (!db) return 0;

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db,
            "SELECT COUNT(*) FROM ga_instances "
            "WHERE queue_id = ? AND state != 'STOPPED'",
            -1, &stmt, nullptr) != SQLITE_OK || !stmt) {
        return 0;
    }
    sqlite3_bind_int(stmt, 1, (int)queue_id);
    int n = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) n = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    return n;
}

int InstanceRegistry::GetActivePlayerCountForQueue(uint32_t queue_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    sqlite3* db = Database::GetConnection();
    if (!db) return 0;

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db,
            "SELECT COUNT(*) FROM ga_instance_players ip "
            "JOIN ga_instances i ON i.instance_id = ip.instance_id "
            "WHERE i.queue_id = ? AND i.state != 'STOPPED' AND ip.left_at IS NULL",
            -1, &stmt, nullptr) != SQLITE_OK || !stmt) {
        return 0;
    }
    sqlite3_bind_int(stmt, 1, (int)queue_id);
    int n = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) n = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    return n;
}

InstanceRegistry::ActiveProfileCounts
InstanceRegistry::GetActiveProfileCountsForQueue(uint32_t queue_id) {
    ActiveProfileCounts out{0, 0, 0, 0};

    std::lock_guard<std::mutex> lock(mutex_);
    sqlite3* db = Database::GetConnection();
    if (!db) return out;

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db,
            "SELECT ip.profile_id, COUNT(*) FROM ga_instance_players ip "
            "JOIN ga_instances i ON i.instance_id = ip.instance_id "
            "WHERE i.queue_id = ? AND i.state != 'STOPPED' AND ip.left_at IS NULL "
            "  AND ip.profile_id IS NOT NULL "
            "GROUP BY ip.profile_id",
            -1, &stmt, nullptr) != SQLITE_OK || !stmt) {
        return out;
    }
    sqlite3_bind_int(stmt, 1, (int)queue_id);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const uint32_t pid = (uint32_t)sqlite3_column_int(stmt, 0);
        const uint32_t cnt = (uint32_t)sqlite3_column_int(stmt, 1);
        switch (pid) {
            case 680: out.assault  = cnt; break;  // PROFILE_ASSAULT  = 0x2A8
            case 567: out.medic    = cnt; break;  // PROFILE_MEDIC    = 0x237
            case 681: out.recon    = cnt; break;  // PROFILE_RECON    = 0x2A9
            case 679: out.robotics = cnt; break;  // PROFILE_ROBOTICS = 0x2A7
            default: break;
        }
    }
    sqlite3_finalize(stmt);
    return out;
}
