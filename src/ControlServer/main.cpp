#include "src/ControlServer/Logger.hpp"
#include "src/ControlServer/Config/ControlServerConfig.hpp"
#include "src/ControlServer/Database/Database.hpp"
#include "src/ControlServer/MapGameInfo/MapGameInfo.hpp"
#include "src/ControlServer/PlayerSessionStore/PlayerSessionStore.hpp"
#include "src/ControlServer/InstanceRegistry/InstanceRegistry.hpp"
#include "src/ControlServer/InstanceSpawner/InstanceSpawner.hpp"
#include "src/ControlServer/TcpListener/TcpListener.hpp"
#include "src/ControlServer/ChatListener/ChatListener.hpp"
#include "src/ControlServer/IpcServer/IpcServer.hpp"
#include "src/Shared/IpcProtocol.hpp"
#include "src/ControlServer/TcpSession/TcpSession.hpp"
#include "src/ControlServer/MatchmakingService/MatchmakingService.hpp"
#include <asio.hpp>
#include <thread>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <csignal>
#include <functional>
#include <algorithm>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

// ---------------------------------------------------------------------------
// Signal handling for clean shutdown
// ---------------------------------------------------------------------------

static asio::io_context* g_io = nullptr;

static void on_signal(int /*sig*/) {
    if (g_io) {
        g_io->stop();
    }
}

static bool UpdateQueueField(uint32_t queue_id, const std::string& field,
                              const nlohmann::json& value, std::string& message) {
    // Whitelist: each field has its own type + range gate. Anything not
    // listed here is rejected.
    enum class Kind { Bool, Uint, NonNegUint, NonNegFloat };
    struct FieldSpec { const char* name; Kind kind; };
    static const FieldSpec kSpecs[] = {
        { "enabled",                  Kind::Bool        },
        { "continue_in_queue",        Kind::Bool        },
        { "min_players_to_pop",       Kind::Uint        },  // >= 1
        { "max_players_per_instance", Kind::NonNegUint  },  // >= 0
        { "pop_delay_seconds",        Kind::NonNegFloat },  // >= 0
    };

    const FieldSpec* spec = nullptr;
    for (const auto& s : kSpecs) if (field == s.name) { spec = &s; break; }
    if (!spec) {
        message = "invalid queue field";
        return false;
    }

    int    bind_int    = 0;
    double bind_double = 0.0;
    bool   is_double   = false;
    switch (spec->kind) {
        case Kind::Bool:
            if (!value.is_boolean()) { message = "value must be boolean"; return false; }
            bind_int = value.get<bool>() ? 1 : 0;
            break;
        case Kind::Uint: {
            if (!value.is_number_integer()) { message = "value must be integer"; return false; }
            int64_t v = value.get<int64_t>();
            if (v < 1) { message = std::string(spec->name) + " must be >= 1"; return false; }
            bind_int = (int)v;
            break;
        }
        case Kind::NonNegUint: {
            if (!value.is_number_integer()) { message = "value must be integer"; return false; }
            int64_t v = value.get<int64_t>();
            if (v < 0) { message = std::string(spec->name) + " must be >= 0"; return false; }
            bind_int = (int)v;
            break;
        }
        case Kind::NonNegFloat:
            if (!value.is_number()) { message = "value must be number"; return false; }
            bind_double = value.get<double>();
            if (bind_double < 0.0) { message = std::string(spec->name) + " must be >= 0"; return false; }
            is_double = true;
            break;
    }

    sqlite3* db = Database::GetConnection();
    if (!db) { message = "database is not open"; return false; }

    std::string sql = "UPDATE ga_queues SET " + field + " = ? WHERE queue_id = ?";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK || !stmt) {
        message = sqlite3_errmsg(db);
        return false;
    }
    if (is_double) sqlite3_bind_double(stmt, 1, bind_double);
    else           sqlite3_bind_int(stmt, 1, bind_int);
    sqlite3_bind_int(stmt, 2, (int)queue_id);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) { message = sqlite3_errmsg(db); return false; }
    if (sqlite3_changes(db) == 0) { message = "queue not found"; return false; }

    if (is_double) {
        Logger::Log("matchmaking",
            "[Matchmaking] Admin update queue=%u field=%s value=%.2f → reloading\n",
            queue_id, field.c_str(), bind_double);
    } else {
        Logger::Log("matchmaking",
            "[Matchmaking] Admin update queue=%u field=%s value=%d → reloading\n",
            queue_id, field.c_str(), bind_int);
    }

    MatchmakingService::ReloadQueues();
    message = "queue updated and reloaded";
    return true;
}

