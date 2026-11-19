#include "src/ControlServer/NodeData/NodeDataStore.hpp"
#include "src/ControlServer/Database/Database.hpp"
#include "src/ControlServer/Logger.hpp"

#include "sqlite3.h"

#include <set>
#include <string>
#include <utility>
#include <vector>

namespace {

const char* kTag = "node";

bool Exec(sqlite3* db, const std::string& sql, std::string& err) {
    char* msg = nullptr;
    if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &msg) != SQLITE_OK) {
        err = msg ? msg : "unknown error";
        sqlite3_free(msg);
        return false;
    }
    return true;
}

std::string QuoteIdent(const std::string& name) {
    std::string out = "\"";
    for (char c : name) {
        if (c == '"') out.push_back('"');
        out.push_back(c);
    }
    out.push_back('"');
    return out;
}

std::string QuoteLiteral(const std::string& s) {
    std::string out = "'";
    for (char c : s) {
        if (c == '\'') out.push_back('\'');
        out.push_back(c);
    }
    out.push_back('\'');
    return out;
}

std::vector<std::pair<std::string, std::string>> QueryTwoColumns(sqlite3* db,
                                                                 const std::string& sql) {
    std::vector<std::pair<std::string, std::string>> rows;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK || !stmt) return rows;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* a = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        const char* b = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        rows.emplace_back(a ? a : "", b ? b : "");
    }
    sqlite3_finalize(stmt);
    return rows;
}

struct ColumnDef {
    std::string name;
    std::string type;
};

// PRAGMA table_info for a table in `schema` ("main" or "content").
std::vector<ColumnDef> ColumnsOf(sqlite3* db, const char* schema, const std::string& table) {
    std::vector<ColumnDef> cols;
    sqlite3_stmt* stmt = nullptr;
    const std::string sql = std::string("PRAGMA ") + schema + ".table_info(" +
                            QuoteIdent(table) + ")";
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK || !stmt) {
        return cols;
    }
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const char* type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        ColumnDef c;
        c.name = name ? name : "";
        c.type = type ? type : "";
        if (!c.name.empty()) cols.push_back(c);
    }
    sqlite3_finalize(stmt);
    return cols;
}

// "CREATE TABLE x (...)" -> "CREATE TABLE IF NOT EXISTS x (...)"
std::string WithIfNotExists(const std::string& sql) {
    static const std::string kPrefix = "CREATE TABLE ";
    if (sql.rfind(kPrefix, 0) == 0) {
        return kPrefix + "IF NOT EXISTS " + sql.substr(kPrefix.size());
    }
    return sql;
}

// Add any column the master's definition has that this database does not. Adds
// only -- a node may legitimately carry extra columns from its own DLL's
// migrations, and dropping data to force an exact match would be worse than a
// superset.
void ReconcileColumns(sqlite3* db, const std::string& table, std::string& err) {
    const auto want = ColumnsOf(db, "content", table);
    const auto have = ColumnsOf(db, "main", table);
    if (want.empty() || have.empty()) return;

    for (const auto& w : want) {
        bool present = false;
        for (const auto& h : have) {
            if (h.name == w.name) {
                present = true;
                break;
            }
        }
        if (present) continue;

        // SQLite cannot add a NOT NULL column without a default, so the constraint
        // is dropped here; the type is kept so affinity still matches the master.
        std::string sql = "ALTER TABLE main." + QuoteIdent(table) + " ADD COLUMN " +
                          QuoteIdent(w.name);
        if (!w.type.empty()) sql += " " + w.type;
        if (!Exec(db, sql + ";", err)) {
            Logger::Log(kTag, "[NodeData] %s: cannot add column %s: %s\n",
                table.c_str(), w.name.c_str(), err.c_str());
            continue;
        }
        Logger::Log(kTag, "[NodeData] %s: added missing column %s (%s)\n",
            table.c_str(), w.name.c_str(), w.type.empty() ? "untyped" : w.type.c_str());
    }
}

bool EnsureStateTable(sqlite3* db, std::string& err) {
    return Exec(db,
        "CREATE TABLE IF NOT EXISTS node_content_state ("
        "  id   INTEGER PRIMARY KEY CHECK (id = 1),"
        "  hash TEXT NOT NULL DEFAULT ''"
        ");",
        err);
}

}  // namespace

std::string NodeDataStore::AppliedContentHash() {
    sqlite3* db = Database::GetConnection();
    if (!db) return std::string();

    std::string err;
    if (!EnsureStateTable(db, err)) return std::string();

    sqlite3_stmt* stmt = nullptr;
    std::string hash;
    if (sqlite3_prepare_v2(db, "SELECT hash FROM node_content_state WHERE id = 1",
                           -1, &stmt, nullptr) == SQLITE_OK && stmt) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* h = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            hash = h ? h : "";
        }
    }
    sqlite3_finalize(stmt);
    return hash;
}

