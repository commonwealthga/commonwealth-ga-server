#include "src/ControlServer/NodeStore/NodeStore.hpp"

#include "src/ControlServer/Config/ControlServerConfig.hpp"
#include "src/ControlServer/Database/Database.hpp"
#include "src/ControlServer/Logger.hpp"
#include "src/ControlServer/NodeStore/NodeKeyVault.hpp"

#include "sqlite3.h"

#include <ctime>
#include <string>
#include <utility>
#include <vector>

namespace {

constexpr const char* kTag = "node";

int64_t ColInt64(sqlite3_stmt* s, int i) { return sqlite3_column_int64(s, i); }

std::string ColText(sqlite3_stmt* s, int i) {
    const unsigned char* t = sqlite3_column_text(s, i);
    return t ? reinterpret_cast<const char*>(t) : "";
}

// One-time import of the config file's entries into empty tables.
bool ImportFromConfig(sqlite3* db, const ControlServerConfig& cfg, std::string& err) {
    if (!cfg.sites.empty()) {
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db,
                "INSERT OR IGNORE INTO ga_sites (id, name_msg_id, label, enabled, sort_order) "
                "VALUES (?, ?, ?, ?, ?)", -1, &stmt, nullptr) != SQLITE_OK) {
            err = sqlite3_errmsg(db);
            return false;
        }
        int order = 0;
        for (const auto& site : cfg.sites) {
            sqlite3_reset(stmt);
            sqlite3_bind_int(stmt, 1, site.id);
            sqlite3_bind_int(stmt, 2, site.name_msg_id);
            sqlite3_bind_text(stmt, 3, site.label.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(stmt, 4, site.enabled ? 1 : 0);
            sqlite3_bind_int(stmt, 5, order++);
            sqlite3_step(stmt);
        }
        sqlite3_finalize(stmt);
        Logger::Log(kTag, "[NodeStore] imported %zu site(s) from the config file\n",
            cfg.sites.size());
    }

    if (!cfg.nodes.empty()) {
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db,
                "INSERT OR IGNORE INTO ga_nodes (id, name, site, address, capacity, home_map, "
                "enabled, key_sealed, updated_at) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)",
                -1, &stmt, nullptr) != SQLITE_OK) {
            err = sqlite3_errmsg(db);
            return false;
        }
        for (const auto& node : cfg.nodes) {
            std::string sealed;
            if (!node.key.empty()) {
                if (!NodeKeyVault::Configured()) {
                    // Do not fail startup over this: a master that refuses to boot
                    // because it cannot *seal* a key is worse than one that boots
                    // with a node that cannot authenticate. The recovery is
                    // spelled out because the key is now no longer read from the
                    // config file.
                    Logger::Log(kTag,
                        "[NodeStore] node '%s' imported WITHOUT its key: "
                        "node_key_secret is not set. Set it in control-server.json, "
                        "delete the ga_nodes row for node id %d, and restart to "
                        "re-import with the key sealed.\n",
                        node.name.c_str(), node.id);
                } else {
                    sealed = NodeKeyVault::Seal(node.key);
                    if (sealed.empty()) {
                        Logger::Log(kTag,
                            "[NodeStore] node '%s': sealing its key failed; importing "
                            "without it\n", node.name.c_str());
                    }
                }
            }
            sqlite3_reset(stmt);
            sqlite3_bind_int(stmt, 1, node.id);
            sqlite3_bind_text(stmt, 2, node.name.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(stmt, 3, node.site);
            sqlite3_bind_text(stmt, 4, node.address.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(stmt, 5, node.capacity);
            sqlite3_bind_int(stmt, 6, node.home_map ? 1 : 0);
            sqlite3_bind_int(stmt, 7, node.enabled ? 1 : 0);
            if (sealed.empty()) sqlite3_bind_null(stmt, 8);
            else sqlite3_bind_text(stmt, 8, sealed.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int64(stmt, 9, static_cast<int64_t>(std::time(nullptr)));
            sqlite3_step(stmt);
        }
        sqlite3_finalize(stmt);
        Logger::Log(kTag,
            "[NodeStore] imported %zu node(s) from the config file (keys sealed)\n",
            cfg.nodes.size());
    }
    return true;
}

}  // namespace

namespace NodeStore {

bool Load(ControlServerConfig& cfg, std::string& err) {
    sqlite3* db = Database::GetConnection();
    if (!db) {
        err = "no database connection";
        return false;
    }

    // Import once, before reading: an existing single-machine setup keeps working
    // and the file stops being consulted afterwards.
    int64_t sites_rows = 0, nodes_rows = 0;
    {
        sqlite3_stmt* stmt = nullptr;
        const int rc = sqlite3_prepare_v2(db,
            "SELECT (SELECT count(*) FROM ga_sites), (SELECT count(*) FROM ga_nodes)",
            -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            // Almost always "no such table": the schema bootstrap did not reach
            // migration 175. Say that instead of leaking a bare SQLite message.
            err = std::string("ga_sites/ga_nodes are missing -- migration "
                              "175-sites-and-nodes.sql did not apply (see the schema "
                              "bootstrap line above): ") + sqlite3_errmsg(db);
            sqlite3_finalize(stmt);
            return false;
        }
        {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                sites_rows = ColInt64(stmt, 0);
                nodes_rows = ColInt64(stmt, 1);
            }
            sqlite3_finalize(stmt);
        }
    }
    if (sites_rows == 0 && nodes_rows == 0 &&
        (!cfg.sites.empty() || !cfg.nodes.empty())) {
        if (!ImportFromConfig(db, cfg, err)) return false;
    } else if (!cfg.sites.empty() || !cfg.nodes.empty()) {
        Logger::Log(kTag,
            "[NodeStore] sites/nodes are database rows now; the config file's entries are "
            "ignored (%lld site(s), %lld node(s) in the database)\n",
            (long long)sites_rows, (long long)nodes_rows);
    }

