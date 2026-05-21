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
#include "src/ControlServer/MatchmakingService/Rules/SimplePvPMatchRule.hpp"
#include "src/ControlServer/MatchmakingService/Rules/SimpleSpecOpsMatchRule.hpp"
#include <asio.hpp>
#include <thread>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <csignal>
#include <functional>

// ---------------------------------------------------------------------------
// Signal handling for clean shutdown
// ---------------------------------------------------------------------------

static asio::io_context* g_io = nullptr;

static void on_signal(int /*sig*/) {
    if (g_io) {
        g_io->stop();
    }
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(int argc, char* argv[]) {
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
    std::signal(SIGCHLD, SIG_IGN);  // Auto-reap child zombies (Linux)

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

    // Initialize matchmaking
    MatchmakingService::Init();

    // Register queues. Each queue carries a map_pool — every fresh spawn
    // (initial match-pop OR pre-warmed successor) picks one entry at random,
    // so a queue can rotate maps/modes without code changes. Add entries
    // here to expand a queue's rotation. Single-entry pools effectively
    // disable randomization.
    //
    // PvP (2) also opts into continue_in_queue: the post-mission "End
    // Mission" button routes survivors into the queue's pre-warmed successor
    // instead of home.
    {
        QueueOptions specops_opts;
        specops_opts.map_pool = {
            { "1P_CPLab05_P", "TgGame.TgGame_Mission" },
        };
        MatchmakingService::RegisterQueue(1, std::make_unique<SimpleSpecOpsMatchRule>(),
                                          std::move(specops_opts));

        QueueOptions pvp_opts;
        pvp_opts.continue_in_queue = false;
        pvp_opts.map_pool = {
            { "Rot_Redistribution05", "TgGame.TgGame_PointRotation" },
            { "Rot_Redistribution04", "TgGame.TgGame_PointRotation" },
            { "Rot_Redistribution03", "TgGame.TgGame_PointRotation" },
            { "Rot_Trafalgar_P", "TgGame.TgGame_PointRotation" },
            { "Rot_BlackwaterLoch_P", "TgGame.TgGame_PointRotation" },

            // { "HEX_AVA_2pt_Theft_Lab1", "TgGame.TgGame_Escort" },

            { "Push_Toxicity", "TgGame.TgGame_Escort" }, // done
            { "push_Ravine_P", "TgGame.TgGame_Escort" }, // done
            { "Push_Dust_P", "TgGame.TgGame_Escort" }, // crashes?
            { "Push_IceFloe3_P", "TgGame.TgGame_Escort" }, // done

            { "3P_Him_Arena_P", "TgGame.TgGame_Mission" }, // done
            { "Climate_Control_P", "TgGame.TgGame_Mission" }, // done
            { "3P_Climate_Control3_P", "TgGame.TgGame_Mission" }, // done
            { "3P_VolcanoAssault_P", "TgGame.TgGame_Mission" }, // done
            { "Ice_GorgeA01_v2", "TgGame.TgGame_Mission" }, // done

			{"Ticket_Datafarm_P",      "TgGame.TgGame_Ticket"},
			{"Ticket_Datafarm2",       "TgGame.TgGame_Ticket"},
			{"Ticket_Datafarm3",       "TgGame.TgGame_Ticket"},
			{"SeaSide_Ticket_P",       "TgGame.TgGame_Ticket"},
			{"SeaSide_Ticket2_P",      "TgGame.TgGame_Ticket"},
			{"SeaSide_Ticket3",        "TgGame.TgGame_Ticket"},
            {"Ticket_Volcano_P",       "TgGame.TgGame_Ticket"},

			// Rot_Redistribution03 // done
			// Rot_Redistribution04 // done
			// Rot_Redistribution05 // done
			// Rot_Trafalgar_P // done
			// Rot_BlackwaterLoch_P // done

			// HEX_AVA_Push_Factory1_P // done
			// push_Ravine_P // done
			// Push_Dust_P // done

			// Climate_Control_P // done
			// 3P_Climate_Control3_P done
			// 3P_VolcanoAssault_P // done
			// Ice_GorgeA01_v2 // done
        };
        MatchmakingService::RegisterQueue(2, std::make_unique<SimplePvPMatchRule>(),
                                          std::move(pvp_opts));
    }

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

            pid_t pid = InstanceSpawner::Spawn(
                cfg, result.map_name, result.game_mode, *port, instance_id);

            if (pid < 0) {
                Logger::Log("matchmaking",
                    "[Matchmaking] Failed to spawn instance for queue %u\n", queue_id);
                InstanceRegistry::MarkStopped(instance_id);
                return;
            }

            InstanceRegistry::UpdatePid(instance_id, pid);

            Logger::Log("matchmaking",
                "[Matchmaking] Spawned instance %lld (pid=%d port=%d) for queue %u\n",
                (long long)instance_id, (int)pid, (int)*port, queue_id);

            // Store pending match — will be consumed when INSTANCE_READY arrives
            PendingMatch pending;
            pending.instance_id = instance_id;
            pending.queue_id = queue_id;
            pending.game_mode = result.game_mode;
            pending.session_guids = std::move(result.session_guids);
            pending.task_force_assignments = std::move(result.task_force_assignments);
            MatchmakingService::AddPendingMatch(instance_id, std::move(pending));
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
        if (!MatchmakingService::GetQueueOptions(parent->queue_id).continue_in_queue) {
            Logger::Log("main", "[SuccessorSpawner] Queue %u is not continue_in_queue — dropping successor request for parent=%lld\n",
                parent->queue_id, (long long)parent_instance_id);
            return;
        }
        auto picked = MatchmakingService::PickRandomMapPoolEntry(parent->queue_id);
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
        pid_t pid = InstanceSpawner::Spawn(
            cfg, picked->map_name, picked->game_mode, *port, instance_id);
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

    // Bind TCP listener (GA client connections)
    TcpListener listener(io, cfg.tcp_port);

    // Bind chat listener (GA chat connections — separate port)
    ChatListener chat_listener(io, cfg.chat_port);

    // Bind IPC server (game instance connections)
    IpcServer ipc_server(io, cfg.ipc_port);

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
                if (inst.pid > 0) {
                    // Kill the entire process group (xvfb-run + wine + game).
                    // SIGTERM first for graceful shutdown, then SIGKILL to ensure cleanup.
                    kill(-inst.pid, SIGTERM);
                    // Give it a moment then force-kill if still alive.
                    std::thread([pid = inst.pid]() {
                        sleep(3);
                        if (kill(-pid, 0) == 0) {
                            kill(-pid, SIGKILL);
                        }
                    }).detach();
                }
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

    // Kill every spawned game-instance process group BEFORE we exit so
    // they don't get orphaned into the systemd cgroup. winedevice.exe
    // ignores SIGTERM (it's a Wine-internal helper that doesn't react
    // to Unix signals the way native processes do), and systemd's
    // `KillMode=control-group` would then sit in stop-sigterm for
    // TimeoutStopSec (typically 90s) before escalating to SIGKILL.
    // We do SIGTERM-then-wait-then-SIGKILL on each pgid, mirroring the
    // idle-cleanup path. Negative pid = kill the process group, which
    // catches xvfb-run + Xvfb + wine + winedevice + the game binary.
    {
        auto running = InstanceRegistry::GetAllRunningInstances();
        Logger::Log("main", "  %zu running instance(s) to terminate\n", running.size());
        for (const auto& inst : running) {
            if (inst.pid > 0) {
                Logger::Log("main", "  SIGTERM pgid=%d (instance_id=%lld map=%s)\n",
                    inst.pid, (long long)inst.instance_id, inst.map_name.c_str());
                kill(-inst.pid, SIGTERM);
            }
        }
        // Brief grace period for clean shutdown, then SIGKILL stragglers.
        sleep(2);
        for (const auto& inst : running) {
            if (inst.pid > 0 && kill(-inst.pid, 0) == 0) {
                Logger::Log("main", "  SIGKILL pgid=%d (didn't exit on SIGTERM)\n", inst.pid);
                kill(-inst.pid, SIGKILL);
            }
            InstanceRegistry::MarkStopped(inst.instance_id);
        }
    }

    Logger::Log("main", "Control server shutdown complete.\n");
    g_io = nullptr;

    return 0;
}
