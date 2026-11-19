#include "src/ControlServer/InstanceRegistry/InstanceRegistry.hpp"
#include "src/ControlServer/Database/Database.hpp"
#include "src/ControlServer/Logger.hpp"
#include "src/ControlServer/MatchmakingService/MatchmakingService.hpp"
#include "src/Shared/MatchSeed.hpp"
#include "sqlite3.h"
#include <random>
#include <set>

std::mutex InstanceRegistry::mutex_;

// Site sources, set once at startup by main() from the config. Both are plain
// integers so this needs no locking beyond the registry's own mutex.
static int g_master_site = 0;
static std::map<int, int> g_node_sites;

void InstanceRegistry::SetSiteSources(int master_site, const std::map<int, int>& node_sites) {
    std::lock_guard<std::mutex> lock(mutex_);
    g_master_site = master_site;
    g_node_sites  = node_sites;
}

int InstanceRegistry::SiteOfInstance(const InstanceInfo& inst) {
    if (inst.node_id == 0) return g_master_site;          // local spawner
    auto it = g_node_sites.find(inst.node_id);
    if (it != g_node_sites.end() && it->second > 0) return it->second;
    return g_master_site;                                  // node with no site declared
}
//std::string InstanceRegistry::s_host_ = "127.0.0.1";

namespace {

// A fresh match seed. Positive (SQLite INTEGER is signed) and non-zero (0 is
// the "unknown/legacy" sentinel), from std::random_device so two instances
// spawned in the same tick don't share a rollout.
uint64_t MintMatchSeed() {
	static std::mt19937_64 eng{std::random_device{}()};
	const uint64_t v = eng() & kMatchSeedMax;
	return v ? v : 1;
}

// One ga_instances row -> InstanceInfo. Every SELECT in this file uses the same
// leading 16 columns; the two queries that also need access_mode /
// owner_party_ids append them, and all of them append node_id last. The reader
// therefore tolerates either width (17 or 19 columns) instead of every call
// site repeating the same 20-line mapping.
InstanceInfo ReadInstanceRow(sqlite3_stmt* stmt) {
    InstanceInfo info;
    info.id           = sqlite3_column_int64(stmt, 0);
    const char* mn    = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    info.map_name     = mn ? mn : "";
    const char* st    = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    info.state        = st ? st : "";
    info.pid          = sqlite3_column_int(stmt, 3);
    info.udp_port     = static_cast<uint16_t>(sqlite3_column_int(stmt, 4));
    // Column 5 is ip_address: unused. Instances are reached through the owning
    // node's advertised address, not a per-row ip.
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

    const int cols = sqlite3_column_count(stmt);
    if (cols >= 19) {
        const char* am = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 16));
        info.access_mode = (am && *am) ? am : "OPEN";
        const char* op = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 17));
        info.owner_party_ids = op ? op : "";
    }
    if (cols >= 17) info.node_id = sqlite3_column_int(stmt, cols - 1);
    return info;
}

}  // namespace

void InstanceRegistry::Init() {
    // No-op: mutex_ is default-constructed.
    // Kept for startup symmetry with PlayerSessionStore::Init().
}

/*void InstanceRegistry::SetHost(const std::string& host) {
    s_host_ = host;
}*/

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
        "       COALESCE(queue_id, 0), COALESCE(predecessor_instance_id, 0), COALESCE(end_mission_at, 0), COALESCE(node_id, 0) "
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
        InstanceInfo info = ReadInstanceRow(stmt);
        result = std::move(info);
    }
    sqlite3_finalize(stmt);
    return result;
}

std::optional<InstanceInfo> InstanceRegistry::GetLiveInstanceByMapName(const std::string& map_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    sqlite3* db = Database::GetConnection();
    if (!db) return std::nullopt;

    // STARTING counts as live — that's what makes the open-world travel path
    // single-instance while a spawn is still in flight. Oldest row first so
    // concurrent callers converge on the same instance.
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db,
        "SELECT id, map_name, state, pid, udp_port, ip_address, player_count, "
        "       started_at, COALESCE(sealed_at, 0), "
        "       COALESCE(instance_id, 0), COALESCE(is_home_map, 0), COALESCE(max_players, 0), "
        "       COALESCE(game_mode, ''), "
        "       COALESCE(queue_id, 0), COALESCE(predecessor_instance_id, 0), COALESCE(end_mission_at, 0), COALESCE(node_id, 0) "
        "FROM ga_instances "
        "WHERE map_name = ? AND state IN ('STARTING','READY') "
        "ORDER BY id ASC LIMIT 1",
        -1, &stmt, nullptr);
    if (rc != SQLITE_OK || !stmt) {
        Logger::Log("db", "[InstanceRegistry] GetLiveInstanceByMapName prepare failed: %s\n",
            sqlite3_errmsg(db));
        return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, map_name.c_str(), -1, SQLITE_TRANSIENT);

    std::optional<InstanceInfo> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        InstanceInfo info = ReadInstanceRow(stmt);
        result = std::move(info);
    }
    sqlite3_finalize(stmt);
    return result;
}

    static const std::string localhost="127.0.0.1";

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
        "(map_name, state, pid, udp_port, ip_address, player_count, started_at, match_seed, seed_scheme) "
        "VALUES (?, 'READY', 0, ?, ?, 0, strftime('%s','now'), ?, ?)",
        -1, &stmt, nullptr);
    if (rc != SQLITE_OK || !stmt) {
        Logger::Log("db", "[InstanceRegistry] SeedHomeMapInstance prepare failed: %s\n",
            sqlite3_errmsg(db));
        return;
    }

    sqlite3_bind_text(stmt, 1, map_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt,  2, static_cast<int>(udp_port));
    sqlite3_bind_text(stmt, 3, /*s_host_*/localhost.c_str(), -1, SQLITE_TRANSIENT);
    // Home maps are seeded too, so ad-hoc tests on them are reproducible.
    const uint64_t home_seed = MintMatchSeed();
    sqlite3_bind_int64(stmt, 4, static_cast<int64_t>(home_seed));
    sqlite3_bind_int  (stmt, 5, kMatchSeedScheme);

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