    std::vector<SiteEntryConfig> sites;
    {
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db,
                "SELECT id, name_msg_id, label, enabled FROM ga_sites ORDER BY sort_order, id",
                -1, &stmt, nullptr) != SQLITE_OK) {
            err = sqlite3_errmsg(db);
            return false;
        }
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            SiteEntryConfig site;
            site.id          = sqlite3_column_int(stmt, 0);
            site.name_msg_id = sqlite3_column_int(stmt, 1);
            site.label       = ColText(stmt, 2);
            site.enabled     = sqlite3_column_int(stmt, 3) != 0;
            sites.push_back(site);
        }
        sqlite3_finalize(stmt);
    }

    std::vector<NodeEntryConfig> nodes;
    {
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db,
                "SELECT id, name, site, address, capacity, home_map, enabled, key_sealed "
                "FROM ga_nodes ORDER BY id", -1, &stmt, nullptr) != SQLITE_OK) {
            err = sqlite3_errmsg(db);
            return false;
        }
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            NodeEntryConfig node;
            node.id       = sqlite3_column_int(stmt, 0);
            node.name     = ColText(stmt, 1);
            node.site     = sqlite3_column_int(stmt, 2);
            node.address  = ColText(stmt, 3);
            node.capacity = sqlite3_column_int(stmt, 4);
            node.home_map = sqlite3_column_int(stmt, 5) != 0;
            node.enabled  = sqlite3_column_int(stmt, 6) != 0;

            const bool has_key = sqlite3_column_type(stmt, 7) != SQLITE_NULL;
            if (has_key) {
                std::string key_err;
                if (!NodeKeyVault::Open(ColText(stmt, 7), node.key, key_err)) {
                    // Do not silently enrol a node we cannot authenticate: its
                    // handshakes would fail anyway, with a much worse message.
                    Logger::Log(kTag,
                        "[NodeStore] node '%s' (id %d): cannot unseal its key (%s); "
                        "it will refuse all handshakes\n",
                        node.name.c_str(), node.id, key_err.c_str());
                }
            }
            nodes.push_back(node);
        }
        sqlite3_finalize(stmt);
    }

    cfg.sites = std::move(sites);
    cfg.nodes = std::move(nodes);
    Logger::Log(kTag, "[NodeStore] loaded %zu site(s) and %zu node(s) from the database\n",
        cfg.sites.size(), cfg.nodes.size());
    return true;
}

bool BumpRevision() {
    sqlite3* db = Database::GetConnection();
    if (!db) return false;
    char* err = nullptr;
    // cs_settings is ("key", value): not the (config_key, config_value) shape
    // ga_user_preferences uses.
    const bool ok = sqlite3_exec(db,
        "INSERT INTO cs_settings (\"key\", value) VALUES ('config_rev', '1') "
        "ON CONFLICT(\"key\") DO UPDATE SET "
        "value = CAST(CAST(value AS INTEGER) + 1 AS TEXT);",
        nullptr, nullptr, &err) == SQLITE_OK;
    if (!ok && err) sqlite3_free(err);
    return ok;
}

