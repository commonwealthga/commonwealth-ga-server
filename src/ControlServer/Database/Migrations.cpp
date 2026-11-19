#include "src/ControlServer/Database/Migrations.hpp"

#include "src/ControlServer/Database/EmbeddedMigrations.hpp"
#include "src/Shared/SchemaVersion.hpp"
#include "src/ControlServer/Logger.hpp"   // NOT src/Utils/Logger: same class name, different TU

#include "sqlite3.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <cstring>
#include <vector>

namespace {

constexpr const char* kTag = "db";

bool Exec(sqlite3* db, const std::string& sql, std::string& err) {
    char* msg = nullptr;
    if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &msg) != SQLITE_OK) {
        err = msg ? msg : "unknown error";
        sqlite3_free(msg);
        return false;
    }
    return true;
}

bool HasAnyTable(sqlite3* db) {
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db,
            "SELECT count(*) FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%'",
            -1, &stmt, nullptr) != SQLITE_OK || !stmt) {
        return false;
    }
    bool any = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) any = sqlite3_column_int(stmt, 0) > 0;
    sqlite3_finalize(stmt);
    return any;
}

// Runs a migration file statement by statement.
//
// Statement-at-a-time rather than one sqlite3_exec of the whole file, for two
// reasons: the error message can name what failed, and one tolerated error does
// not skip everything after it in the file (a failed ALTER would otherwise also
// skip the CREATE INDEX written after it).
bool ExecScript(sqlite3* db, const std::string& sql, std::string& err) {
    auto duplicate_column = [](const std::string& m) {
        return m.find("duplicate column name") != std::string::npos;
    };

    const char* tail = sql.c_str();
    while (tail && *tail) {
        sqlite3_stmt* stmt = nullptr;
        const char* next = nullptr;
        const int prc = sqlite3_prepare_v2(db, tail, -1, &stmt, &next);
        if (prc != SQLITE_OK) {
            const std::string msg = sqlite3_errmsg(db);
            // ALTER TABLE ... ADD COLUMN for a column that is already there fails
            // during PREPARE, and pzTail is only set on success -- so the
            // remainder has to be found by hand. Safe because the only statement
            // that can raise this is an ADD COLUMN, which carries no literal
            // semicolon.
            if (duplicate_column(msg)) {
                Logger::Log(kTag, "[Migrations] already applied (%s)\n", msg.c_str());
                const char* semi = std::strchr(tail, ';');
                if (!semi) return true;          // nothing after it
                tail = semi + 1;
                continue;
            }
            err = msg;
            return false;
        }
        if (!stmt) {   // whitespace or a comment only
            tail = next;
            continue;
        }

        const int rc = sqlite3_step(stmt);
        std::string msg;
        if (rc != SQLITE_DONE && rc != SQLITE_ROW) msg = sqlite3_errmsg(db);
        sqlite3_finalize(stmt);

        if (!msg.empty()) {
            // Same tolerance on the step path (some ALTERs report here).
            if (duplicate_column(msg)) {
                Logger::Log(kTag, "[Migrations] already applied (%s)\n", msg.c_str());
            } else {
                err = msg;
                return false;
            }
        }
        tail = next;
    }
    return true;
}

// Copies the baseline over the open database with SQLite's own backup API: one
// page-aware bulk copy, no per-row statements, no journal churn.
bool InstallBase(sqlite3* db, const std::string& base_path, std::string& err) {
    if (!std::filesystem::exists(base_path)) {
        err = "baseline database not found: " + base_path +
              " (generate it with scripts/make_dbbase.py)";
        return false;
    }

    sqlite3* src = nullptr;
    if (sqlite3_open_v2(base_path.c_str(), &src, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK) {
        err = "cannot open baseline " + base_path + ": " + sqlite3_errmsg(db);
        sqlite3_close(src);
        return false;
    }

    sqlite3_backup* bk = sqlite3_backup_init(db, "main", src, "main");
    if (!bk) {
        err = "cannot start baseline copy: " + std::string(sqlite3_errmsg(db));
        sqlite3_close(src);
        return false;
    }

    int rc = sqlite3_backup_step(bk, -1);   // one call, whole file
    if (rc != SQLITE_DONE) {
        err = rc == SQLITE_OK ? "baseline copy incomplete"
                              : std::string("baseline copy failed: ") + sqlite3_errstr(rc);
        sqlite3_backup_finish(bk);
        sqlite3_close(src);
        return false;
    }
    sqlite3_backup_finish(bk);
    sqlite3_close(src);
    return true;
}

}  // namespace

