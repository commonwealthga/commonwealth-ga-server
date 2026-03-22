#include "src/ControlServer/Logger.hpp"
#include "src/ControlServer/Config/ControlServerConfig.hpp"
#include "src/ControlServer/Database/Database.hpp"
#include "src/ControlServer/PlayerSessionStore/PlayerSessionStore.hpp"
#include "src/ControlServer/InstanceRegistry/InstanceRegistry.hpp"
#include "src/ControlServer/InstanceSpawner/InstanceSpawner.hpp"
#include "src/ControlServer/TcpListener/TcpListener.hpp"
#include "src/ControlServer/IpcServer/IpcServer.hpp"
#include "src/Shared/IpcProtocol.hpp"
#include <asio.hpp>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <csignal>

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
                "  --help, -h        Print this help and exit\n"
            );
            return 0;
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

    // Clear stale instances from any previous (crashed) run
    InstanceRegistry::ClearStaleInstances();

    // Allocate a UDP port for the home map instance
    auto home_port = InstanceRegistry::AllocatePort(cfg.udp_port_range.lo, cfg.udp_port_range.hi);
    if (!home_port) {
        Logger::Log("main", "FATAL: No UDP ports available in range %d-%d\n",
            (int)cfg.udp_port_range.lo, (int)cfg.udp_port_range.hi);
        return 1;
    }

    // Register instance as STARTING in registry (returns instance_id = SQLite rowid)
    int64_t home_instance_id = InstanceRegistry::InsertStarting(
        cfg.home_map_name, *home_port, 0, /*is_home_map=*/true);

    // Spawn the UE3 process via fork/exec with Wine
    pid_t home_pid = InstanceSpawner::Spawn(
        cfg, cfg.home_map_name, cfg.home_map_game_mode, *home_port, home_instance_id);

    if (home_pid < 0) {
        Logger::Log("main", "FATAL: Failed to spawn home map instance\n");
        return 1;
    }

    Logger::Log("main", "Home map instance spawned: instance_id=%lld pid=%d port=%d map=%s\n",
        (long long)home_instance_id, (int)home_pid, (int)*home_port, cfg.home_map_name.c_str());

    // Create ASIO io_context and register signal handler reference
    asio::io_context io;
    g_io = &io;

    // Bind TCP listener (GA client connections)
    TcpListener listener(io, cfg.tcp_port);

    // Bind IPC server (game instance connections)
    IpcServer ipc_server(io, cfg.ipc_port);

    Logger::Log("main", "Control server ready. TCP port %d, IPC port %d, DB %s, home_map=%s\n",
        (int)cfg.tcp_port, (int)cfg.ipc_port, cfg.db_path.c_str(), cfg.home_map_name.c_str());

    // Run event loop -- blocks until io.stop() or all work complete.
    // INSTANCE_READY from the game DLL will call InstanceRegistry::MarkReady() asynchronously.
    io.run();

    Logger::Log("main", "Control server shutting down.\n");
    g_io = nullptr;

    return 0;
}
