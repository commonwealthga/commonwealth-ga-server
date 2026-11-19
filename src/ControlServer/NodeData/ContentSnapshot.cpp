#include "src/ControlServer/NodeData/ContentSnapshot.hpp"
#include "src/ControlServer/Logger.hpp"

#include "monocypher.h"
#include "sqlite3.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <string>
#include <vector>

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

namespace {

ContentSnapshot::Info g_current;

constexpr const char* kDirTag = "content";

void EnsureDir(const std::string& dir) {
    if (dir.empty() || dir == ".") return;
#ifdef _WIN32
    const int rc = _mkdir(dir.c_str());
#else
    const int rc = mkdir(dir.c_str(), 0755);
#endif
    // EEXIST is the normal case. Anything else surfaces when the first write
    // fails, which is reported with a real path.
    (void)rc;
}

std::string JoinPath(const std::string& dir, const std::string& name) {
    if (dir.empty() || dir == ".") return name;
    const char last = dir.back();
    if (last == '/' || last == '\\') return dir + name;
    return dir + "/" + name;
}

std::string HexDigest(const std::uint8_t* digest, std::size_t len) {
    static const char* hex = "0123456789abcdef";
    std::string out;
    out.reserve(len * 2);
    for (std::size_t i = 0; i < len; ++i) {
        out.push_back(hex[digest[i] >> 4]);
        out.push_back(hex[digest[i] & 0x0F]);
    }
    return out;
}

// BLAKE2b-256 of a whole file; "" when it cannot be read.
std::string HashFile(const std::string& path) {
    std::FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return std::string();

    crypto_blake2b_ctx ctx;
    crypto_blake2b_init(&ctx, 32);
    char buf[64 * 1024];
    std::size_t n = 0;
    while ((n = std::fread(buf, 1, sizeof(buf), f)) > 0) {
        crypto_blake2b_update(&ctx, reinterpret_cast<const std::uint8_t*>(buf), n);
    }
    std::fclose(f);

    std::uint8_t digest[32];
    crypto_blake2b_final(&ctx, digest);
    std::string hex = HexDigest(digest, sizeof(digest));
    crypto_wipe(digest, sizeof(digest));
    return hex;
}

// What a node needs to run a match, and nothing else. Anything ga_* is player or
// match data (master-owned), and map_tg_* are per-map dumps the DLL regenerates.
// Per-player tables go into the artifact as SCHEMA ONLY (no rows): the node needs
// the master's definition of them, not its data. Without this a node's schemas
// come from whatever migrations its own history ran, and the two sides drift --
// which is exactly what produced "ga_character_devices is missing column X".
//
// The match-history tables are excluded: the node never records matches, and they
// are the largest tables in the database.
bool IsSchemaOnlyTable(const std::string& name) {
    if (name.rfind("ga_", 0) != 0) return false;
    if (name == "ga_match_events") return false;
    if (name == "ga_match_player_stats") return false;
    if (name == "ga_match_device_stats") return false;
    return true;
}

bool IncludeTable(const std::string& name) {
    if (name.rfind("asm_", 0) == 0) {
        // ~1 MB of client-side text; the DLL never queries it.
        return name != "asm_data_set_msg_translations";
    }
    if (name.rfind("mod_data_set_", 0) == 0) return true;
    if (name.rfind("obj_", 0) == 0) return true;
    if (name == "map_game_info") return true;
    if (name == "map_object_config") return true;
    return false;
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

std::string QuoteLiteral(const std::string& s) {
    std::string out = "'";
    for (char c : s) {
        if (c == '\'') out.push_back('\'');
        out.push_back(c);
    }
    out.push_back('\'');
    return out;
}

struct Row {
    std::string name;
    std::string sql;
};

std::vector<Row> QueryRows(sqlite3* db, const std::string& sql) {
    std::vector<Row> rows;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK || !stmt) return rows;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* a = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        const char* b = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        rows.push_back(Row{a ? a : "", b ? b : ""});
    }
    sqlite3_finalize(stmt);
    return rows;
}

}  // namespace

