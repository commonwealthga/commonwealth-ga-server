#include "src/ControlServer/Logger.hpp"
#include "src/ControlServer/Database/Database.hpp"
#include "src/ControlServer/PlayerSessionStore/PlayerSessionStore.hpp"
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
    // Defaults
    uint16_t    tcp_port = 9000;
    uint16_t    ipc_port = IpcProtocol::DEFAULT_IPC_PORT;
    std::string db_path  = "server.db";

    // Parse CLI arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--help" || arg == "-h") {
            fprintf(stdout,
                "Usage: control-server [OPTIONS]\n"
                "\n"
                "Options:\n"
                "  --port=N          TCP listen port for GA clients (default: 9000)\n"
                "  --port N          (space-separated form)\n"
                "  --ipc-port=N      IPC listen port for game instances (default: 9010)\n"
                "  --ipc-port N      (space-separated form)\n"
                "  --db-path=PATH    SQLite database file path (default: server.db)\n"
                "  --db-path PATH    (space-separated form)\n"
                "  --help, -h        Print this help and exit\n"
            );
            return 0;
        }
        else if (arg == "--port" && i + 1 < argc) {
            tcp_port = static_cast<uint16_t>(std::atoi(argv[++i]));
        }
        else if (arg.rfind("--port=", 0) == 0) {
            tcp_port = static_cast<uint16_t>(std::atoi(arg.c_str() + 7));
        }
        else if (arg == "--ipc-port" && i + 1 < argc) {
            ipc_port = static_cast<uint16_t>(std::atoi(argv[++i]));
        }
        else if (arg.rfind("--ipc-port=", 0) == 0) {
            ipc_port = static_cast<uint16_t>(std::atoi(arg.c_str() + 11));
        }
        else if (arg == "--db-path" && i + 1 < argc) {
            db_path = argv[++i];
        }
        else if (arg.rfind("--db-path=", 0) == 0) {
            db_path = arg.substr(10);
        }
        else {
            fprintf(stderr, "[control-server] Unknown argument: %s (use --help for usage)\n",
                arg.c_str());
        }
    }

    // Signal handlers for clean shutdown
    std::signal(SIGINT,  on_signal);
    std::signal(SIGTERM, on_signal);

    Logger::Log("main", "Control server v0.0.7 starting...\n");

    // Initialize database
    Database::SetDbPath(db_path);
    Database::Init();

    // Initialize session store
    PlayerSessionStore::Init();

    // Create ASIO io_context and register signal handler reference
    asio::io_context io;
    g_io = &io;

    // Bind TCP listener (GA client connections on port 9000)
    TcpListener listener(io, tcp_port);

    // Bind IPC server (game instance connections on port 9010)
    IpcServer ipc_server(io, ipc_port);

    Logger::Log("main", "Control server ready. TCP port %d, IPC port %d, DB %s\n",
        (int)tcp_port, (int)ipc_port, db_path.c_str());

    // Run event loop -- blocks until io.stop() or all work complete
    io.run();

    Logger::Log("main", "Control server shutting down.\n");
    g_io = nullptr;

    return 0;
}