static bool UpdatePoolEntry(uint32_t map_pool_id, const std::string& map_name,
                            const std::string& game_mode, const nlohmann::json& payload,
                            std::string& message) {
    if (map_name.empty() || game_mode.empty()) {
        message = "missing map_name or game_mode";
        return false;
    }

    sqlite3* db = Database::GetConnection();
    if (!db) {
        message = "database is not open";
        return false;
    }

    // Dynamic SET-clause builder: each entry is a SQL fragment + a bind
    // closure. Avoids the combinatorial explosion of optional-field
    // permutations. "= NULL" fragments use a no-op bind closure.
    struct SetClause {
        std::string sql_fragment;
        std::function<void(sqlite3_stmt*, int)> bind;
    };
    std::vector<SetClause> clauses;

    if (payload.contains("enabled") && payload["enabled"].is_boolean()) {
        const bool v = payload["enabled"].get<bool>();
        clauses.push_back({ "enabled = ?",
            [v](sqlite3_stmt* s, int i){ sqlite3_bind_int(s, i, v ? 1 : 0); } });
    }
    if (payload.contains("weight") && payload["weight"].is_number_integer()) {
        const int w = std::max(1, payload["weight"].get<int>());
        clauses.push_back({ "weight = ?",
            [w](sqlite3_stmt* s, int i){ sqlite3_bind_int(s, i, w); } });
    }
    // min_players / max_players: empty payload-value is impossible (caller
    // wouldn't include the key); null OR 0 → SQL NULL; positive integer
    // → that value; anything else rejected.
    for (const char* col : { "min_players", "max_players" }) {
        if (!payload.contains(col)) continue;
        const auto& v = payload[col];
        bool to_null = v.is_null() || (v.is_number_integer() && v.get<int>() == 0);
        if (to_null) {
            clauses.push_back({ std::string(col) + " = NULL",
                [](sqlite3_stmt*, int){} });
        } else if (v.is_number_integer() && v.get<int>() > 0) {
            const int n = v.get<int>();
            clauses.push_back({ std::string(col) + " = ?",
                [n](sqlite3_stmt* s, int i){ sqlite3_bind_int(s, i, n); } });
        } else {
            message = std::string(col) + " must be a non-negative integer or null";
            return false;
        }
    }

    if (clauses.empty()) {
        message = "no editable fields in payload";
        return false;
    }

    std::string sql = "UPDATE ga_map_pool_entries SET ";
    for (size_t i = 0; i < clauses.size(); ++i) {
        if (i > 0) sql += ", ";
        sql += clauses[i].sql_fragment;
    }
    sql += " WHERE map_pool_id = ? AND map_name = ? AND game_mode = ?";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK || !stmt) {
        message = sqlite3_errmsg(db);
        return false;
    }

    int col = 1;
    for (const auto& c : clauses) {
        c.bind(stmt, col);  // no-op for "= NULL" fragments
        if (c.sql_fragment.find('?') != std::string::npos) col++;
    }
    sqlite3_bind_int(stmt, col++, (int)map_pool_id);
    sqlite3_bind_text(stmt, col++, map_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, col++, game_mode.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) { message = sqlite3_errmsg(db); return false; }
    if (sqlite3_changes(db) == 0) { message = "pool entry not found"; return false; }

    // Summary log: what was set, comma-joined.
    std::string summary;
    if (payload.contains("enabled") && payload["enabled"].is_boolean()) {
        if (!summary.empty()) summary += ",";
        summary += std::string("enabled=") + (payload["enabled"].get<bool>() ? "1" : "0");
    }
    if (payload.contains("weight") && payload["weight"].is_number_integer()) {
        if (!summary.empty()) summary += ",";
        summary += "weight=" + std::to_string(std::max(1, payload["weight"].get<int>()));
    }
    for (const char* k : { "min_players", "max_players" }) {
        if (!payload.contains(k)) continue;
        const auto& v = payload[k];
        if (!summary.empty()) summary += ",";
        summary += std::string(k) + "=";
        if (v.is_null() || (v.is_number_integer() && v.get<int>() == 0)) {
            summary += "NULL";
        } else {
            summary += std::to_string(v.get<int>());
        }
    }
    Logger::Log("matchmaking",
        "[Matchmaking] Admin update pool_entry=%u:%s:%s set=%s → reloading\n",
        map_pool_id, map_name.c_str(), game_mode.c_str(), summary.c_str());

    MatchmakingService::ReloadQueues();
    message = "pool entry updated and queues reloaded";
    return true;
}