int64_t Revision() {
    sqlite3* db = Database::GetConnection();
    if (!db) return -1;
    sqlite3_stmt* stmt = nullptr;
    int64_t rev = -1;
    if (sqlite3_prepare_v2(db,
            "SELECT value FROM cs_settings WHERE \"key\" = 'config_rev'",
            -1, &stmt, nullptr) == SQLITE_OK && stmt) {
        if (sqlite3_step(stmt) == SQLITE_ROW) rev = sqlite3_column_int64(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return rev;
}


// ─── Editing ─────────────────────────────────────────────────────────────────

bool SaveNode(const NodeRecord& node, std::string& err) {
    sqlite3* db = Database::GetConnection();
    if (!db) { err = "no database connection"; return false; }
    if (node.id <= 0) { err = "node id must be > 0 (0 is the master)"; return false; }
    if (node.name.empty()) { err = "node name is required"; return false; }

    // A key is only written when one was supplied: editing a node's capacity must
    // not wipe its sealed key.
    std::string sealed;
    if (!node.key.empty()) {
        if (!NodeKeyVault::Configured()) {
            err = "node_key_secret is not set, so a key cannot be sealed";
            return false;
        }
        sealed = NodeKeyVault::Seal(node.key);
        if (sealed.empty()) { err = "could not seal the key"; return false; }
    }

    sqlite3_stmt* stmt = nullptr;
    const char* sql = sealed.empty()
        ? "UPDATE ga_nodes SET name=?, site=?, address=?, capacity=?, home_map=?, enabled=?, "
          "updated_at=? WHERE id=?"
        : "UPDATE ga_nodes SET name=?, site=?, address=?, capacity=?, home_map=?, enabled=?, "
          "updated_at=?, key_sealed=? WHERE id=?";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        err = sqlite3_errmsg(db);
        return false;
    }
    sqlite3_bind_text(stmt, 1, node.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, node.site);
    sqlite3_bind_text(stmt, 3, node.address.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, node.capacity);
    sqlite3_bind_int(stmt, 5, node.home_map ? 1 : 0);
    sqlite3_bind_int(stmt, 6, node.enabled ? 1 : 0);
    sqlite3_bind_int64(stmt, 7, static_cast<int64_t>(std::time(nullptr)));
    if (sealed.empty()) {
        sqlite3_bind_int(stmt, 8, node.id);
    } else {
        sqlite3_bind_text(stmt, 8, sealed.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 9, node.id);
    }
    const bool updated = (sqlite3_step(stmt) == SQLITE_DONE) &&
                         (sqlite3_changes(db) > 0);
    sqlite3_finalize(stmt);

    if (!updated) {
        // Insert. The key is mandatory for a new node: without it the node can
        // never complete a handshake, and finding that out at enrolment beats
        // finding it out at deploy time.
        if (sealed.empty()) {
            err = "a key is required when creating a node";
            return false;
        }
        if (sqlite3_prepare_v2(db,
                "INSERT INTO ga_nodes (id, name, site, address, capacity, home_map, enabled, "
                "key_sealed, updated_at) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)",
                -1, &stmt, nullptr) != SQLITE_OK) {
            err = sqlite3_errmsg(db);
            return false;
        }
        sqlite3_bind_int(stmt, 1, node.id);
        sqlite3_bind_text(stmt, 2, node.name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 3, node.site);
        sqlite3_bind_text(stmt, 4, node.address.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 5, node.capacity);
        sqlite3_bind_int(stmt, 6, node.home_map ? 1 : 0);
        sqlite3_bind_int(stmt, 7, node.enabled ? 1 : 0);
        sqlite3_bind_text(stmt, 8, sealed.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt, 9, static_cast<int64_t>(std::time(nullptr)));
        const int rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) err = sqlite3_errmsg(db);
        sqlite3_finalize(stmt);
        if (rc != SQLITE_DONE) return false;
    }

    BumpRevision();
    return true;
}

bool DeleteNode(int id, std::string& err) {
    sqlite3* db = Database::GetConnection();
    if (!db) { err = "no database connection"; return false; }
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, "DELETE FROM ga_nodes WHERE id = ?", -1, &stmt, nullptr)
            != SQLITE_OK) {
        err = sqlite3_errmsg(db);
        return false;
    }
    sqlite3_bind_int(stmt, 1, id);
    const bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    if (!ok) { err = sqlite3_errmsg(db); return false; }
    BumpRevision();
    return true;
}

bool SaveSite(const SiteRecord& site, std::string& err) {
    sqlite3* db = Database::GetConnection();
    if (!db) { err = "no database connection"; return false; }
    if (site.id <= 0) { err = "site id must be > 0"; return false; }

    sqlite3_stmt* stmt = nullptr;
    // Upsert on the primary key: sites are edited far more often than created.
    if (sqlite3_prepare_v2(db,
            "INSERT INTO ga_sites (id, name_msg_id, label, enabled, sort_order) "
            "VALUES (?, ?, ?, ?, ?) "
            "ON CONFLICT(id) DO UPDATE SET name_msg_id=excluded.name_msg_id, "
            "label=excluded.label, enabled=excluded.enabled, sort_order=excluded.sort_order",
            -1, &stmt, nullptr) != SQLITE_OK) {
        err = sqlite3_errmsg(db);
        return false;
    }
    sqlite3_bind_int(stmt, 1, site.id);
    sqlite3_bind_int(stmt, 2, site.name_msg_id);
    sqlite3_bind_text(stmt, 3, site.label.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, site.enabled ? 1 : 0);
    sqlite3_bind_int(stmt, 5, site.sort_order);
    const int rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) err = sqlite3_errmsg(db);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) return false;

    BumpRevision();
    return true;
}

bool DeleteSite(int id, std::string& err) {
    sqlite3* db = Database::GetConnection();
    if (!db) { err = "no database connection"; return false; }
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, "DELETE FROM ga_sites WHERE id = ?", -1, &stmt, nullptr)
            != SQLITE_OK) {
        err = sqlite3_errmsg(db);
        return false;
    }
    sqlite3_bind_int(stmt, 1, id);
    const bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    if (!ok) { err = sqlite3_errmsg(db); return false; }
    BumpRevision();
    return true;
}



}  // namespace NodeStore