ContentSnapshot::Info ContentSnapshot::Ensure(const std::string& db_path,
                                              const std::string& dir) {
    Info info;

    EnsureDir(dir);
    const std::string tmp = JoinPath(dir, ".content-build.tmp");
    // A previous run may have left a half-built file; opening it and re-creating
    // tables would silently keep stale rows from it.
    std::remove(tmp.c_str());

    sqlite3* dst = nullptr;
    if (sqlite3_open_v2(tmp.c_str(), &dst,
                        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr) != SQLITE_OK) {
        info.error = "cannot create " + tmp;
        Logger::Log(kDirTag, "[ContentSnapshot] %s\n", info.error.c_str());
        sqlite3_close(dst);
        return info;
    }

    std::string err;
    // Fresh build every time: a stale cache is the only way a content change can
    // silently not reach the nodes. That is only affordable because the build is
    // now cheap -- see the transaction below.
    //
    // journal_mode=OFF / synchronous=OFF: this file is a rebuildable artifact, so
    // there is nothing to recover. With the defaults, every CREATE / INSERT /
    // INDEX was its own transaction with its own fsync -- ~450 of them, which on
    // a bind-mounted or network filesystem is where a minute of startup went.
    (void)Exec(dst, "PRAGMA journal_mode=OFF;", err);
    (void)Exec(dst, "PRAGMA synchronous=OFF;", err);
    (void)Exec(dst, "PRAGMA cache_size=-65536;", err);   // 64 MB, whole artifact in RAM
    if (!Exec(dst, "ATTACH DATABASE " + QuoteLiteral(db_path) + " AS src;", err)) {
        info.error = "cannot attach " + db_path + ": " + err;
        Logger::Log(kDirTag, "[ContentSnapshot] %s\n", info.error.c_str());
        sqlite3_close(dst);
        return info;
    }

    // One transaction for the whole build. ATTACH and DETACH must stay outside it.
    const auto build_started = std::chrono::steady_clock::now();
    const bool in_txn = Exec(dst, "BEGIN;", err);

    const auto tables = QueryRows(dst,
        "SELECT name, sql FROM src.sqlite_master "
        "WHERE type='table' AND sql IS NOT NULL AND name NOT LIKE 'sqlite_%' ORDER BY name");

    int copied_tables = 0;
    std::vector<std::string> included;
    std::vector<std::string> schema_only;
    for (const auto& t : tables) {
        // Schema-only tables: definition travels, rows do not.
        if (!IncludeTable(t.name)) {
            if (IsSchemaOnlyTable(t.name)) {
                if (Exec(dst, t.sql + ";", err)) schema_only.push_back(t.name);
            }
            continue;
        }
        // Reuse the source CREATE TABLE so types, constraints and defaults survive
        // (a CREATE TABLE AS SELECT would drop them; affinity differences change
        // query plans and comparisons on the node).
        if (!Exec(dst, t.sql + ";", err)) {
            Logger::Log(kDirTag, "[ContentSnapshot] create %s failed: %s\n",
                t.name.c_str(), err.c_str());
            continue;
        }
        if (!Exec(dst, "INSERT INTO main." + QuoteLiteral(t.name) +
                       " SELECT * FROM src." + QuoteLiteral(t.name) + ";", err)) {
            Logger::Log(kDirTag, "[ContentSnapshot] copy %s failed: %s\n",
                t.name.c_str(), err.c_str());
            continue;
        }
        included.push_back(t.name);
        ++copied_tables;
    }

    // Preserve the source indexes: the node's hot lookups (effect groups by id,
    // device modes by device) are exactly what these cover.
    int copied_indexes = 0;
    if (!included.empty()) {
        // The first column is the index's tbl_name from sqlite_master, so no
        // SQL-text matching is needed to tell which table it belongs to.
        const auto indexes = QueryRows(dst,
            "SELECT tbl_name, sql FROM src.sqlite_master "
            "WHERE type='index' AND sql IS NOT NULL ORDER BY name");
        for (const auto& idx : indexes) {
            if (std::find(included.begin(), included.end(), idx.name) == included.end()) {
                continue;   // index on a table we did not copy
            }
            if (Exec(dst, idx.sql + ";", err)) ++copied_indexes;
        }
    }
    // Mark which is which, so the node can tell "replace these" from "converge to
    // these" without guessing from row counts.
    Exec(dst, "CREATE TABLE IF NOT EXISTS artifact_content_tables (name TEXT PRIMARY KEY);",
         err);
    Exec(dst, "CREATE TABLE IF NOT EXISTS artifact_schema_tables (name TEXT PRIMARY KEY);", err);
    for (const auto& n : included) {
        Exec(dst, "INSERT OR IGNORE INTO artifact_content_tables (name) VALUES (" +
                  QuoteLiteral(n) + ");", err);
    }
    for (const auto& n : schema_only) {
        Exec(dst, "INSERT OR IGNORE INTO artifact_schema_tables (name) VALUES (" +
                  QuoteLiteral(n) + ");", err);
    }

    if (in_txn) Exec(dst, "COMMIT;", err);
    Exec(dst, "DETACH DATABASE src;", err);
    sqlite3_close(dst);

    const auto build_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - build_started).count();

    if (copied_tables == 0) {
        info.error = "no content tables copied from " + db_path;
        Logger::Log(kDirTag, "[ContentSnapshot] %s\n", info.error.c_str());
        std::remove(tmp.c_str());
        return info;
    }

    info.hash = HashFile(tmp);
    if (info.hash.empty()) {
        info.error = "cannot hash " + tmp;
        Logger::Log(kDirTag, "[ContentSnapshot] %s\n", info.error.c_str());
        std::remove(tmp.c_str());
        return info;
    }

    const std::string final_path = JoinPath(dir, std::string("content-") + info.hash + ".db");
    std::FILE* existing = std::fopen(final_path.c_str(), "rb");
    if (existing) {
        // Already on disk with exactly this content: keep it, drop the rebuild.
        std::fclose(existing);
        std::remove(tmp.c_str());
    } else if (std::rename(tmp.c_str(), final_path.c_str()) != 0) {
        info.error = "cannot move the snapshot into place at " + final_path;
        Logger::Log(kDirTag, "[ContentSnapshot] %s\n", info.error.c_str());
        std::remove(tmp.c_str());
        return info;
    }

    info.ok   = true;
    info.path = final_path;
    std::FILE* f = std::fopen(final_path.c_str(), "rb");
    if (f) {
        std::fseek(f, 0, SEEK_END);
        info.size = static_cast<std::int64_t>(std::ftell(f));
        std::fclose(f);
    }

    Logger::Log(kDirTag,
        "[ContentSnapshot] %d content tables (%d indexes) + %zu schema-only -> %s "
        "(%lld KB) hash=%s in %lld ms\n",
        copied_tables, copied_indexes, schema_only.size(), final_path.c_str(),
        (long long)(info.size / 1024), info.hash.c_str(), (long long)build_ms);
    if (!schema_only.empty()) {
        Logger::Log(kDirTag,
            "[ContentSnapshot] schema-only tables carry the master's definitions for the "
            "per-player tables, so nodes converge to one schema instead of their own "
            "migration history\n");
    }

    g_current = info;
    return info;
}

const ContentSnapshot::Info& ContentSnapshot::Current() {
    return g_current;
}

bool ContentSnapshot::ReadChunk(std::int64_t offset, std::int64_t len,
                                std::string& out, std::string& err) {
    out.clear();
    if (!g_current.ok) {
        err = "no content snapshot available";
        return false;
    }
    if (offset < 0 || len <= 0 || offset + len > g_current.size) {
        err = "range out of bounds";
        return false;
    }

    std::FILE* f = std::fopen(g_current.path.c_str(), "rb");
    if (!f) {
        err = "cannot open the snapshot";
        return false;
    }
    if (std::fseek(f, static_cast<long>(offset), SEEK_SET) != 0) {
        std::fclose(f);
        err = "seek failed";
        return false;
    }
    out.resize(static_cast<std::size_t>(len));
    const std::size_t got = std::fread(&out[0], 1, out.size(), f);
    std::fclose(f);
    if (got != out.size()) {
        out.clear();
        err = "short read";
        return false;
    }
    return true;
}