static bool SpawnAdminInstance(const ControlServerConfig& cfg, const std::string& map_name,
                               const std::string& game_mode, int max_players,
                               std::string& message) {
    if (map_name.empty() || game_mode.empty()) {
        message = "missing map_name or game_mode";
        return false;
    }
    if (max_players <= 0 || max_players > 1000) {
        message = "max_players must be between 1 and 1000";
        return false;
    }

    auto port = InstanceRegistry::AllocatePort(cfg.udp_port_range.lo, cfg.udp_port_range.hi);
    if (!port) {
        message = "no UDP ports available";
        return false;
    }

    int64_t instance_id = InstanceRegistry::InsertStarting(
        map_name, game_mode, *port, 0, /*is_home_map=*/false);
    InstanceRegistry::SetMaxPlayers(instance_id, max_players);

    pid_t pid = InstanceSpawner::Spawn(cfg, map_name, game_mode, *port, instance_id);
    if (pid < 0) {
        InstanceRegistry::MarkStopped(instance_id);
        message = "InstanceSpawner::Spawn failed";
        return false;
    }

    InstanceRegistry::UpdatePid(instance_id, pid);
    Logger::Log("main", "Admin action: created instance %lld pid=%d port=%d map=%s mode=%s max_players=%d\n",
        (long long)instance_id, (int)pid, (int)*port, map_name.c_str(), game_mode.c_str(),
        max_players);
    message = "instance created: " + std::to_string(instance_id);
    return true;
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    SetConsoleOutputCP(CP_UTF8);
#endif

    std::string config_path = "control-server.json";
    bool wine_debug = false;

    // Parse CLI arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            fprintf(stdout,
                "Usage: control-server [OPTIONS]\n"
                "\n"
                "Options:\n"
                "  --config=PATH     Path to JSON config file (default: control-server.json)\n"
                "  --config PATH     (space-separated form)\n"
                "  --wine-debug      Use winedbg with --command cont (prints crash traces)\n"
                "  --help, -h        Print this help and exit\n"
            );
            return 0;
        }
        else if (arg == "--wine-debug") {
            wine_debug = true;
        }
        else if (arg == "--config" && i + 1 < argc) {
            config_path = argv[++i];
        }
        else if (arg.rfind("--config=", 0) == 0) {
            config_path = arg.substr(9);
        }
        else {
            fprintf(stderr, "[control-server] Unknown argument: %s (use --help for usage)\n",
                arg.c_str());
        }
    }

    // Load config from JSON file (falls back to defaults if file is absent).
    ControlServerConfig cfg = ControlServerConfig::Load(config_path);
    cfg.wine_debug = wine_debug;

    // Signal handlers for clean shutdown
    std::signal(SIGINT,  on_signal);
    std::signal(SIGTERM, on_signal);
#ifndef _WIN32
    std::signal(SIGCHLD, SIG_IGN);  // Auto-reap child zombies (Linux)
