// _GNU_SOURCE is required for cpu_set_t / CPU_SET / sched_setaffinity
// from <sched.h> under glibc when compiling with -std=c++17 (which
// otherwise hides the GNU extensions).
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "src/ControlServer/InstanceSpawner/InstanceSpawner.hpp"
#include "src/ControlServer/Logger.hpp"
#include <unistd.h>
#include <sys/stat.h>
#include <cstring>
#include <cerrno>
#include <vector>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <atomic>
#include <cstdint>
#include <algorithm>
#if defined(__linux__)
#include <sched.h>
#endif

int InstanceSpawner::SlotCount(const ControlServerConfig& cfg) {
    if (cfg.game_cpu_range.lo < 0 || cfg.game_cpu_range.hi < cfg.game_cpu_range.lo) return 0;
    const int span = cfg.game_cpu_range.hi - cfg.game_cpu_range.lo + 1;
    int cpi = cfg.cores_per_instance;
    if (cpi <= 0 || cpi > span) cpi = span;
    return std::max(1, span / cpi);
}

std::string InstanceSpawner::SlotPrefixPath(const ControlServerConfig& cfg, int slot_idx) {
    if (slot_idx < 0) return std::string();
    return cfg.wine_prefix + "-slot-" + std::to_string(slot_idx);
}

bool InstanceSpawner::EnsureSlotPrefixes(const ControlServerConfig& cfg) {
    if (!cfg.per_slot_prefix) return true;
    if (cfg.game_cpu_range.lo < 0 || cfg.game_cpu_range.hi < cfg.game_cpu_range.lo) {
        Logger::Log("spawner",
            "[InstanceSpawner] per_slot_prefix=true but game_cpu_range disabled — using base prefix\n");
        return true;
    }

    const int slots = SlotCount(cfg);
    Logger::Log("spawner",
        "[InstanceSpawner] preparing %d slot prefixes from base '%s'\n",
        slots, cfg.wine_prefix.c_str());

    for (int s = 0; s < slots; ++s) {
        const std::string path = SlotPrefixPath(cfg, s);
        struct stat st{};
        if (stat(path.c_str(), &st) == 0) {
            Logger::Log("spawner",
                "[InstanceSpawner]   slot %d: '%s' already exists, skipping clone\n",
                s, path.c_str());
            continue;
        }

        Logger::Log("spawner",
            "[InstanceSpawner]   slot %d: cloning '%s' -> '%s' (cp -a)...\n",
            s, cfg.wine_prefix.c_str(), path.c_str());

        // cp -a preserves permissions, ownership, symlinks, timestamps.
        // Single-quoted to handle spaces in the path; assumes the path
        // doesn't contain a single quote (cfg-controlled, safe).
        const std::string cmd =
            "cp -a '" + cfg.wine_prefix + "' '" + path + "'";
        const int rc = system(cmd.c_str());
        if (rc != 0) {
            Logger::Log("spawner",
                "[InstanceSpawner]   slot %d: FAILED (rc=%d): %s\n", s, rc, cmd.c_str());
            return false;
        }
        Logger::Log("spawner", "[InstanceSpawner]   slot %d: ready\n", s);
    }
    return true;
}