uint64_t InstanceRegistry::GetMatchSeed(int64_t instance_id) {
    if (instance_id <= 0) return 0;
    sqlite3* db = Database::GetConnection();
    if (!db) return 0;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db,
        "SELECT match_seed FROM ga_instances WHERE instance_id = ? LIMIT 1",
        -1, &stmt, nullptr) != SQLITE_OK || !stmt) {
        return 0;
    }
    sqlite3_bind_int64(stmt, 1, instance_id);
    uint64_t seed = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const sqlite3_int64 v = sqlite3_column_int64(stmt, 0);
        if (v > 0) seed = (uint64_t)v;
    }
    sqlite3_finalize(stmt);
    return seed;
}

void InstanceRegistry::SetMatchSeed(int64_t instance_id, uint64_t seed, int seed_scheme) {
    if (instance_id <= 0) return;
    sqlite3* db = Database::GetConnection();
    if (!db) return;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db,
        "UPDATE ga_instances SET match_seed = ?, seed_scheme = ? WHERE instance_id = ?",
        -1, &stmt, nullptr) != SQLITE_OK || !stmt) {
        return;
    }
    sqlite3_bind_int64(stmt, 1, (sqlite3_int64)(seed & kMatchSeedMax));
    sqlite3_bind_int  (stmt, 2, seed_scheme);
    sqlite3_bind_int64(stmt, 3, instance_id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

namespace {

// Insert body shared by InsertStarting (instance id = rowid) and
// InsertStartingWithId (caller supplies it, so a node can mirror the master's
// row verbatim). Caller holds mutex_. Returns the written instance_id or 0.
int64_t InsertInstanceRow(sqlite3* db,
                          const std::string& map_name,
                          const std::string& game_mode,
                          uint16_t udp_port,
                          int pid,
                          bool is_home_map,
                          uint32_t queue_id,
                          int64_t predecessor_instance_id,
                          const std::string& access_mode,
                          const std::string& owner_party_ids,
                          int node_id,
                          int64_t requested_instance_id,
                          uint64_t match_seed) {
    static const std::string localhost = "127.0.0.1";

    sqlite3_stmt* stmt = nullptr;
    // victory_bonus_lives inherits the queue's configured threshold at spawn
    // time (snapshot, not a live join — later queue edits don't retroactively
    // change running instances). Non-queue spawns (home map, admin) get 0 =
    // challenge bonus off.
    int rc = sqlite3_prepare_v2(db,
        "INSERT INTO ga_instances "
        "(map_name, game_mode, state, pid, udp_port, ip_address, player_count, started_at, instance_id, is_home_map, queue_id, predecessor_instance_id, access_mode, owner_party_ids, victory_bonus_lives, match_seed, seed_scheme, node_id) "
        "VALUES (?, ?, 'STARTING', ?, ?, ?, 0, strftime('%s','now'), ?, ?, ?, ?, ?, ?, "
        "COALESCE((SELECT victory_bonus_lives FROM ga_queues WHERE queue_id = ?), 0), ?, ?, ?)",
        -1, &stmt, nullptr);
    if (rc != SQLITE_OK || !stmt) {
        Logger::Log("db", "[InstanceRegistry] InsertStarting prepare failed: %s\n",
            sqlite3_errmsg(db));
        // Also on 'main': a failure here used to be invisible (the db channel is
        // usually off), and the caller then spawned an instance with id 0.
        Logger::Log("main", "[InstanceRegistry] InsertStarting prepare failed: %s\n",
            sqlite3_errmsg(db));
        return 0;
    }

    sqlite3_bind_text(stmt, 1, map_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, game_mode.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt,  3, pid);
    sqlite3_bind_int(stmt,  4, static_cast<int>(udp_port));
    sqlite3_bind_text(stmt, 5, localhost.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 6, requested_instance_id);   // 0 = patched to the rowid below
    sqlite3_bind_int(stmt,  7, is_home_map ? 1 : 0);
    if (queue_id != 0) sqlite3_bind_int64(stmt, 8, static_cast<int64_t>(queue_id));
    else               sqlite3_bind_null(stmt,  8);
    if (predecessor_instance_id != 0) sqlite3_bind_int64(stmt, 9, predecessor_instance_id);
    else                              sqlite3_bind_null(stmt,  9);
    sqlite3_bind_text(stmt, 10, access_mode.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 11, owner_party_ids.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 12, static_cast<int64_t>(queue_id));  // victory_bonus_lives subselect
    sqlite3_bind_int64(stmt, 13, static_cast<int64_t>(match_seed));
    sqlite3_bind_int (stmt, 14, kMatchSeedScheme);
    sqlite3_bind_int (stmt, 15, node_id);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        Logger::Log("db", "[InstanceRegistry] InsertStarting insert failed: %s\n",
            sqlite3_errmsg(db));
        // Also on 'main' for the same reason -- this is the failure that produced
        // a swarm of -instanceid=0 processes.
        Logger::Log("main", "[InstanceRegistry] InsertStarting insert failed: %s\n",
            sqlite3_errmsg(db));
        return 0;
    }

    int64_t instance_id = requested_instance_id;
    if (instance_id == 0) {
        // Legacy path: the instance id IS the rowid, which only exists after
        // the insert, hence the second statement.
        instance_id = sqlite3_last_insert_rowid(db);
        char* err = nullptr;
        char sql[128];
        snprintf(sql, sizeof(sql),
            "UPDATE ga_instances SET instance_id = %lld WHERE id = %lld",
            (long long)instance_id, (long long)instance_id);
        if (sqlite3_exec(db, sql, nullptr, nullptr, &err) != SQLITE_OK) {
            Logger::Log("db", "[InstanceRegistry] InsertStarting update instance_id failed: %s\n",
                err ? err : "?");
            sqlite3_free(err);
        }
    }

    Logger::Log("db", "[InstanceRegistry] InsertStarting: map=%s udp_port=%d pid=%d is_home_map=%d queue_id=%u predecessor=%lld node_id=%d -> instance_id=%lld seed=%llu scheme=%d\n",
        map_name.c_str(), (int)udp_port, pid, (int)is_home_map,
        queue_id, (long long)predecessor_instance_id, node_id, (long long)instance_id,
        (unsigned long long)match_seed, kMatchSeedScheme);
    return instance_id;
}

}  // namespace