bool NodeDataStore::SyncContent(const std::string& content_db_path,
                                const std::string& content_hash,
                                std::string& err) {
    sqlite3* db = Database::GetConnection();
    if (!db) {
        err = "no database connection";
        return false;
    }
    if (!EnsureStateTable(db, err)) return false;

    if (AppliedContentHash() == content_hash && !content_hash.empty()) {
        return true;   // already there
    }

    // The snapshot is opened read-write only because ATTACH has no read-only form
    // short of URI filenames; nothing below writes through `content.`.
    if (!Exec(db, "ATTACH DATABASE " + QuoteLiteral(content_db_path) + " AS content;", err)) {
        err = "cannot attach " + content_db_path + ": " + err;
        return false;
    }

    // Same filter the master's export uses, applied here so a snapshot built by a
    // newer master with extra tables cannot surprise the node.
    auto include = [](const std::string& name) {
        if (name.rfind("asm_", 0) == 0) return true;
        if (name.rfind("mod_data_set_", 0) == 0) return true;
        if (name.rfind("obj_", 0) == 0) return true;
        return name == "map_game_info" || name == "map_object_config";
    };

    // The artifact says which tables to REPLACE (immutable content, rows included)
    // and which to CONVERGE to (per-player tables, definition only). Without that
    // manifest -- an older master -- fall back to "everything with rows is content".
    std::set<std::string> content_set;
    std::set<std::string> schema_set;
    bool have_manifest = false;
    auto read_names = [db](const char* sql, std::set<std::string>& out) -> bool {
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK || !stmt) return false;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* n = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            if (n) out.insert(n);
        }
        sqlite3_finalize(stmt);
        return true;
    };
    have_manifest = read_names("SELECT name FROM content.artifact_content_tables", content_set);
    read_names("SELECT name FROM content.artifact_schema_tables", schema_set);
    if (!have_manifest) {
        Logger::Log(kTag,
            "[NodeData] snapshot has no table manifest (older master): every table with "
            "rows is treated as content, and no schema convergence happens\n");
    }

    const auto tables = QueryTwoColumns(db,
        "SELECT name, sql FROM content.sqlite_master "
        "WHERE type='table' AND sql IS NOT NULL AND name NOT LIKE 'sqlite_%' "
        "AND name NOT LIKE 'artifact_%'");

    int replaced = 0;
    int converged = 0;
    std::vector<std::string> done;
    for (const auto& t : tables) {
        const bool is_content =
            have_manifest ? (content_set.count(t.first) != 0) : include(t.first);
        const bool is_schema = have_manifest && (schema_set.count(t.first) != 0);
        if (!is_content && !is_schema) continue;

        if (is_schema) {
            if (!Exec(db, WithIfNotExists(t.second) + ";", err)) {
                Logger::Log(kTag, "[NodeData] create %s failed: %s\n",
                    t.first.c_str(), err.c_str());
                continue;
            }
            ReconcileColumns(db, t.first, err);
            ++converged;
            continue;
        }

        // Drop and recreate rather than trusting the local schema: the snapshot's
        // definition is the authoritative one, and a new column in a newer master
        // would otherwise be silently missing. The table's indexes go with it.
        if (!Exec(db, "DROP TABLE IF EXISTS main." + QuoteIdent(t.first) + ";", err)) {
            Logger::Log(kTag, "[NodeData] drop %s failed: %s\n", t.first.c_str(), err.c_str());
            continue;
        }
        if (!Exec(db, t.second + ";", err)) {
            Logger::Log(kTag, "[NodeData] create %s failed: %s\n", t.first.c_str(), err.c_str());
            continue;
        }
        if (!Exec(db, "INSERT INTO main." + QuoteIdent(t.first) +
                      " SELECT * FROM content." + QuoteIdent(t.first) + ";", err)) {
            Logger::Log(kTag, "[NodeData] copy %s failed: %s\n", t.first.c_str(), err.c_str());
            continue;
        }
        done.push_back(t.first);
        ++replaced;
    }

    int indexes = 0;
    if (!done.empty()) {
        // Column 1 is the index's tbl_name from sqlite_master, so no SQL-text
        // matching is needed to tell which table it belongs to.
        const auto idx = QueryTwoColumns(db,
            "SELECT tbl_name, sql FROM content.sqlite_master "
            "WHERE type='index' AND sql IS NOT NULL");
        for (const auto& i : idx) {
            bool wanted = false;
            for (const auto& t : done) {
                if (t == i.first) {
                    wanted = true;
                    break;
                }
            }
            if (!wanted) continue;
            if (Exec(db, i.second + ";", err)) ++indexes;
        }
    }

    std::string detach_err;
    Exec(db, "DETACH DATABASE content;", detach_err);

    if (replaced == 0 && converged == 0) {
        err = "the snapshot had no usable tables";
        return false;
    }

    if (!Exec(db, "INSERT OR REPLACE INTO node_content_state (id, hash) VALUES (1, " +
                  QuoteLiteral(content_hash) + ");", err)) {
        return false;
    }

    Logger::Log(kTag,
        "[NodeData] applied content %s: %d content table(s), %d index(es), "
        "%d player table(s) converged to the master's schema\n",
        content_hash.c_str(), replaced, indexes, converged);
    return true;
}