#endif

    Logger::Log("main", "Control server v0.0.9 starting...\n");

    // Initialize database
    Database::SetDbPath(cfg.db_path);
    Database::Init();

    // Preload the map_game_info registry from the shared DB. Safe to call
    // even if the table doesn't exist yet (control server may start before
    // any game-server has applied schema v51) — lookups just miss and
    // send_go_play_to_instance falls back to its hardcoded HQ defaults.
    MapGameInfo::Init();

    // Initialize session store
    PlayerSessionStore::Init();

    // Initialize instance registry
    InstanceRegistry::Init();
    InstanceRegistry::SetHost(cfg.host);

    // Set network config for TcpSession responses (external IP + chat port)
    TcpSession::SetNetworkConfig(cfg.host, cfg.chat_port);

    // Clear stale instances from any previous (crashed) run
    InstanceRegistry::ClearStaleInstances();

    // Provision per-slot WINEPREFIXes if enabled. Idempotent — only clones
    // missing prefixes. Must run before any Spawn() so the first match
    // doesn't pay clone latency.
    if (!InstanceSpawner::EnsureSlotPrefixes(cfg)) {
        Logger::Log("main", "EnsureSlotPrefixes failed — aborting startup\n");
        return 1;
    }

    // Initialize matchmaking. Init() reads ga_queues + ga_map_pool_entries from
    // the shared DB and constructs one MatchmakingService::Queue per enabled
    // row via RuleFactory. Edit queues without rebuilding by changing those
    // tables — see Database.cpp migrations + the `-reload-queues` chat command
    // for hot-reload semantics.
    //
    // Map pool note: maps that need to be downloaded by the client to work
    // (e.g. 3P_Beachhead2_P, 3P_Beachhead_P) belong in ga_map_pool_entries with
    // enabled=0 until the client downloads them.
    MatchmakingService::Init();
    // Provide queue-scoped running-instance info to matchmaking rules. Rules
    // iterate this list to find a seat-available instance regardless of map —
    // because pool-randomized queues can have instances on different maps,
    // map equality is no longer the join criterion. Filter is done in-memory
    // (small N — only READY mission instances live).
    MatchmakingService::SetInstanceProvider([](uint32_t queue_id) -> std::vector<RunningInstance> {
        auto all = InstanceRegistry::GetReadyMissionInstances();
        std::vector<RunningInstance> filtered;
        filtered.reserve(all.size());
        for (const auto& inst : all) {
            if (inst.queue_id != queue_id) continue;
            RunningInstance ri;
            ri.instance_id  = inst.instance_id;
            ri.map_name     = inst.map_name;
            ri.game_mode    = inst.game_mode;
            ri.player_count = inst.player_count;
            ri.max_players  = inst.max_players;
            ri.queue_id     = inst.queue_id;
            filtered.push_back(std::move(ri));
        }
        return filtered;
    });

    // Handle match pop: spawn instance or route to existing one
    MatchmakingService::SetMatchPopCallback(
        [&cfg](uint32_t queue_id, MatchResult result) {
            if (result.existing_instance_id) {
                // Route to existing instance — send invitation immediately
                Logger::Log("matchmaking",
                    "[Matchmaking] Routing %zu players to existing instance %lld\n",
                    result.session_guids.size(), (long long)*result.existing_instance_id);
                for (const auto& guid : result.session_guids) {
                    int tf = 1;
                    auto it = result.task_force_assignments.find(guid);
                    if (it != result.task_force_assignments.end()) tf = it->second;
                    TcpSession::DeliverMatchInvitation(guid, *result.existing_instance_id, result.game_mode, tf);
                }
                return;
            }

            // Spawn new instance
            auto port = InstanceRegistry::AllocatePort(
                cfg.udp_port_range.lo, cfg.udp_port_range.hi);
            if (!port) {
                Logger::Log("matchmaking",
                    "[Matchmaking] No UDP ports available for match spawn\n");
                return;
            }

            int64_t instance_id = InstanceRegistry::InsertStarting(
                result.map_name, result.game_mode, *port, 0, /*is_home_map=*/false,
                /*queue_id=*/queue_id);

            // Register pending match BEFORE Spawn so the unwind path on a
            // spawn-failure (and the symmetric IPC-disconnect-before-READY
            // path in IpcServer) can call DiscardPendingMatchForDeadInstance
            // to unstick the matched players. Without this, a process that
            // fails to fork leaves its session_guids out of queue.players
            // with no way back.
            PendingMatch pending;
            pending.instance_id = instance_id;
            pending.queue_id = queue_id;
            pending.game_mode = result.game_mode;
            pending.session_guids = std::move(result.session_guids);
            pending.task_force_assignments = std::move(result.task_force_assignments);
            pending.profile_ids = std::move(result.profile_ids);
            if (auto qcfg = MatchmakingService::GetQueueConfig(queue_id)) {
                pending.cap = qcfg->max_players_per_instance;
            }
            MatchmakingService::AddPendingMatch(instance_id, std::move(pending));

            // Look up queue's configured difficulty so the instance runs
            // at the queue's tier instead of the DLL's map-name heuristic.
            uint32_t queue_difficulty = 0;
            if (auto qcfg = MatchmakingService::GetQueueConfig(queue_id)) {
                queue_difficulty = qcfg->difficulty_value_id;
            }

            pid_t pid = InstanceSpawner::Spawn(
                cfg, result.map_name, result.game_mode, *port, instance_id,
                queue_difficulty);

            if (pid < 0) {
                Logger::Log("matchmaking",
                    "[Matchmaking] Failed to spawn instance for queue %u\n", queue_id);
                MatchmakingService::DiscardPendingMatchForDeadInstance(
                    instance_id, "InstanceSpawner::Spawn returned -1");
                InstanceRegistry::MarkStopped(instance_id);
                return;
            }

            InstanceRegistry::UpdatePid(instance_id, pid);

            Logger::Log("matchmaking",
                "[Matchmaking] Spawned instance %lld (pid=%d port=%d) for queue %u\n",
                (long long)instance_id, (int)pid, (int)*port, queue_id);
        }
    );

    // Successor spawner — invoked from IpcServer's MSG_REQUEST_SUCCESSOR
    // handler after dedupe. Picks a fresh (map, game_mode) at random from
    // the parent's queue's map_pool, so successors don't necessarily
    // inherit the parent's map — that's the whole point of pool-based
    // randomization. Binds the new row via predecessor_instance_id so
    // MarkReady lands it in DRAFTING (not READY); MSG_MISSION_ENDED on the
    // parent promotes DRAFTING → READY.
    IpcServer::SetSuccessorSpawner([&cfg](int64_t parent_instance_id) {
        auto parent = InstanceRegistry::GetInstanceById(parent_instance_id);
        if (!parent) {
            Logger::Log("main", "[SuccessorSpawner] parent_instance_id=%lld not found — aborting\n",
                (long long)parent_instance_id);
            return;
        }
        if (parent->queue_id == 0) {
            Logger::Log("main", "[SuccessorSpawner] parent_instance_id=%lld has no queue_id — refusing to spawn (continuation requires a queue context)\n",
                (long long)parent_instance_id);
            return;
        }
        // The DLL fires MSG_REQUEST_SUCCESSOR from any mission whose mode/score
        // condition trips — it doesn't know whether its queue is opted into
        // continuation. Gate here so non-continuing queues (e.g. SpecOps)
        // don't burn a slot on a successor nobody will ever join.
        if (!MatchmakingService::GetContinueInQueue(parent->queue_id)) {
            Logger::Log("main", "[SuccessorSpawner] Queue %u is not continue_in_queue — dropping successor request for parent=%lld\n",
                parent->queue_id, (long long)parent_instance_id);
            return;
        }
        const int parent_count =
            (int)InstanceRegistry::GetActivePlayersForInstance(parent_instance_id).size();
        auto picked = MatchmakingService::PickRandomMapPoolEntryForCount(
            parent->queue_id, parent_count);
        if (!picked) {
            Logger::Log("main", "[SuccessorSpawner] Queue %u has no map_pool configured — refusing successor for parent=%lld\n",
                parent->queue_id, (long long)parent_instance_id);
            return;
        }
        auto port = InstanceRegistry::AllocatePort(cfg.udp_port_range.lo, cfg.udp_port_range.hi);
        if (!port) {
            Logger::Log("main", "[SuccessorSpawner] No UDP ports available for successor of %lld\n",
                (long long)parent_instance_id);
            return;
        }
        int64_t instance_id = InstanceRegistry::InsertStarting(
            picked->map_name, picked->game_mode, *port, 0, /*is_home_map=*/false,
            /*queue_id=*/parent->queue_id,
            /*predecessor_instance_id=*/parent_instance_id);
        // Successor inherits parent queue's difficulty_value_id.
        uint32_t queue_difficulty = 0;
        if (auto qcfg = MatchmakingService::GetQueueConfig(parent->queue_id)) {
            queue_difficulty = qcfg->difficulty_value_id;
        }
        pid_t pid = InstanceSpawner::Spawn(
            cfg, picked->map_name, picked->game_mode, *port, instance_id,
            queue_difficulty);
        if (pid < 0) {
            Logger::Log("main", "[SuccessorSpawner] Failed to spawn successor for parent=%lld\n",
                (long long)parent_instance_id);
            InstanceRegistry::MarkStopped(instance_id);
            return;
        }
        InstanceRegistry::UpdatePid(instance_id, pid);
        Logger::Log("main", "[SuccessorSpawner] Spawned successor %lld (pid=%d port=%d map=%s mode=%s) for parent=%lld queue=%u — will land in DRAFTING on INSTANCE_READY\n",
            (long long)instance_id, (int)pid, (int)*port,
            picked->map_name.c_str(), picked->game_mode.c_str(),
            (long long)parent_instance_id, parent->queue_id);
    });

    // Home map spawns on demand when the first player selects a character.
    TcpSession::SetHomeMapSpawner([&cfg]() {
        // Double-check: is there already a non-STOPPED home instance?
        auto existing = InstanceRegistry::GetHomeInstance();
        if (existing) {
            Logger::Log("main", "Home map already exists (instance_id=%lld state=%s) — skipping spawn\n",
                (long long)existing->instance_id, existing->state.c_str());
            return;
        }

        auto port = InstanceRegistry::AllocatePort(cfg.udp_port_range.lo, cfg.udp_port_range.hi);
        if (!port) {
            Logger::Log("main", "No UDP ports available for home map spawn\n");
            return;
        }

        int64_t instance_id = InstanceRegistry::InsertStarting(
            cfg.home_map_name, cfg.home_map_game_mode, *port, 0, /*is_home_map=*/true);

        pid_t pid = InstanceSpawner::Spawn(cfg, cfg.home_map_name, cfg.home_map_game_mode, *port, instance_id);
        if (pid < 0) {
            Logger::Log("main", "Failed to spawn home map instance\n");
            InstanceRegistry::MarkStopped(instance_id);
            return;
        }

        InstanceRegistry::UpdatePid(instance_id, pid);

        Logger::Log("main", "Home map instance spawned on demand: instance_id=%lld pid=%d port=%d\n",
            (long long)instance_id, (int)pid, (int)*port);
    });

    // Create ASIO io_context and register signal handler reference
    asio::io_context io;
    g_io = &io;

    // Hand the io_context to MatchmakingService so it can schedule
    // pop-delay timers. Must happen BEFORE any TCP/chat listener starts
    // (those can trigger AddPlayer which may start a delay timer).
    MatchmakingService::SetIoContext(&io);

    Logger::Log("main", "Binding TCP listener on port %d\n", (int)cfg.tcp_port);
    // Bind TCP listener (GA client connections)
    TcpListener listener(io, cfg.tcp_port);
    if (!listener.IsListening()) {
        Logger::Log("main", "TCP listener failed; aborting startup\n");
        return 1;
    }

    Logger::Log("main", "Binding chat listener on port %d\n", (int)cfg.chat_port);
    // Bind chat listener (GA chat connections — separate port)
    ChatListener chat_listener(io, cfg.chat_port);
    if (!chat_listener.IsListening()) {
        Logger::Log("main", "Chat listener failed; aborting startup\n");
        return 1;
    }

    Logger::Log("main", "Binding IPC listener on port %d\n", (int)cfg.ipc_port);
    // Bind IPC server (game instance connections)
    IpcServer ipc_server(io, cfg.ipc_port);
    if (!ipc_server.IsListening()) {
        Logger::Log("main", "IPC listener failed; aborting startup\n");
        return 1;
    }
    Logger::Log("main", "All listeners bound\n");
    IpcServer::SetAdminToken(cfg.admin_token);
    IpcServer::SetAdminActionHandler([&cfg](const std::string& subtype,
                                            const nlohmann::json& payload,
                                            std::string& message) -> bool {
        if (subtype == "stop-instance") {
            int64_t instance_id = payload.value("instance_id", (int64_t)0);
            if (instance_id == 0) {
                message = "missing instance_id";
                return false;
            }

            auto inst = InstanceRegistry::GetInstanceById(instance_id);
            if (!inst) {
                message = "instance not found";
                return false;
            }
            if (inst->state == "STOPPED") {
                message = "instance is already stopped";
                return false;
            }

            Logger::Log("main", "Admin action: stopping instance %lld (pid=%d, map=%s)\n",
                (long long)inst->instance_id, inst->pid, inst->map_name.c_str());
            InstanceSpawner::StopInstanceProcess(*inst, "admin stop-instance");
            InstanceRegistry::MarkStopped(inst->instance_id);
            message = "instance stop requested";
            return true;
        }

        if (subtype == "move-players") {
            int64_t target_instance_id = payload.value("target_instance_id", (int64_t)0);
            int target_task_force = payload.value("target_task_force", 0);
            const auto players = payload.value("players", nlohmann::json::array());
            if (target_instance_id == 0) {
                message = "missing target_instance_id";
                return false;
            }
            if (target_task_force != 1 && target_task_force != 2) {
                message = "target_task_force must be 1 or 2";
                return false;
            }
            if (!players.is_array() || players.empty()) {
                message = "missing players";
                return false;
            }

            int requested = 0;
            int skipped = 0;
            int failed = 0;
            for (const auto& player : players) {
                std::string guid = player.value("session_guid", "");
                if (guid.empty()) {
                    ++skipped;
                    continue;
                }
                int64_t source_instance_id = player.value("source_instance_id", (int64_t)0);
                int source_task_force = player.value("source_task_force", 0);
                std::string player_message;
                if (TcpSession::AdminMovePlayerToInstance(guid, target_instance_id,
                        target_task_force, source_instance_id, source_task_force,
                        player_message)) {
                    ++requested;
                } else {
                    ++failed;
                    Logger::Log("main", "Admin move-players failed for %s: %s\n",
                        guid.c_str(), player_message.c_str());
                }
            }

            message = "move requested=" + std::to_string(requested)
                + " failed=" + std::to_string(failed)
                + " skipped=" + std::to_string(skipped);
            return requested > 0 && failed == 0;
        }

        if (subtype == "reload-queues") {
            MatchmakingService::ReloadQueues();
            message = "queues reloaded";
            return true;
        }

        if (subtype == "update-queue") {
            uint32_t queue_id = payload.value("queue_id", 0u);
            std::string field = payload.value("field", "");
            if (queue_id == 0) {
                message = "missing queue_id";
                return false;
            }
            if (!payload.contains("value")) {
                message = "missing value";
                return false;
            }
            return UpdateQueueField(queue_id, field, payload["value"], message);
        }

        if (subtype == "update-pool-entry") {
            uint32_t map_pool_id = payload.value("map_pool_id", 0u);
            if (map_pool_id == 0) {
                message = "missing map_pool_id";
                return false;
            }
            return UpdatePoolEntry(map_pool_id,
                payload.value("map_name", std::string()),
                payload.value("game_mode", std::string()),
                payload,
                message);
        }

        if (subtype == "create-instance") {
            return SpawnAdminInstance(cfg,
                payload.value("map_name", std::string()),
                payload.value("game_mode", std::string()),
                payload.value("max_players", 0),
                message);
        }

        message = "unknown admin action subtype";
        return false;
    });

    // Periodic idle instance cleanup (every 60 seconds, kills instances empty for 5+ minutes)
    auto idle_timer = std::make_shared<asio::steady_timer>(io);
    std::function<void()> schedule_idle_check;
    schedule_idle_check = [&io, &idle_timer, &schedule_idle_check]() {
        idle_timer->expires_after(std::chrono::seconds(60));
        idle_timer->async_wait([&io, &idle_timer, &schedule_idle_check](std::error_code ec) {
            if (ec) return;  // timer cancelled (shutdown)

            auto idle = InstanceRegistry::GetIdleInstances(300);  // 5 minutes
            for (const auto& inst : idle) {
                Logger::Log("main", "Idle cleanup: killing instance %lld (pid=%d, map=%s) — empty for 5+ minutes\n",
                    (long long)inst.instance_id, inst.pid, inst.map_name.c_str());
                InstanceSpawner::StopInstanceProcess(inst, "idle cleanup");
                InstanceRegistry::MarkStopped(inst.instance_id);
            }

            schedule_idle_check();
        });
    };
    schedule_idle_check();

    Logger::Log("main", "Control server ready. TCP port %d, Chat port %d, IPC port %d, DB %s, home_map=%s\n",
        (int)cfg.tcp_port, (int)cfg.chat_port, (int)cfg.ipc_port, cfg.db_path.c_str(), cfg.home_map_name.c_str());

    // Run event loop -- blocks until io.stop() or all work complete.
    // INSTANCE_READY from the game DLL will call InstanceRegistry::MarkReady() asynchronously.
    io.run();

    Logger::Log("main", "Control server shutting down — terminating spawned instances.\n");

    // Stop every spawned game-instance before exit. InstanceSpawner owns
    // platform-specific process teardown: Linux uses process groups; Windows
    // terminates the tracked child process handle by PID.
    {
        auto running = InstanceRegistry::GetAllRunningInstances();
        Logger::Log("main", "  %zu running instance(s) to terminate\n", running.size());
        for (const auto& inst : running) {
            InstanceSpawner::StopInstanceProcess(inst, "control-server shutdown", 2);
            InstanceRegistry::MarkStopped(inst.instance_id);
        }
    }

    Logger::Log("main", "Control server shutdown complete.\n");
    g_io = nullptr;

    return 0;
}
