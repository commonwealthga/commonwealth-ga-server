#include "src/ControlServer/NodeData/PlayerBundle.hpp"
#include "src/ControlServer/Database/Database.hpp"
#include "src/ControlServer/Logger.hpp"
#include "src/Shared/HexUtils.hpp"
#include "lib/nlohmann/json.hpp"

#include "sqlite3.h"

#include <set>
#include <string>
#include <vector>

namespace {

const char* kTag = "bundle";

// One carried table: where its rows come from, and whether both directions carry
// it. `key` selects which id the WHERE clause binds to.
// Which directions carry a table.
//
// The UP direction is deliberately narrow: only what a running instance can
// actually modify. It looked symmetric at first, but shipping the inventory back
// every push meant ~1800 rows, ~1800 INSERTs on the master's io thread, per player
// per push -- a write burst that stalls logins, chat and IPC for its duration. The
// DLL never writes ga_players_inventory or ga_user_components at all, so there is
// nothing to report from them.
enum class Carried {
    DownOnly,   // account/roster data the instance reads, never writes
    Both,
};

struct Spec {
    const char* table;
    const char* key_column;   // id | user_id | character_id
    Carried     carried;
};

const Spec kSpecs[] = {
    { "ga_users",             "id",           Carried::DownOnly },  // id/username only
    { "ga_characters",        "user_id",      Carried::Both },      // DLL creates/updates
    { "ga_character_devices", "character_id", Carried::Both },      // DLL deletes+reinserts
    { "ga_character_skills",  "character_id", Carried::Both },
    { "ga_players_inventory", "user_id",      Carried::DownOnly },  // read-only for the DLL
    { "ga_user_components",   "user_id",      Carried::DownOnly },  // account-wide
    { "ga_user_preferences",  "user_id",      Carried::Both },      // DLL writes prefs
    { "ga_players",           "user_id",      Carried::Both },      // presence rows
};

// Columns that must never leave the master, whatever direction.
bool ExcludedColumn(const std::string& table, const std::string& column) {
    if (table == "ga_users") {
        // The DLL reads id and username; everything else is account material.
        return column != "id" && column != "username";
    }
    return false;
}

struct ColumnInfo {
    std::string name;
    std::string type;
    bool        is_primary_key = false;
};

std::vector<ColumnInfo> TableColumns(sqlite3* db, const std::string& table) {
    std::vector<ColumnInfo> cols;
    sqlite3_stmt* stmt = nullptr;
    const std::string sql = "PRAGMA table_info(" + table + ")";
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK || !stmt) return cols;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ColumnInfo c;
        const char* name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const char* type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        c.name = name ? name : "";
        c.type = type ? type : "";
        c.is_primary_key = sqlite3_column_int(stmt, 5) > 0;
        if (!c.name.empty()) cols.push_back(c);
    }
    sqlite3_finalize(stmt);
    return cols;
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

// BLOBs travel hex-encoded under a marker key, because JSON has no byte type and
// the alternative (base64 everywhere) would obscure the readable columns.
nlohmann::json ColumnToJson(sqlite3_stmt* stmt, int index) {
    switch (sqlite3_column_type(stmt, index)) {
        case SQLITE_NULL:
            return nullptr;
        case SQLITE_INTEGER:
            return static_cast<long long>(sqlite3_column_int64(stmt, index));
        case SQLITE_FLOAT:
            return sqlite3_column_double(stmt, index);
        case SQLITE_BLOB: {
            const void* blob = sqlite3_column_blob(stmt, index);
            const int len = sqlite3_column_bytes(stmt, index);
            nlohmann::json wrapper;
            wrapper["$b"] = HexUtils::hex_encode(
                reinterpret_cast<const std::uint8_t*>(blob), static_cast<std::size_t>(len));
            return wrapper;
        }
        case SQLITE_TEXT:
        default: {
            const char* text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, index));
            return std::string(text ? text : "");
        }
    }
}