int64_t InstanceRegistry::InsertStarting(const std::string& map_name, const std::string& game_mode,
                                          uint16_t udp_port, int pid, bool is_home_map,
                                          uint32_t queue_id, int64_t predecessor_instance_id,
                                          const std::string& access_mode,
                                          const std::string& owner_party_ids,
                                          int node_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    sqlite3* db = Database::GetConnection();
    if (!db) return 0;

    return InsertInstanceRow(db, map_name, game_mode, udp_port, pid, is_home_map,
                             queue_id, predecessor_instance_id, access_mode,
                             owner_party_ids, node_id,
                             /*requested_instance_id=*/0, MintMatchSeed());
}

int64_t InstanceRegistry::InsertStartingWithId(int64_t instance_id,
                                               const std::string& map_name,
                                               const std::string& game_mode,
                                               uint16_t udp_port, int pid, bool is_home_map,
                                               uint32_t queue_id, int64_t predecessor_instance_id,
                                               const std::string& access_mode,
                                               const std::string& owner_party_ids,
                                               int node_id, uint64_t match_seed,
                                               int victory_bonus_lives) {
    if (instance_id == 0) {
        Logger::Log("db", "[InstanceRegistry] InsertStartingWithId needs a non-zero instance_id\n");
        return 0;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    sqlite3* db = Database::GetConnection();
    if (!db) return 0;

    const uint64_t seed = match_seed != 0 ? match_seed : MintMatchSeed();
    static const std::string localhost = "127.0.0.1";

    // Mirror semantics. The master owns the id, so this row may already exist:
    // either because the node shares the master's database (single-machine test
    // setup) or because the same spawn is being re-dispatched after a reconnect.
    // Reset it to STARTING in place rather than failing the unique index.
    bool exists = false;
    {
        sqlite3_stmt* probe = nullptr;
        if (sqlite3_prepare_v2(db,
                "SELECT 1 FROM ga_instances WHERE instance_id = ?", -1, &probe, nullptr) == SQLITE_OK
            && probe) {
            sqlite3_bind_int64(probe, 1, instance_id);
            exists = (sqlite3_step(probe) == SQLITE_ROW);
        } else {
            Logger::Log("db", "[InstanceRegistry] InsertStartingWithId probe failed: %s\n",
                sqlite3_errmsg(db));
        }
        sqlite3_finalize(probe);
    }

    sqlite3_stmt* stmt = nullptr;
    int rc = SQLITE_OK;
    if (exists) {
        rc = sqlite3_prepare_v2(db,
            "UPDATE ga_instances SET "
            "  map_name = ?, game_mode = ?, state = 'STARTING', pid = ?, udp_port = ?, "
            "  is_home_map = ?, queue_id = ?, predecessor_instance_id = ?, "
            "  access_mode = ?, owner_party_ids = ?, node_id = ?, "
            "  match_seed = ?, seed_scheme = ?, victory_bonus_lives = ?, "
            "  started_at = strftime('%s','now'), sealed_at = NULL, end_mission_at = NULL, "
            "  outcome = NULL, player_count = 0 "
            "WHERE instance_id = ?",
            -1, &stmt, nullptr);
    } else {
        rc = sqlite3_prepare_v2(db,
            "INSERT INTO ga_instances "
            "(map_name, game_mode, state, pid, udp_port, ip_address, player_count, started_at, "
            " instance_id, is_home_map, queue_id, predecessor_instance_id, access_mode, "
            " owner_party_ids, victory_bonus_lives, match_seed, seed_scheme, node_id) "
            "VALUES (?, ?, 'STARTING', ?, ?, ?, 0, strftime('%s','now'), ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
            -1, &stmt, nullptr);
    }
    if (rc != SQLITE_OK || !stmt) {
        Logger::Log("db", "[InstanceRegistry] InsertStartingWithId prepare failed: %s\n",
            sqlite3_errmsg(db));
        return 0;
    }

    // Two statements with different parameter orders, so two bind sequences.
    if (exists) {
        int b = 1;
        sqlite3_bind_text(stmt, b++, map_name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, b++, game_mode.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int (stmt, b++, pid);
        sqlite3_bind_int (stmt, b++, static_cast<int>(udp_port));
        sqlite3_bind_int (stmt, b++, is_home_map ? 1 : 0);
        if (queue_id != 0) sqlite3_bind_int64(stmt, b++, static_cast<int64_t>(queue_id));
        else               sqlite3_bind_null(stmt,  b++);
        if (predecessor_instance_id != 0) sqlite3_bind_int64(stmt, b++, predecessor_instance_id);
        else                              sqlite3_bind_null(stmt,  b++);
        sqlite3_bind_text(stmt, b++, access_mode.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, b++, owner_party_ids.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int (stmt, b++, node_id);
        sqlite3_bind_int64(stmt, b++, static_cast<int64_t>(seed));
        sqlite3_bind_int (stmt, b++, kMatchSeedScheme);
        sqlite3_bind_int (stmt, b++, victory_bonus_lives);
        sqlite3_bind_int64(stmt, b++, instance_id);   // WHERE instance_id = ?
    } else {
        int b = 1;
        sqlite3_bind_text(stmt, b++, map_name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, b++, game_mode.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int (stmt, b++, pid);
        sqlite3_bind_int (stmt, b++, static_cast<int>(udp_port));
        sqlite3_bind_text(stmt, b++, localhost.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt, b++, instance_id);
        sqlite3_bind_int (stmt, b++, is_home_map ? 1 : 0);
        if (queue_id != 0) sqlite3_bind_int64(stmt, b++, static_cast<int64_t>(queue_id));
        else               sqlite3_bind_null(stmt,  b++);
        if (predecessor_instance_id != 0) sqlite3_bind_int64(stmt, b++, predecessor_instance_id);
        else                              sqlite3_bind_null(stmt,  b++);
        sqlite3_bind_text(stmt, b++, access_mode.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, b++, owner_party_ids.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int (stmt, b++, victory_bonus_lives);
        sqlite3_bind_int64(stmt, b++, static_cast<int64_t>(seed));
        sqlite3_bind_int (stmt, b++, kMatchSeedScheme);
        sqlite3_bind_int (stmt, b++, node_id);
    }

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) {
        Logger::Log("db", "[InstanceRegistry] InsertStartingWithId %s failed: %s\n",
            exists ? "update" : "insert", sqlite3_errmsg(db));
        return 0;
    }

    Logger::Log("db",
        "[InstanceRegistry] mirrored instance=%lld (%s) map=%s port=%d pid=%d node_id=%d "
        "bonus_lives=%d seed=%llu\n",
        (long long)instance_id, exists ? "updated" : "inserted", map_name.c_str(),
        (int)udp_port, pid, node_id, victory_bonus_lives, (unsigned long long)seed);
    return instance_id;
}

int InstanceRegistry::CountActiveInstancesForNode(int node_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    sqlite3* db = Database::GetConnection();
    if (!db) return 0;

    sqlite3_stmt* stmt = nullptr;
    int count = 0;
    if (sqlite3_prepare_v2(db,
            "SELECT COUNT(*) FROM ga_instances WHERE node_id = ? AND state != 'STOPPED'",
            -1, &stmt, nullptr) == SQLITE_OK && stmt) {
        sqlite3_bind_int(stmt, 1, node_id);
        if (sqlite3_step(stmt) == SQLITE_ROW) count = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return count;
}

std::vector<InstanceInfo> InstanceRegistry::GetInstancesForNode(int node_id) {
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
        "       COALESCE(queue_id, 0), COALESCE(predecessor_instance_id, 0), COALESCE(end_mission_at, 0), "
        "       COALESCE(node_id, 0) "
        "FROM ga_instances WHERE node_id = ? AND state != 'STOPPED'",
        -1, &stmt, nullptr);
    if (rc != SQLITE_OK || !stmt) {
        Logger::Log("db", "[InstanceRegistry] GetInstancesForNode prepare failed: %s\n",
            sqlite3_errmsg(db));
        return results;
    }
    sqlite3_bind_int(stmt, 1, node_id);
    while (sqlite3_step(stmt) == SQLITE_ROW) results.push_back(ReadInstanceRow(stmt));
    sqlite3_finalize(stmt);
    return results;
}

int InstanceRegistry::GetVictoryBonusLives(int64_t instance_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    sqlite3* db = Database::GetConnection();
    if (!db) return 0;

    sqlite3_stmt* stmt = nullptr;
    int bonus = 0;
    if (sqlite3_prepare_v2(db,
            "SELECT COALESCE(victory_bonus_lives, 0) FROM ga_instances WHERE instance_id = ?",
            -1, &stmt, nullptr) == SQLITE_OK && stmt) {
        sqlite3_bind_int64(stmt, 1, instance_id);
        if (sqlite3_step(stmt) == SQLITE_ROW) bonus = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return bonus;
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

void InstanceRegistry::UpdateUdpPort(int64_t instance_id, uint16_t udp_port) {
    std::lock_guard<std::mutex> lock(mutex_);
    sqlite3* db = Database::GetConnection();
    if (!db) return;

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db,
            "UPDATE ga_instances SET udp_port = ? WHERE instance_id = ?",
            -1, &stmt, nullptr) != SQLITE_OK || !stmt) {
        Logger::Log("db", "[InstanceRegistry] UpdateUdpPort prepare failed: %s\n",
            sqlite3_errmsg(db));
        return;
    }
    sqlite3_bind_int(stmt, 1, static_cast<int>(udp_port));
    sqlite3_bind_int64(stmt, 2, instance_id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    Logger::Log("db", "[InstanceRegistry] instance=%lld udp_port=%d\n",
        (long long)instance_id, (int)udp_port);
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
        "    max_players=CASE WHEN max_players > 0 THEN max_players ELSE ? END, "
        "    last_empty_at=strftime('%s','now') "
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
        "       COALESCE(queue_id, 0), COALESCE(predecessor_instance_id, 0), COALESCE(end_mission_at, 0), COALESCE(node_id, 0) "
        "FROM ga_instances "
        "WHERE predecessor_instance_id = ? AND state != 'STOPPED' "
        "LIMIT 1",
        -1, &stmt, nullptr);
    if (rc != SQLITE_OK || !stmt) return std::nullopt;

    sqlite3_bind_int64(stmt, 1, parent_instance_id);

    std::optional<InstanceInfo> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        InstanceInfo info = ReadInstanceRow(stmt);
        result = std::move(info);
    }
    sqlite3_finalize(stmt);
    return result;
}

void InstanceRegistry::SetMaxPlayers(int64_t instance_id, int max_players) {
    std::lock_guard<std::mutex> lock(mutex_);
    sqlite3* db = Database::GetConnection();
    if (!db) return;

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db,
        "UPDATE ga_instances SET max_players=? WHERE instance_id=?",
        -1, &stmt, nullptr);
    if (rc != SQLITE_OK || !stmt) {
        Logger::Log("db", "[InstanceRegistry] SetMaxPlayers prepare failed: %s\n",
            sqlite3_errmsg(db));
        return;
    }

    sqlite3_bind_int(stmt, 1, max_players);
    sqlite3_bind_int64(stmt, 2, instance_id);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) {
        Logger::Log("db", "[InstanceRegistry] SetMaxPlayers failed: %s\n",
            sqlite3_errmsg(db));
        return;
    }

    Logger::Log("db", "[InstanceRegistry] SetMaxPlayers: instance_id=%lld max_players=%d\n",
        (long long)instance_id, max_players);
}

std::optional<InstanceInfo> InstanceRegistry::GetReadyHomeInstance(int site) {
    std::lock_guard<std::mutex> lock(mutex_);
    sqlite3* db = Database::GetConnection();
    if (!db) return std::nullopt;

    sqlite3_stmt* stmt = nullptr;
    // No LIMIT: an instance's site is derived from its node, not stored, so the
    // filter happens below. There is at most one READY home instance per node, so
    // this returns a handful of rows at most.
    int rc = sqlite3_prepare_v2(db,
        "SELECT id, map_name, state, pid, udp_port, ip_address, player_count, "
        "       started_at, COALESCE(sealed_at, 0), "
        "       COALESCE(instance_id, 0), COALESCE(is_home_map, 0), COALESCE(max_players, 0), "
        "       COALESCE(game_mode, ''), "
        "       COALESCE(queue_id, 0), COALESCE(predecessor_instance_id, 0), COALESCE(end_mission_at, 0), COALESCE(node_id, 0) "
        "FROM ga_instances "
        "WHERE is_home_map=1 AND state='READY' "
        "ORDER BY instance_id",
        -1, &stmt, nullptr);
    if (rc != SQLITE_OK || !stmt) {
        Logger::Log("db", "[InstanceRegistry] GetReadyHomeInstance prepare failed: %s\n",
            sqlite3_errmsg(db));
        return std::nullopt;
    }

    std::optional<InstanceInfo> in_site;
    std::optional<InstanceInfo> on_master;
    std::optional<InstanceInfo> any;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        InstanceInfo info = ReadInstanceRow(stmt);
        const int inst_site = SiteOfInstance(info);
        if (!any) any = info;
        if (inst_site == g_master_site && !on_master) on_master = info;
        if (site != 0 && inst_site == site && !in_site) in_site = info;
        if (site == 0 && !in_site) in_site = info;   // "any site": first row wins
    }
    sqlite3_finalize(stmt);

    // Preferred site, then the master's own hub, then anything ready. Logged when
    // we do not get the first choice -- with two regions this is the difference
    // between "you are in Europe" and "we put you somewhere else".
    if (in_site) return in_site;
    if (on_master) {
        if (site != 0) {
            Logger::Log("tcp",
                "[InstanceRegistry] no READY home instance in site %d -- using the "
                "master's site %d\n", site, g_master_site);
        }
        return on_master;
    }
    if (any && site != 0) {
        Logger::Log("tcp",
            "[InstanceRegistry] no READY home instance in site %d or %d -- using "
            "instance %lld (site %d)\n",
            site, g_master_site, (long long)any->instance_id, SiteOfInstance(*any));
    }
    return any;
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
        "       COALESCE(queue_id, 0), COALESCE(predecessor_instance_id, 0), COALESCE(end_mission_at, 0), "
        "       COALESCE(access_mode, 'OPEN'), COALESCE(owner_party_ids, ''), COALESCE(node_id, 0) "
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
        InstanceInfo info = ReadInstanceRow(stmt);
        result = std::move(info);
    }
    sqlite3_finalize(stmt);
    return result;
}

