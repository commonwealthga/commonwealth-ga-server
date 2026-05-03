#include "src/ControlServer/InstanceSpawner/InstanceSpawner.hpp"
#include "src/ControlServer/Logger.hpp"
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <vector>
#include <string>
#include <cstdlib>

pid_t InstanceSpawner::Spawn(const ControlServerConfig& cfg,
                              const std::string& map_name,
                              const std::string& game_mode,
                              uint16_t udp_port,
                              int64_t instance_id) {
    pid_t pid = fork();

    if (pid < 0) {
        Logger::Log("spawner", "[InstanceSpawner] fork() failed: %s\n", strerror(errno));
        return -1;
    }

    if (pid == 0) {
        // Child process: become its own process group leader so the
        // parent can kill the entire tree (xvfb-run + wine + game)
        // with kill(-pid, SIGKILL).
        setpgid(0, 0);

        // Set Wine environment and exec the game binary.
        setenv("WINEDLLOVERRIDES", cfg.dll_overrides.c_str(), 1);
        setenv("WINEPREFIX", cfg.wine_prefix.c_str(), 1);

        // Build all arg strings as named locals to prevent dangling c_str() pointers.
        std::string map_arg     = map_name + "?Game=" + game_mode;
        std::string host_arg    = "-host=" + cfg.host;
        std::string hostdns_arg = "-hostdns=" + cfg.hostdns;
        std::string port_arg    = "-port=" + std::to_string(udp_port);
        std::string ipc_port_arg  = "-ipcport=" + std::to_string(cfg.ipc_port);
        std::string inst_id_arg   = "-instanceid=" + std::to_string(instance_id);
        std::string gamepath_arg  = "-gamepath=" + cfg.game_binary;
        std::string fixguids_arg  = std::string("-fixpackageguids=") + (cfg.fix_package_guids ? "1" : "0");
        std::string crashdir_arg  = "-crashdir=" + cfg.crash_dir;
        std::string logdir_arg    = "-logdir=" + cfg.log_dir;

        // Join channel allowlists with ',' for transport to the in-process DLL.
        // Empty list -> empty value, which the DLL treats as "no channels".
        auto join_csv = [](const std::vector<std::string>& v) {
            std::string out;
            for (size_t i = 0; i < v.size(); ++i) {
                if (i) out += ',';
                out += v[i];
            }
            return out;
        };
        std::string enabled_channels_arg       = "-enabledchannels="      + join_csv(cfg.enabled_channels);
        std::string enabled_crash_channels_arg = "-enabledcrashchannels=" + join_csv(cfg.enabled_crash_channels);

        // --wine-debug: use winedbg instead of wine, with "--command cont" to auto-resume
        std::string wine_bin = cfg.wine_debug ? cfg.wine_binary + "dbg" : cfg.wine_binary;

        std::vector<const char*> argv = {
            cfg.xvfb_run_path.c_str(), "-a",
            wine_bin.c_str(),
        };
        if (cfg.wine_debug) {
            argv.push_back("--command");
            argv.push_back("cont");
        }
        for (const char* a : {
            cfg.game_binary.c_str(),
            "server",
            map_arg.c_str(),
            "-nodatabase", "-unattended",
            host_arg.c_str(), hostdns_arg.c_str(), port_arg.c_str(),
            "-seekfreeloading", "-tcp=300", "-nullrhi",
            ipc_port_arg.c_str(), inst_id_arg.c_str(), gamepath_arg.c_str(),
            fixguids_arg.c_str(), crashdir_arg.c_str(), logdir_arg.c_str(),
            enabled_channels_arg.c_str(), enabled_crash_channels_arg.c_str(),
        }) {
            argv.push_back(a);
        }
        argv.push_back(nullptr);

        execvp(cfg.xvfb_run_path.c_str(), const_cast<char* const*>(argv.data()));

        // execvp only returns on failure.
        _exit(1);
    }

    // Parent process.
    Logger::Log("spawner", "[InstanceSpawner] Spawned instance_id=%lld pid=%d port=%d map=%s\n",
        (long long)instance_id, (int)pid, (int)udp_port, map_name.c_str());
    return pid;
}