void BindJsonValue(sqlite3_stmt* stmt, int index, const nlohmann::json& v) {
    if (v.is_null()) {
        sqlite3_bind_null(stmt, index);
    } else if (v.is_boolean()) {
        sqlite3_bind_int(stmt, index, v.get<bool>() ? 1 : 0);
    } else if (v.is_number_integer()) {
        sqlite3_bind_int64(stmt, index, v.get<long long>());
    } else if (v.is_number_unsigned()) {
        sqlite3_bind_int64(stmt, index, static_cast<long long>(v.get<unsigned long long>()));
    } else if (v.is_number()) {
        sqlite3_bind_double(stmt, index, v.get<double>());
    } else if (v.is_object() && v.contains("$b")) {
        const std::vector<std::uint8_t> bytes = HexUtils::hex_decode(v["$b"].get<std::string>());
        if (bytes.empty()) {
            sqlite3_bind_blob(stmt, index, "", 0, SQLITE_TRANSIENT);
        } else {
            sqlite3_bind_blob(stmt, index, bytes.data(), static_cast<int>(bytes.size()),
                              SQLITE_TRANSIENT);
        }
    } else if (v.is_string()) {
        const std::string s = v.get<std::string>();
        sqlite3_bind_text(stmt, index, s.c_str(), static_cast<int>(s.size()), SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt, index);
    }
}

std::int64_t KeyFor(const Spec& spec, std::int64_t user_id, std::int64_t character_id) {
    const std::string key = spec.key_column;
    if (key == "character_id") return character_id;
    return user_id;   // "id" (ga_users) and "user_id" are both the user's id
}

// A node can legitimately carry columns its own DLL migrations added; the master
// ignores what it does not know. Log each (table, column) pair once instead of on
// every bundle -- the periodic write-back would otherwise turn this into a wall of
// identical warnings that hides real problems.
bool WarnOnce(const std::string& table, const std::string& column) {
    static std::set<std::string> seen;
    return seen.insert(table + "." + column).second;
}

bool Exec(sqlite3* db, const std::string& sql, std::string& err) {
    char* msg = nullptr;
    if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &msg) != SQLITE_OK) {
        err = msg ? msg : "unknown error";
        sqlite3_free(msg);
        return false;
    }
    return true;
}

}  // namespace

std::string PlayerBundle::DescribeTables(Direction dir) {
    std::string out;
    for (const auto& spec : kSpecs) {
        if (dir == Direction::Up && spec.carried == Carried::DownOnly) continue;
        if (!out.empty()) out += ",";
        out += spec.table;
    }
    return out;
}

bool PlayerBundle::Build(std::int64_t user_id, std::int64_t character_id,
                         Direction dir, std::string& json_out, std::string& err) {
    sqlite3* db = Database::GetConnection();
    if (!db) {
        err = "no database connection";
        return false;
    }
    if (user_id <= 0) {
        err = "no user id";
        return false;
    }

    nlohmann::json tables = nlohmann::json::object();
    for (const auto& spec : kSpecs) {
        if (dir == Direction::Up && spec.carried == Carried::DownOnly) continue;
        if (std::string(spec.key_column) == "character_id" && character_id <= 0) continue;

        const auto cols = TableColumns(db, spec.table);
        if (cols.empty()) continue;   // table absent on this side: skip silently

        std::string column_list;
        int included = 0;
        for (const auto& c : cols) {
            if (ExcludedColumn(spec.table, c.name)) continue;
            if (included++) column_list += ",";
            column_list += QuoteIdent(c.name);
        }
        if (included == 0) continue;

        const std::string sql = "SELECT " + column_list + " FROM " + QuoteIdent(spec.table) +
                                " WHERE " + QuoteIdent(spec.key_column) + " = ?";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK || !stmt) {
            err = std::string("cannot read ") + spec.table + ": " + sqlite3_errmsg(db);
            return false;
        }
        sqlite3_bind_int64(stmt, 1, KeyFor(spec, user_id, character_id));

        nlohmann::json rows = nlohmann::json::array();
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            nlohmann::json row = nlohmann::json::array();
            for (int i = 0; i < included; ++i) row.push_back(ColumnToJson(stmt, i));
            rows.push_back(std::move(row));
        }
        sqlite3_finalize(stmt);

        if (rows.empty()) continue;
        nlohmann::json entry;
        entry["columns"] = nlohmann::json::array();
        for (const auto& c : cols) {
            if (ExcludedColumn(spec.table, c.name)) continue;
            entry["columns"].push_back(c.name);
        }
        entry["rows"] = std::move(rows);
        tables[spec.table] = std::move(entry);
    }

    nlohmann::json out;
    out["user_id"]      = user_id;
    out["character_id"] = character_id;
    out["direction"]    = (dir == Direction::Down) ? "down" : "up";
    out["tables"]       = std::move(tables);
    json_out = out.dump();
    return true;
}