void InstanceRegistry::MarkStopped(int64_t instance_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    sqlite3* db = Database::GetConnection();
    if (!db) return;

    // Outcome fallback (design 2026-06-12): instance dying without a
    // recorded outcome was CRASHED if players were still on it, else
    // ABANDONED. Write-once; never for home maps. Must run before the
    // left_at wipe below.
    {
        char osql[512];
        snprintf(osql, sizeof(osql),
            "UPDATE ga_instances SET outcome = CASE WHEN "
            "(SELECT COUNT(*) FROM ga_instance_players "
            " WHERE instance_id = %lld AND left_at IS NULL) > 0 "
            "THEN 'CRASHED' ELSE 'ABANDONED' END "
            "WHERE instance_id = %lld AND outcome IS NULL AND is_home_map = 0",
            (long long)instance_id, (long long)instance_id);
        char* oerr = nullptr;
        sqlite3_exec(db, osql, nullptr, nullptr, &oerr);
        if (oerr) { sqlite3_free(oerr); }
    }

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

    // Evict any pre-assigned team entries for this instance — they're
    // dead weight now that the instance is gone. MatchmakingService does
    // not acquire InstanceRegistry::mutex_ so no recursive-lock risk.
    MatchmakingService::DropPreAssignedTeams(instance_id);
    MatchmakingService::DropReadyMatchReservations(instance_id);
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

    // Outcome fallback for instances orphaned by a control-server restart
    // (design 2026-06-12). Same CRASHED/ABANDONED rule as MarkStopped.
    {
        char* oerr = nullptr;
        sqlite3_exec(db,
            "UPDATE ga_instances SET outcome = CASE WHEN "
            "(SELECT COUNT(*) FROM ga_instance_players p "
            " WHERE p.instance_id = ga_instances.instance_id "
            " AND p.left_at IS NULL) > 0 "
            "THEN 'CRASHED' ELSE 'ABANDONED' END "
            "WHERE state != 'STOPPED' AND outcome IS NULL AND is_home_map = 0",
            nullptr, nullptr, &oerr);
        if (oerr) { sqlite3_free(oerr); }
    }

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

void InstanceRegistry::UpdateInstancePlayerTaskForce(int64_t instance_id, const std::string& session_guid,
                                                     int task_force) {
    std::lock_guard<std::mutex> lock(mutex_);
    sqlite3* db = Database::GetConnection();
    if (!db) return;

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db,
        "UPDATE ga_instance_players SET task_force_number = ? "
        "WHERE instance_id = ? AND session_guid = ? AND left_at IS NULL",
        -1, &stmt, nullptr);
    if (rc != SQLITE_OK || !stmt) {
        Logger::Log("db", "[InstanceRegistry] UpdateInstancePlayerTaskForce prepare failed: %s\n",
            sqlite3_errmsg(db));
        return;
    }

    sqlite3_bind_int(stmt, 1, task_force);
    sqlite3_bind_int64(stmt, 2, instance_id);
    sqlite3_bind_text(stmt, 3, session_guid.c_str(), -1, SQLITE_TRANSIENT);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        Logger::Log("db", "[InstanceRegistry] UpdateInstancePlayerTaskForce failed: %s\n",
            sqlite3_errmsg(db));
        return;
    }

    Logger::Log("db", "[InstanceRegistry] UpdateInstancePlayerTaskForce: instance=%lld guid=%s tf=%d changed=%d\n",
        (long long)instance_id, session_guid.c_str(), task_force, sqlite3_changes(db));
}