pid_t InstanceSpawner::Spawn(const ControlServerConfig& cfg,
                              const std::string& map_name,
                              const std::string& game_mode,
                              uint16_t udp_port,
                              int64_t instance_id) {
    // Round-robin counter for CPU pinning. Persists across calls so each
    // spawn lands on the next slot in the configured range. Lifetime is
    // the control-server process; reset on restart, which is fine.
    static std::atomic<uint64_t> spawn_counter{0};

    // Pre-fork: pick the slot for this instance. slot_idx maps to BOTH a
    // contiguous core range (start..end inclusive) AND, if per_slot_prefix
    // is on, a dedicated WINEPREFIX. -1 in any of these = pinning disabled.
    // Computed in the parent so we can log it AND the child sees the values
    // via the post-fork memory copy.
    int slot_idx           = -1;
    int affinity_core_start = -1;
    int affinity_core_end   = -1;
    if (cfg.game_cpu_range.lo >= 0 && cfg.game_cpu_range.hi >= cfg.game_cpu_range.lo) {
        const int span = cfg.game_cpu_range.hi - cfg.game_cpu_range.lo + 1;
        // cores_per_instance: 0 (or > span) → full pool; clamp otherwise.
        int cpi = cfg.cores_per_instance;
        if (cpi <= 0 || cpi > span) cpi = span;
        const int slots = std::max(1, span / cpi);   // integer floor — leftover cores at the top stay unused
        const uint64_t idx = spawn_counter.fetch_add(1, std::memory_order_relaxed);
        slot_idx = static_cast<int>(idx % static_cast<uint64_t>(slots));
        affinity_core_start = cfg.game_cpu_range.lo + slot_idx * cpi;
        affinity_core_end   = affinity_core_start + cpi - 1;
    }

    // Pick the WINEPREFIX. Slot-isolated when per_slot_prefix is on AND
    // we have a valid slot; otherwise fall back to the base prefix.
    const std::string wine_prefix =
        (cfg.per_slot_prefix && slot_idx >= 0)
            ? SlotPrefixPath(cfg, slot_idx)
            : cfg.wine_prefix;

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

#if defined(__linux__)
        // Pin the process tree (xvfb-run -> Xvfb -> wine -> game) to the
        // configured core range. Affinity inherits across fork/exec, so
        // this one call covers everything below us. Silent failure: if
        // the kernel refuses (e.g. CPU offline) we just run unpinned.
        if (affinity_core_start >= 0) {
            cpu_set_t mask;
            CPU_ZERO(&mask);
            for (int c = affinity_core_start; c <= affinity_core_end; ++c) {
                CPU_SET(c, &mask);
            }
            sched_setaffinity(0, sizeof(mask), &mask);
        }
#endif

        // Silence Wine's fixme:/warn: spam — every line is a stdio write
        // that round-trips through conhost.exe and chews CPU/I/O. Skip
        // when --wine-debug is on so winedbg's output is preserved.
        if (!cfg.wine_debug) {
            setenv("WINEDEBUG", "-all", 1);
        }

        // Detach stdout/stderr to /dev/null. Combined with WINEDEBUG=-all
        // above this leaves conhost.exe with nothing to do; without it
        // conhost.exe sits at several percent CPU on the same core as
        // the game tick. Skipped under --wine-debug so winedbg's
        // interactive output stays attached.
        if (!cfg.wine_debug) {
            [[maybe_unused]] FILE* devnull_out = freopen("/dev/null", "w", stdout);
            [[maybe_unused]] FILE* devnull_err = freopen("/dev/null", "w", stderr);
        }

        // Set Wine environment and exec the game binary.
        setenv("WINEDLLOVERRIDES", cfg.dll_overrides.c_str(), 1);
        setenv("WINEPREFIX", wine_prefix.c_str(), 1);

        // Build all arg strings as named locals to prevent dangling c_str() pointers.
        std::string map_arg     = map_name + "?Game=" + game_mode;
        std::string host_arg    = "-host=" + cfg.host;
        std::string hostdns_arg = "-hostdns=" + cfg.hostdns;
        std::string port_arg    = "-port=" + std::to_string(udp_port);
        std::string ipc_port_arg  = "-ipcport=" + std::to_string(cfg.ipc_port);
        std::string inst_id_arg   = "-instanceid=" + std::to_string(instance_id);
        std::string gamepath_arg  = "-gamepath=" + cfg.game_binary;
        std::string fixguids_arg  = std::string("-fixpackageguids=") + (cfg.fix_package_guids ? "1" : "0");
        std::string clearlogs_arg = std::string("-clearlogs=")       + (cfg.clear_logs        ? "1" : "0");
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
            fixguids_arg.c_str(), clearlogs_arg.c_str(),
            crashdir_arg.c_str(), logdir_arg.c_str(),
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
    if (affinity_core_start >= 0) {
        Logger::Log("spawner",
            "[InstanceSpawner] Spawned instance_id=%lld pid=%d port=%d slot=%d cores=%d-%d prefix='%s' map=%s\n",
            (long long)instance_id, (int)pid, (int)udp_port,
            slot_idx, affinity_core_start, affinity_core_end,
            wine_prefix.c_str(), map_name.c_str());
    } else {
        Logger::Log("spawner",
            "[InstanceSpawner] Spawned instance_id=%lld pid=%d port=%d map=%s\n",
            (long long)instance_id, (int)pid, (int)udp_port, map_name.c_str());
    }

#if defined(__linux__)
    // Legacy mitigation: when prefixes are SHARED (per_slot_prefix=false)
    // wineserver is a singleton inherited from whichever instance forked
    // it first, so other instances pay a cross-core IPC tax on every
    // Win32 syscall. Widening its mask lets the kernel co-locate it with
    // the active instance. Skipped under per_slot_prefix=true because
    // each slot already has its own dedicated wineserver auto-affinitied
    // via the spawning instance — pinning would only cause pointless
    // mask churn across multiple wineservers.
    //
    // Done via a backgrounded shell helper so we don't block the spawn
    // path (and to gracefully handle "wineserver hasn't started yet" on
    // the first spawn — `pgrep` empty → `taskset` fails harmlessly →
    // next spawn applies it). The 1-second sleep covers the typical
    // wine cold-start delay.
    if (cfg.pin_wineserver && !cfg.per_slot_prefix
        && cfg.game_cpu_range.lo >= 0 && cfg.game_cpu_range.hi >= cfg.game_cpu_range.lo) {
        uint64_t bitmask = 0;
        for (int c = cfg.game_cpu_range.lo; c <= cfg.game_cpu_range.hi; ++c) {
            bitmask |= (1ULL << c);
        }
        char cmd[256];
        snprintf(cmd, sizeof(cmd),
            "(sleep 1; taskset -ap %llx $(pgrep -x wineserver) >/dev/null 2>&1) &",
            (unsigned long long)bitmask);
        [[maybe_unused]] int rc = system(cmd);
        Logger::Log("spawner", "[InstanceSpawner] wineserver pin queued: mask=0x%llx\n",
            (unsigned long long)bitmask);
    }
#endif

    return pid;
}