bool PlayerBundle::Apply(const std::string& json_bundle, Direction dir, std::string& err) {
    sqlite3* db = Database::GetConnection();
    if (!db) {
        err = "no database connection";
        return false;
    }

    nlohmann::json bundle = nlohmann::json::parse(
        json_bundle, nullptr, /*allow_exceptions=*/false, /*ignore_comments=*/true);
    if (bundle.is_discarded() || !bundle.is_object() || !bundle.contains("tables")) {
        err = "malformed bundle";
        return false;
    }
    const auto user_id = bundle.value("user_id", (std::int64_t)0);
    const auto character_id = bundle.value("character_id", (std::int64_t)0);
    if (user_id <= 0) {
        err = "bundle has no user id";
        return false;
    }

    // One transaction: a half-applied bundle would leave a player with some of
    // their equipped devices and none of their inventory.
    std::string begin_err;
    const bool in_txn = Exec(db, "BEGIN IMMEDIATE;", begin_err);
    if (!in_txn) {
        Logger::Log(kTag, "[Bundle] could not open a transaction (%s); applying row by row\n",
            begin_err.c_str());
    }

    int applied_rows = 0;
    int removed_rows = 0;

    for (const auto& spec : kSpecs) {
        if (dir == Direction::Up && spec.carried == Carried::DownOnly) continue;
        const std::string table = spec.table;
        if (!bundle["tables"].contains(table)) continue;

        const auto& entry = bundle["tables"][table];
        if (!entry.is_object() || !entry.contains("columns") || !entry.contains("rows")) continue;
        if (!entry["columns"].is_array() || !entry["rows"].is_array()) continue;

        const auto local_cols = TableColumns(db, table);
        if (local_cols.empty()) continue;   // no such table on this side
        std::vector<std::string> local_names;
        for (const auto& c : local_cols) local_names.push_back(c.name);

        // Two filters, both applied to the *names* while the row arrays keep the
        // bundle's own column order (indices are carried alongside):
        //   1. never accept account material the master does not ship;
        //   2. a column this database does not have yet is skipped, not fatal. A
        //      node's schema can lag the master's, and refusing the whole bundle
        //      over one column would keep the player out of the match entirely.
        struct Use {
            std::size_t index;
            std::string name;
        };
        std::vector<Use> use;
        std::vector<std::string> skipped_absent;
        const auto& bundle_columns = entry["columns"];
        for (std::size_t i = 0; i < bundle_columns.size(); ++i) {
            if (!bundle_columns[i].is_string()) continue;
            const std::string name = bundle_columns[i].get<std::string>();
            if (ExcludedColumn(table, name)) continue;
            bool present = false;
            for (const auto& n : local_names) {
                if (n == name) {
                    present = true;
                    break;
                }
            }
            if (!present) {
                skipped_absent.push_back(name);
                continue;
            }
            use.push_back(Use{i, name});
        }
        for (const auto& name : skipped_absent) {
            if (!WarnOnce(table, name)) continue;
            Logger::Log(kTag,
                "[Bundle] %s.%s is not in this database -- that column does not travel "
                "(reported once)\n", table.c_str(), name.c_str());
        }
        if (use.empty()) continue;

        std::string names;
        std::string placeholders;
        for (std::size_t i = 0; i < use.size(); ++i) {
            if (i) {
                names += ",";
                placeholders += ",";
            }
            names += QuoteIdent(use[i].name);
            placeholders += "?";
        }
        const std::string sql = "INSERT OR REPLACE INTO " + QuoteIdent(table) +
                                " (" + names + ") VALUES (" + placeholders + ")";

        // Prepared once per table, reset per row. Preparing per row was costing a
        // parse and a statement allocation for every one of them.
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK || !stmt) {
            err = std::string("cannot write ") + table + ": " + sqlite3_errmsg(db);
            if (in_txn) {
                std::string rollback_err;
                Exec(db, "ROLLBACK;", rollback_err);
            }
            return false;
        }
        for (const auto& row : entry["rows"]) {
            if (!row.is_array() || row.size() != bundle_columns.size()) continue;
            for (std::size_t i = 0; i < use.size(); ++i) {
                BindJsonValue(stmt, static_cast<int>(i + 1), row[use[i].index]);
            }
            if (sqlite3_step(stmt) == SQLITE_DONE) ++applied_rows;
            sqlite3_reset(stmt);
            sqlite3_clear_bindings(stmt);
        }
        sqlite3_finalize(stmt);

        // Rows the source no longer has must go: the DLL deletes and re-inserts
        // device/skill rows on a loadout change, and a stale row would resurrect an
        // unequipped item.
        //
        // Only where that is actually true, though. Deletions are reconciled:
        //   * always on the way DOWN (the master is authoritative, the node's copy
        //     should match it exactly), and
        //   * on the way UP only for the two tables the running instance owns.
        // Reconciling inventory/components/preferences upwards would delete rows
        // the master legitimately gained while the match was running (a market
        // purchase, an AP grant) simply because the node never saw them.
        const bool instance_owned =
            table == "ga_character_devices" || table == "ga_character_skills";
        const bool reconcile = (dir == Direction::Down) || instance_owned;

        int pk_count = 0;
        std::string pk_col;
        for (const auto& c : local_cols) {
            if (!c.is_primary_key) continue;
            ++pk_count;
            pk_col = c.name;
        }
        if (reconcile && pk_count == 1) {
            const auto key = KeyFor(spec, user_id, character_id);
            std::string keep;
            for (const auto& row : entry["rows"]) {
                if (!row.is_array()) continue;
                for (std::size_t i = 0; i < bundle_columns.size(); ++i) {
                    if (!bundle_columns[i].is_string()) continue;
                    if (bundle_columns[i].get<std::string>() != pk_col) continue;
                    if (!keep.empty()) keep += ",";
                    if (row[i].is_number_integer()) {
                        keep += std::to_string(row[i].get<long long>());
                    } else if (row[i].is_string()) {
                        // Text keys (ga_players.session_guid) are quoted literals.
                        keep += "'";
                        for (char ch : row[i].get<std::string>()) {
                            if (ch == '\'') keep += "''";
                            keep += ch;
                        }
                        keep += "'";
                    }
                    break;
                }
            }
            std::string sql = "DELETE FROM " + QuoteIdent(table) + " WHERE " +
                              QuoteIdent(spec.key_column) + " = " + std::to_string(key);
            if (!keep.empty()) sql += " AND " + QuoteIdent(pk_col) + " NOT IN (" + keep + ")";
            if (Exec(db, sql + ";", err)) removed_rows += sqlite3_changes(db);
        }
    }

    if (in_txn) {
        std::string commit_err;
        if (!Exec(db, "COMMIT;", commit_err)) {
            err = "commit failed: " + commit_err;
            return false;
        }
    }

    Logger::Log(kTag, "[Bundle] applied %s bundle for user=%lld character=%lld: %d row(s), %d removed\n",
        (dir == Direction::Down) ? "down" : "up", (long long)user_id,
        (long long)character_id, applied_rows, removed_rows);
    return true;
}