std::optional<std::pair<int64_t, int>>
InstanceRegistry::GetInstancePlayerTaskForce(const std::string& session_guid) {
    std::lock_guard<std::mutex> lock(mutex_);
    sqlite3* db = Database::GetConnection();
    if (!db) return std::nullopt;

    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "SELECT instance_id, task_force_number FROM ga_instance_players "
        "WHERE session_guid = ? AND left_at IS NULL "
        // Newest first: if a previous instance's row was ever left unstamped,
        // an unordered LIMIT 1 would hand back the stale instance/task force.
        "ORDER BY joined_at DESC, id DESC "
        "LIMIT 1";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK || !stmt) {
        Logger::Log("db", "[InstanceRegistry] GetInstancePlayerTaskForce prepare failed: %s\n",
            sqlite3_errmsg(db));
        return std::nullopt;
    }
    sqlite3_bind_text(stmt, 1, session_guid.c_str(), -1, SQLITE_TRANSIENT);

    std::optional<std::pair<int64_t, int>> out;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int64_t inst_id = sqlite3_column_int64(stmt, 0);
        int     tf      = sqlite3_column_int(stmt, 1);
        out = std::make_pair(inst_id, tf);
    }
    sqlite3_finalize(stmt);
    return out;
}

std::vector<InstanceRegistry::InstancePlayerRef>
InstanceRegistry::GetAllActiveInstancePlayers() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<InstancePlayerRef> out;
    sqlite3* db = Database::GetConnection();
    if (!db) return out;

    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "SELECT instance_id, session_guid "
        "FROM ga_instance_players "
        "WHERE left_at IS NULL";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK || !stmt) {
        Logger::Log("db", "[InstanceRegistry] GetAllActiveInstancePlayers prepare failed: %s\n",
            sqlite3_errmsg(db));
        return out;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        InstancePlayerRef row;
        row.instance_id = sqlite3_column_int64(stmt, 0);
        if (auto* p = sqlite3_column_text(stmt, 1)) row.session_guid = (const char*)p;
        out.push_back(std::move(row));
    }
    sqlite3_finalize(stmt);
    return out;
}

