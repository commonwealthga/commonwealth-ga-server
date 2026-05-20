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
        // parent can kill the entire tree (xvfb-run + wine + game, or
        // `docker run` + container) with kill(-pid, SIGKILL).
        setpgid(0, 0);

#if defined(__linux__)
        // Pin the process tree to the configured core range via affinity
        // inheritance across fork/exec. SKIPPED in docker mode because
        // --cpuset-cpus on `docker run` does this at the cgroup level
        // (a stronger guarantee than affinity hints) — applying it here
        // too would just confuse troubleshooting.
        if (!cfg.use_docker && affinity_core_start >= 0) {
            cpu_set_t mask;
            CPU_ZERO(&mask);
            for (int c = affinity_core_start; c <= affinity_core_end; ++c) {
                CPU_SET(c, &mask);
            }
            sched_setaffinity(0, sizeof(mask), &mask);
        }
#endif

        // Detach stdout/stderr to /dev/null. In bare-metal mode this
        // muzzles wine's fixme:/warn: spam (with WINEDEBUG=-all below it
        // also lets conhost.exe go idle, saving several %CPU). In docker
        // mode the same call redirects the `docker run` client's stdio
        // — equally desirable, since the daemon already captures the
        // container's own stdio independently. Skipped under wine_debug
        // (need winedbg interactive output) and under docker_debug
        // (need docker client errors like "image not found" visible).
        const bool redirect_stdio = !cfg.wine_debug
                                 && !(cfg.use_docker && cfg.docker_debug);
        if (redirect_stdio) {
            [[maybe_unused]] FILE* devnull_out = freopen("/dev/null", "w", stdout);
            [[maybe_unused]] FILE* devnull_err = freopen("/dev/null", "w", stderr);
        }

        // Bare-metal mode sets the Wine env vars on the child directly.
        // Docker mode passes them via `-e` on the `docker run` line below
        // (because they need to be set inside the container, not on the
        // docker client process).
        if (!cfg.use_docker) {
            if (!cfg.wine_debug) setenv("WINEDEBUG", "-all", 1);
            setenv("WINEDLLOVERRIDES", cfg.dll_overrides.c_str(), 1);
            setenv("WINEPREFIX", wine_prefix.c_str(), 1);
        }

        // ---- Build the "inner" argv (xvfb-run + wine + game.exe + game args).
        // SAME shape in both modes — the DLL sees an identical command line
        // whether wrapped in `docker run` or not. Storage lives in
        // game_argv_storage to keep c_str() pointers valid through execvp.

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
        // Per-instance log subdirectory: append the instance_id to the base
        // log_dir so two simultaneous instances don't write to the same
        // <channel>.txt files and clobber each other's lines. Backslash to
        // match the Wine-path convention used by the rest of the logger code
        // (Logger::Log uses "%s\\%s.txt"). The DLL ensures the dir exists at
        // init via Logger::EnsureLogDirExists() — fopen("a") won't create it.
        // Control server's own log_dir (cfg.log_dir) stays unscoped; only the
        // per-instance arg is rewritten here.
        std::string logdir_arg    = "-logdir=" + cfg.log_dir + "\\" + std::to_string(instance_id);

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

        std::vector<const char*> inner_argv = {
            cfg.xvfb_run_path.c_str(), "-a",
            wine_bin.c_str(),
        };
        if (cfg.wine_debug) {
            inner_argv.push_back("--command");
            inner_argv.push_back("cont");
        }
        for (const char* a : {
            cfg.game_binary.c_str(),
            "server",
            map_arg.c_str(),
            "-nolog", "-noconsole", "-unattended",
            host_arg.c_str(), hostdns_arg.c_str(), port_arg.c_str(),
            "-seekfreeloading", "-tcp=300", "-nullrhi",
            ipc_port_arg.c_str(), inst_id_arg.c_str(), gamepath_arg.c_str(),
            fixguids_arg.c_str(), clearlogs_arg.c_str(),
            crashdir_arg.c_str(), logdir_arg.c_str(),
            enabled_channels_arg.c_str(), enabled_crash_channels_arg.c_str(),
        }) {
            inner_argv.push_back(a);
        }

        // ---- Bare-metal exec path: same as before.
        if (!cfg.use_docker) {
            inner_argv.push_back(nullptr);
            execvp(cfg.xvfb_run_path.c_str(), const_cast<char* const*>(inner_argv.data()));
            _exit(1);
        }

        // ---- Docker exec path: wrap the inner argv in `docker run`.
        // Auto-mounts mirror host paths into the container (no
        // translation), so cfg fields work as-is on both sides of the
        // namespace boundary. Anything we can't derive deterministically
        // (e.g. crash dir if it's not a Z:\ wine path) the user adds via
        // cfg.docker_extra_mounts.

        // dirname helper that returns the empty string for "/" or "" (so
        // we never accidentally bind-mount the root filesystem).
        auto dirname_of = [](const std::string& p) -> std::string {
            if (p.empty()) return {};
            size_t last = p.find_last_of('/');
            if (last == std::string::npos || last == 0) return {};
            return p.substr(0, last);
        };

        // Translate "Z:\\foo\\bar" → "/foo/bar" (Wine's default unix-root
        // drive). Returns empty for anything that isn't a Z: path.
        auto translate_wine_z = [](const std::string& wp) -> std::string {
            if (wp.size() < 3) return {};
            char d = wp[0];
            if ((d != 'Z' && d != 'z') || wp[1] != ':' ||
                (wp[2] != '\\' && wp[2] != '/')) return {};
            std::string out = wp.substr(2);
            for (auto& c : out) if (c == '\\') c = '/';
            return out;
        };

        // (host_path, "-v" value with optional :container[:ro] suffix).
        // We push each mount as one "-v" + one value string to keep args
        // simple and avoid quoting concerns.
        std::vector<std::string> docker_storage;
        docker_storage.reserve(64);
        docker_storage.push_back("docker");
        docker_storage.push_back("run");
        if (cfg.docker_debug) {
            // Keep the container around after exit so we can `docker logs` it,
            // and name it predictably so we know what to inspect.
            docker_storage.push_back("--name=ga-inst-" + std::to_string(instance_id));
        } else {
            docker_storage.push_back("--rm");
        }
        docker_storage.push_back("--init");                 // PID 1 reaping for the container
        docker_storage.push_back("--network=host");
        // Wine + Xvfb + game exceed docker's 64M /dev/shm default; on
        // bare metal you implicitly get the host's full shm pool.
        docker_storage.push_back("--shm-size=2g");
        // Share host's IPC namespace so wineserver's SysV semaphores /
        // shared memory behave the same as bare metal.
        docker_storage.push_back("--ipc=host");

        if (affinity_core_start >= 0) {
            std::string cset = std::to_string(affinity_core_start);
            if (affinity_core_end != affinity_core_start) {
                cset += "-" + std::to_string(affinity_core_end);
            }
            docker_storage.push_back("--cpuset-cpus=" + cset);
        }
        if (cfg.cores_per_instance > 0) {
            docker_storage.push_back("--cpus=" + std::to_string(cfg.cores_per_instance) + ".0");
        }
        if (!cfg.docker_memory.empty() && cfg.docker_memory != "0") {
            docker_storage.push_back("--memory=" + cfg.docker_memory);
        }

        // Match host UID/GID so bind-mounted writes keep host ownership
        // (otherwise wine creates root-owned files in your slot prefix
        // and bare-metal runs trip on permissions). The Dockerfile must
        // also have a /etc/passwd entry for this UID so wine's
        // getpwuid() lookups (used to resolve "My Documents" =
        // WINEPREFIX/drive_c/users/<name>/) resolve correctly.
        {
            char ug[64];
            snprintf(ug, sizeof(ug), "--user=%u:%u",
                     (unsigned)getuid(), (unsigned)getgid());
            docker_storage.push_back(ug);
        }

        // Mirror the control-server CWD inside the container so relative
        // paths like sqlite3_open("server.db") resolve to the same file
        // both the control server and the DLL see on the host.
        std::string host_cwd;
        {
            char cwdbuf[4096];
            if (getcwd(cwdbuf, sizeof(cwdbuf))) host_cwd = cwdbuf;
        }
        if (!host_cwd.empty()) {
            docker_storage.push_back("--workdir=" + host_cwd);
        }

        // Helper: add "-v host[:container[:ro]]" entries.
        auto add_mount = [&](const std::string& host_path,
                             const std::string& container_path,
                             bool read_only) {
            if (host_path.empty()) return;
            docker_storage.push_back("-v");
            std::string v = host_path + ":" + container_path;
            if (read_only) v += ":ro";
            docker_storage.push_back(v);
        };

        // Auto-mounts.
        //  1. Wine install dir — so cfg.wine_binary is invokable as-is.
        {
            std::string wine_root = cfg.wine_install_dir;
            if (wine_root.empty()) wine_root = dirname_of(dirname_of(cfg.wine_binary));
            add_mount(wine_root, wine_root, /*ro=*/true);
        }
        //  2. Wineprefix (slot or base) — RW so wine + DLL can write.
        add_mount(wine_prefix, wine_prefix, /*ro=*/false);
        //  3. Game install ROOT (two levels up from cfg.game_binary).
        //     UE3 layout is <root>/Binaries/<exe> with sibling dirs like
        //     TgGame/Config/, Engine/Content/, etc. Mounting only the
        //     Binaries dir lets the EXE load but hides every config and
        //     content file → PreInit fails → shutdown → PhysX cleanup
        //     null-deref. Mount RW since UE3 writes cache/lock files.
        {
            const std::string install_root = dirname_of(dirname_of(cfg.game_binary));
            add_mount(install_root, install_root, /*ro=*/false);
        }
        //  4. Control-server CWD — covers relative paths (server.db etc.).
        add_mount(host_cwd, host_cwd, /*ro=*/false);
        //  5. crash_dir if it's a Z:\\ wine path we can resolve to host.
        {
            std::string crash_host = translate_wine_z(cfg.crash_dir);
            if (!crash_host.empty()) add_mount(crash_host, crash_host, /*ro=*/false);
        }

        // User extras — each is "host[:container][:ro]" as the user wrote
        // it; mirror-layout default when no ':' is present.
        for (const auto& extra : cfg.docker_extra_mounts) {
            docker_storage.push_back("-v");
            if (extra.find(':') == std::string::npos) {
                docker_storage.push_back(extra + ":" + extra);
            } else {
                docker_storage.push_back(extra);
            }
        }

        // Env (analogues of the setenv() calls used in bare-metal mode).
        // Under docker_debug, skip WINEDEBUG=-all so wine's fixme/err
        // output reaches `docker logs`.
        if (!cfg.wine_debug && !cfg.docker_debug) {
            docker_storage.push_back("-e");
            docker_storage.push_back("WINEDEBUG=-all");
        }
        docker_storage.push_back("-e");
        docker_storage.push_back("WINEDLLOVERRIDES=" + cfg.dll_overrides);
        docker_storage.push_back("-e");
        docker_storage.push_back("WINEPREFIX=" + wine_prefix);
        // HOME must point at a writable path inside the container or
        // fontconfig/wine can't write their per-user caches and spam err
        // logs (some code paths also choke harder).
        docker_storage.push_back("-e");
        docker_storage.push_back("HOME=/var/cache/ga-server/home");

        // Image, then the inner argv verbatim.
        docker_storage.push_back(cfg.docker_image);

        // Build the final const char* argv. inner_argv currently points into
        // the named-local strings above, which outlive this function in the
        // child until execvp / _exit — same lifetime story as before.
        std::vector<const char*> docker_argv;
        docker_argv.reserve(docker_storage.size() + inner_argv.size() + 1);
        for (const auto& s : docker_storage) docker_argv.push_back(s.c_str());
        for (const char* p : inner_argv)     docker_argv.push_back(p);
        docker_argv.push_back(nullptr);

        execvp("docker", const_cast<char* const*>(docker_argv.data()));
        _exit(1);
    }

    // Parent process.
    const char* mode_tag = cfg.use_docker ? "docker" : "bare";
    if (affinity_core_start >= 0) {
        Logger::Log("spawner",
            "[InstanceSpawner] Spawned (%s) instance_id=%lld pid=%d port=%d slot=%d cores=%d-%d prefix='%s' map=%s\n",
            mode_tag, (long long)instance_id, (int)pid, (int)udp_port,
            slot_idx, affinity_core_start, affinity_core_end,
            wine_prefix.c_str(), map_name.c_str());
    } else {
        Logger::Log("spawner",
            "[InstanceSpawner] Spawned (%s) instance_id=%lld pid=%d port=%d map=%s\n",
            mode_tag, (long long)instance_id, (int)pid, (int)udp_port, map_name.c_str());
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
    if (cfg.pin_wineserver && !cfg.per_slot_prefix && !cfg.use_docker
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
