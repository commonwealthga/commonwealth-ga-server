#include "src/ControlServer/Logger.hpp"
#include "src/ControlServer/Config/ControlServerConfig.hpp"
#include "src/ControlServer/Database/Database.hpp"
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

    // Initialize session store
    PlayerSessionStore::Init();

    // Initialize instance registry
    InstanceRegistry::Init();
    InstanceRegistry::SetHost(cfg.host);

    // Set network config for TcpSession responses (external IP + chat port)
    TcpSession::SetNetworkConfig(cfg.host, cfg.chat_port);

    // Clear stale instances from any previous (crashed) run
    InstanceRegistry::ClearStaleInstances();

    // Initialize matchmaking
    MatchmakingService::Init();

    // Register queues
	MatchmakingService::RegisterQueue(1, std::make_unique<SimpleSpecOpsMatchRule>());
	MatchmakingService::RegisterQueue(2, std::make_unique<SimplePvPMatchRule>());

    // Provide running instance info to matchmaking rules
    MatchmakingService::SetInstanceProvider([]() -> std::vector<RunningInstance> {
        // For now, return empty — rules that need instance info will
        // query InstanceRegistry directly in their own implementations later.
        // This is the hook point for that data.
        return {};
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
                result.map_name, result.game_mode, *port, 0, /*is_home_map=*/false);

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

    Logger::Log("main", "Control server shutting down.\n");
    g_io = nullptr;

    return 0;
}