std::vector<InstanceRegistry::ActivePlayerRow>
InstanceRegistry::GetActivePlayersForInstance(int64_t instance_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<ActivePlayerRow> out;
    sqlite3* db = Database::GetConnection();
    if (!db) return out;

    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "SELECT ip.session_guid, ip.profile_id, ip.task_force_number, "
        "       COALESCE(c.user_id, 0) "
        "FROM ga_instance_players ip "
        "LEFT JOIN ga_characters c ON c.id = ip.character_id "
        "WHERE ip.instance_id = ? AND ip.left_at IS NULL";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK || !stmt) {
        Logger::Log("db", "[InstanceRegistry] GetActivePlayersForInstance prepare failed: %s\n",
            sqlite3_errmsg(db));
        return out;
    }
    sqlite3_bind_int64(stmt, 1, instance_id);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ActivePlayerRow row;
        if (auto* p = sqlite3_column_text(stmt, 0)) row.guid = (const char*)p;
        row.profile_id = (uint32_t)sqlite3_column_int(stmt, 1);
        row.task_force = sqlite3_column_int(stmt, 2);
        row.user_id    = sqlite3_column_int64(stmt, 3);
        out.push_back(std::move(row));
    }
    sqlite3_finalize(stmt);
    return out;
}

std::vector<InstanceRegistry::SearchablePlayerRow>
InstanceRegistry::GetActiveSearchablePlayers() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<SearchablePlayerRow> out;
    sqlite3* db = Database::GetConnection();
    if (!db) return out;

    sqlite3_stmt* stmt = nullptr;
    // Name comes from ga_users via the character row — ga_players is an
    // ephemeral session table (wiped wholesale by PlayerRegistry::Init on
    // every game-server boot), kept only as a fallback for character-less rows.
    const char* sql =
        "SELECT ip.session_guid, COALESCE(ip.character_id, 0), "
        "       COALESCE(u.username, p.player_name, ''), COALESCE(c.profile_id, 0), "
        "       i.map_name, COALESCE(i.is_home_map, 0) "
        "FROM ga_instance_players ip "
        "JOIN ga_instances i ON i.instance_id = ip.instance_id AND i.state != 'STOPPED' "
        "LEFT JOIN ga_characters c ON c.id = ip.character_id "
        "LEFT JOIN ga_users u ON u.id = c.user_id "
        "LEFT JOIN ga_players p ON p.session_guid = ip.session_guid "
        "WHERE ip.left_at IS NULL";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK || !stmt) {
        Logger::Log("db", "[InstanceRegistry] GetActiveSearchablePlayers prepare failed: %s\n",
            sqlite3_errmsg(db));
        return out;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        SearchablePlayerRow row;
        if (auto* g = sqlite3_column_text(stmt, 0)) row.session_guid = (const char*)g;
        row.character_id = sqlite3_column_int64(stmt, 1);
        if (auto* n = sqlite3_column_text(stmt, 2)) row.player_name = (const char*)n;
        row.profile_id = (uint32_t)sqlite3_column_int(stmt, 3);
        if (auto* m = sqlite3_column_text(stmt, 4)) row.map_name = (const char*)m;
        row.in_mission = sqlite3_column_int(stmt, 5) == 0;
        out.push_back(std::move(row));
    }
    sqlite3_finalize(stmt);
    return out;
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
    // Empty READY missions are also excluded. Even if they are between the
    // last PLAYER_LEFT and shutdown, fresh players must not join an abandoned
    // mid-game process and skip the normal setup flow.
    int rc = sqlite3_prepare_v2(db,
        "SELECT id, map_name, state, pid, udp_port, ip_address, player_count, "
        "       started_at, COALESCE(sealed_at, 0), "
        "       COALESCE(instance_id, 0), COALESCE(is_home_map, 0), COALESCE(max_players, 0), "
        "       COALESCE(game_mode, ''), "
        "       COALESCE(queue_id, 0), COALESCE(predecessor_instance_id, 0), COALESCE(end_mission_at, 0), "
        "       COALESCE(access_mode, 'OPEN'), COALESCE(owner_party_ids, ''), COALESCE(node_id, 0) "
        "FROM ga_instances "
        "WHERE state = 'READY' AND is_home_map = 0 AND end_mission_at IS NULL "
        "  AND EXISTS ("
        "        SELECT 1 FROM ga_instance_players ip "
        "        WHERE ip.instance_id = ga_instances.instance_id "
        "          AND ip.left_at IS NULL"
        "      )",
        -1, &stmt, nullptr);
    if (rc != SQLITE_OK || !stmt) return results;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        InstanceInfo info = ReadInstanceRow(stmt);
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
        "       COALESCE(queue_id, 0), COALESCE(predecessor_instance_id, 0), COALESCE(end_mission_at, 0), COALESCE(node_id, 0) "
        "FROM ga_instances "
        "WHERE state != 'STOPPED' AND pid IS NOT NULL AND pid > 0",
        -1, &stmt, nullptr);
    if (rc != SQLITE_OK || !stmt) return results;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        InstanceInfo info = ReadInstanceRow(stmt);
        results.push_back(std::move(info));
    }
    sqlite3_finalize(stmt);
    return results;
}

