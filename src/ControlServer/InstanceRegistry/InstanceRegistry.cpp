#include "src/ControlServer/InstanceRegistry/InstanceRegistry.hpp"
#include "src/ControlServer/Database/Database.hpp"
#include "src/ControlServer/Logger.hpp"
#include "sqlite3.h"

std::mutex InstanceRegistry::mutex_;

void InstanceRegistry::Init() {
    // No-op: mutex_ is default-constructed.
    // Kept for startup symmetry with PlayerSessionStore::Init().
}

std::optional<InstanceInfo> InstanceRegistry::GetReadyInstance(const std::string& map_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    sqlite3* db = Database::GetConnection();
    if (!db) return std::nullopt;

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db,
        "SELECT id, map_name, state, pid, udp_port, ip_address, player_count, "
        "       started_at, COALESCE(sealed_at, 0) "
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
        "VALUES (?, 'READY', 0, ?, '127.0.0.1', 0, strftime('%s','now'))",
        -1, &stmt, nullptr);
    if (rc != SQLITE_OK || !stmt) {
        Logger::Log("db", "[InstanceRegistry] SeedHomeMapInstance prepare failed: %s\n",
            sqlite3_errmsg(db));
        return;
    }

    sqlite3_bind_text(stmt, 1, map_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt,  2, static_cast<int>(udp_port));

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
