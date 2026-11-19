#include "src/ControlServer/MasterLease/MasterLease.hpp"
#include "src/ControlServer/Database/Database.hpp"
#include "src/ControlServer/Logger.hpp"

#include "sqlite3.h"

#include <cstring>
#include <ctime>

#ifndef _WIN32
#include <unistd.h>
#endif

namespace {

// Created on demand rather than in the schema migration: only a master ever
// touches it, and the node shares the same database in the single-machine setup.
bool EnsureTable(sqlite3* db) {
    char* err = nullptr;
    const int rc = sqlite3_exec(db,
        "CREATE TABLE IF NOT EXISTS cs_master_lease ("
        "  id        INTEGER PRIMARY KEY CHECK (id = 1),"
        "  holder    TEXT    NOT NULL,"
        "  heartbeat INTEGER NOT NULL"
        ");",
        nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        Logger::Log("lease", "[MasterLease] cannot create cs_master_lease: %s\n",
            err ? err : "?");
        sqlite3_free(err);
        return false;
    }
    return true;
}

bool WriteHolder(sqlite3* db, const std::string& holder, std::int64_t now) {
    sqlite3_stmt* stmt = nullptr;
    // INSERT OR REPLACE rather than UPSERT: works on any bundled SQLite version.
    int rc = sqlite3_prepare_v2(db,
        "INSERT OR REPLACE INTO cs_master_lease (id, holder, heartbeat) VALUES (1, ?, ?)",
        -1, &stmt, nullptr);
    if (rc != SQLITE_OK || !stmt) return false;
    sqlite3_bind_text(stmt, 1, holder.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, now);
    const int step = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return step == SQLITE_DONE;
}

}  // namespace

std::string MasterLease::Holder() {
    std::string host = "unknown";
#ifndef _WIN32
    char buf[256];
    if (gethostname(buf, sizeof(buf) - 1) == 0) {
        buf[sizeof(buf) - 1] = '\0';
        host = buf;
    }
#endif
#ifndef _WIN32
    const long pid = static_cast<long>(getpid());
#else
    const long pid = 0;
#endif
    return host + ":" + std::to_string(pid);
}

bool MasterLease::Acquire() {
    sqlite3* db = Database::GetConnection();
    if (!db) {
        Logger::Log("lease", "[MasterLease] no database connection\n");
        return false;
    }
    if (!EnsureTable(db)) return false;

    const std::int64_t now = static_cast<std::int64_t>(std::time(nullptr));
    const std::string me = Holder();

    std::string holder;
    std::int64_t heartbeat = 0;
    bool present = false;
    {
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, "SELECT holder, heartbeat FROM cs_master_lease WHERE id = 1",
                               -1, &stmt, nullptr) == SQLITE_OK && stmt) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                present = true;
                const char* h = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                holder = h ? h : "";
                heartbeat = sqlite3_column_int64(stmt, 1);
            }
        }
        sqlite3_finalize(stmt);
    }

    if (!present) {
        if (!WriteHolder(db, me, now)) {
            Logger::Log("lease", "[MasterLease] could not write the lease\n");
            return false;
        }
        Logger::Log("lease", "[MasterLease] acquired (no previous holder)\n");
        return true;
    }

    if (holder == me) {
        WriteHolder(db, me, now);
        Logger::Log("lease", "[MasterLease] re-acquired (same process)\n");
        return true;
    }

    const std::int64_t age = now - heartbeat;
    if (age > kStaleSeconds) {
        WriteHolder(db, me, now);
        Logger::Log("lease",
            "[MasterLease] took over from '%s' (no heartbeat for %llds, stale after %ds)\n",
            holder.c_str(), (long long)age, kStaleSeconds);
        return true;
    }

    Logger::Log("lease",
        "[MasterLease] REFUSING to start: database is leased by '%s' (heartbeat %llds ago). "
        "Stop that master first, or wait %ds for the lease to go stale.\n",
        holder.c_str(), (long long)age, kStaleSeconds - static_cast<int>(age));
    return false;
}

void MasterLease::Heartbeat() {
    sqlite3* db = Database::GetConnection();
    if (!db) return;

    const std::int64_t now = static_cast<std::int64_t>(std::time(nullptr));
    const std::string me = Holder();

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db,
            "UPDATE cs_master_lease SET heartbeat = ? WHERE id = 1 AND holder = ?",
            -1, &stmt, nullptr) != SQLITE_OK || !stmt) {
        return;
    }
    sqlite3_bind_int64(stmt, 1, now);
    sqlite3_bind_text(stmt, 2, me.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (sqlite3_changes(db) == 0) {
        // Someone else owns the database now. Not fatal by itself -- this process
        // keeps serving until told otherwise -- but it must never be silent.
        Logger::Log("lease",
            "[MasterLease] LOST the lease: another master has taken the database\n");
    }
}

void MasterLease::Release() {
    sqlite3* db = Database::GetConnection();
    if (!db) return;

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db,
            "DELETE FROM cs_master_lease WHERE id = 1 AND holder = ?",
            -1, &stmt, nullptr) != SQLITE_OK || !stmt) {
        return;
    }
    sqlite3_bind_text(stmt, 1, Holder().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    Logger::Log("lease", "[MasterLease] released\n");
}