std::optional<InstanceInfo> InstanceRegistry::GetHomeInstance(int site) {
    std::lock_guard<std::mutex> lock(mutex_);
    sqlite3* db = Database::GetConnection();
    if (!db) return std::nullopt;

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db,
        "SELECT id, map_name, state, pid, udp_port, ip_address, player_count, "
        "       started_at, COALESCE(sealed_at, 0), "
        "       COALESCE(instance_id, 0), COALESCE(is_home_map, 0), COALESCE(max_players, 0), "
        "       COALESCE(game_mode, ''), "
        "       COALESCE(queue_id, 0), COALESCE(predecessor_instance_id, 0), COALESCE(end_mission_at, 0), COALESCE(node_id, 0) "
        "FROM ga_instances "
        "WHERE is_home_map = 1 AND state != 'STOPPED' "
        "ORDER BY instance_id",
        -1, &stmt, nullptr);
    if (rc != SQLITE_OK || !stmt) return std::nullopt;

    // Same derivation as GetReadyHomeInstance: site 0 means any, and an exact
    // match is preferred over the master's site.
    std::optional<InstanceInfo> in_site;
    std::optional<InstanceInfo> any;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        InstanceInfo info = ReadInstanceRow(stmt);
        if (!any) any = info;
        if (site == 0) { in_site = info; break; }
        if (SiteOfInstance(info) == site) { in_site = info; break; }
    }
    sqlite3_finalize(stmt);
    return in_site ? in_site : any;
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
        "       COALESCE(queue_id, 0), COALESCE(predecessor_instance_id, 0), COALESCE(end_mission_at, 0), COALESCE(node_id, 0) "
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
        InstanceInfo info = ReadInstanceRow(stmt);
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
    // A mission counts when it is either STARTING (spawned, players not yet
    // registered — the 0->1 flip tells queued players an invitation is seconds
    // away) or READY with at least one active player (an empty READY mission is
    // abandoned, not a live match). DRAFTING successors are intentionally NOT
    // counted (state filter) — they're waiting on their predecessor.
    //
    // Successor de-dup: a chained mission must read as ONE mission even during
    // the brief window where the predecessor is still live and the successor
    // has already spawned. So a successor (predecessor_instance_id set) is
    // suppressed while its predecessor is still an active (STARTING/READY,
    // not-ended) mission; once the predecessor wraps (end_mission_at set →
    // dropped here) the successor — by then promoted to READY — counts in its
    // place, keeping the total at 1 across the handoff.
    if (sqlite3_prepare_v2(db,
            "SELECT COUNT(*) FROM ga_instances i "
            "WHERE i.queue_id = ? AND i.is_home_map = 0 AND i.end_mission_at IS NULL "
            "  AND ( i.state = 'STARTING' "
            // A just-promoted successor is briefly READY with no players yet
            // (they're travelling from the predecessor) — count it anyway so the
            // chain never dips to 0 across the handoff.
            "        OR ( i.state = 'READY' AND ( i.predecessor_instance_id IS NOT NULL "
            "             OR EXISTS ( SELECT 1 FROM ga_instance_players ip "
            "                         WHERE ip.instance_id = i.instance_id AND ip.left_at IS NULL ) ) ) ) "
            "  AND ( i.predecessor_instance_id IS NULL "
            "        OR NOT EXISTS ("
            "             SELECT 1 FROM ga_instances p "
            "             WHERE p.instance_id = i.predecessor_instance_id "
            "               AND p.is_home_map = 0 AND p.end_mission_at IS NULL "
            "               AND p.state IN ('STARTING','READY') ) )",
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
            // PARTY_LOCKED (private) matches are excluded from the player /
            // class counters — a solo can't be matched into them, so they don't
            // describe "who you'd play with". They still count toward
            // instance_count (GetActiveInstanceCountForQueue, unfiltered).
            "SELECT COUNT(*) FROM ga_instance_players ip "
            "JOIN ga_instances i ON i.instance_id = ip.instance_id "
            "WHERE i.queue_id = ? AND i.state != 'STOPPED' AND ip.left_at IS NULL "
            "  AND COALESCE(i.access_mode, 'OPEN') != 'PARTY_LOCKED'",
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
            // PARTY_LOCKED (private) matches excluded — see GetActivePlayerCountForQueue.
            "SELECT ip.profile_id, COUNT(*) FROM ga_instance_players ip "
            "JOIN ga_instances i ON i.instance_id = ip.instance_id "
            "WHERE i.queue_id = ? AND i.state != 'STOPPED' AND ip.left_at IS NULL "
            "  AND ip.profile_id IS NOT NULL "
            "  AND COALESCE(i.access_mode, 'OPEN') != 'PARTY_LOCKED' "
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