namespace Migrations {

Result Run(sqlite3* db, const std::string& db_path, const std::string& base_path) {
    Result r;
    if (!db) {
        r.error = "no database connection";
        return r;
    }

    std::string err;

    // 1. Baseline install. Only when the file has no schema at all: an existing
    //    database is never replaced, or a live server would lose its players.
    if (!HasAnyTable(db)) {
        if (!InstallBase(db, base_path, err)) {
            r.error = err;
            return r;
        }
        r.installed_base = true;
        Logger::Log(kTag, "[Migrations] installed baseline %s into %s\n",
            base_path.c_str(), db_path.c_str());
    }

    if (!Exec(db, "CREATE TABLE IF NOT EXISTS version_info (version INTEGER DEFAULT 0)", err)) {
        r.error = "cannot ensure version_info: " + err;
        return r;
    }

    // 2. Current version.
    int version = 0;
    {
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, "SELECT max(version) FROM version_info", -1, &stmt, nullptr)
                == SQLITE_OK && stmt) {
            if (sqlite3_step(stmt) == SQLITE_ROW) version = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
        if (version <= 0) {
            // A database with a schema but no version row cannot be reasoned
            // about; stamp it with the baseline's version rather than replaying.
            r.error = "database has a schema but no version_info row";
            return r;
        }
    }
    r.from_version = version;

    // 3. Pending migrations. Already in ascending version order -- the generator
    //    sorts and refuses duplicate version numbers.
    std::vector<const EmbeddedMigrations::Migration*> pending;
    for (int i = 0; i < EmbeddedMigrations::kCount; ++i) {
        const auto& m = EmbeddedMigrations::kMigrations[i];
        if (m.version > version) pending.push_back(&m);
    }

    for (const auto* m : pending) {
        if (!Exec(db, "BEGIN IMMEDIATE;", err)) {
            r.error = std::string("cannot begin ") + m->name + ": " + err;
            return r;
        }
        // One transaction per file: it lands completely or not at all.
        if (!ExecScript(db, m->sql, err)) {
            std::string ignored;
            Exec(db, "ROLLBACK;", ignored);
            r.error = std::string("migration ") + m->name + " failed: " + err;
            return r;
        }
        if (!Exec(db, "UPDATE version_info SET version = " + std::to_string(m->version) + ";",
                  err)) {
            std::string ignored;
            Exec(db, "ROLLBACK;", ignored);
            r.error = "cannot stamp version " + std::to_string(m->version) + ": " + err;
            return r;
        }
        if (!Exec(db, "COMMIT;", err)) {
            r.error = std::string("cannot commit ") + m->name + ": " + err;
            return r;
        }
        ++r.applied;
        version = m->version;
        Logger::Log(kTag, "[Migrations] applied %s -> version %d\n", m->name, m->version);
    }

    r.to_version = version;

    if (version < Schema::kVersion) {
        // No ladder replays these any more, so say what to actually do. A node's
        // database is derived data (content snapshot plus player bundles) and is
        // safe to delete; a master's is not, and needs scripts/make_dbbase.py
        // style manual attention.
        Logger::Log(kTag,
            "[Migrations] WARNING: %s is at schema %d, this build expects %d. Nothing "
            "will migrate it automatically. If this is a node or other derived "
            "database, delete the file and it will be reinstalled from the baseline.\n",
            db_path.c_str(), version, Schema::kVersion);
    } else if (r.applied == 0 && !r.installed_base) {
        Logger::Log(kTag, "[Migrations] schema up to date at %d\n", version);
    }

    r.ok = true;
    return r;
}

}  // namespace Migrations
