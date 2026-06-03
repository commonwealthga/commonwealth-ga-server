#pragma once

#include <string>
#include <vector>
#include <cstdint>

// ControlServerConfig.hpp -- Configuration for the control server.
// Loaded from a JSON file at startup; all fields have sensible defaults.

struct PortRange {
    uint16_t lo;
    uint16_t hi;
};

// Inclusive CPU core range for round-robin pinning of spawned game
// instances via sched_setaffinity. {-1, -1} disables pinning.
struct CpuRange {
    int lo;
    int hi;
};

struct ControlServerConfig {
    std::string wine_binary        = "/home/zax/lutrisproton/lutrisprotonge/bin/wine";
    std::string wine_prefix        = "/home/zax/games/gawineprefixserver";
    std::string xvfb_run_path      = "xvfb-run";
    std::string game_binary        = "/home/zax/games/ga/Binaries/GlobalAgenda.exe";
    std::string host               = "77.237.240.162";
    std::string hostdns            = "77.237.240.162";
    std::string dll_overrides      = "version=n,b";
    std::string home_map_name      = "Dome3_VR_Arena_P";
    std::string home_map_game_mode = "TgGame.TgGame_Mission";
    PortRange   udp_port_range     = {9002, 9020};
    // Cores for game-instance affinity. Spawned instances round-robin
    // across [lo..hi] inclusive; once exhausted, wraps. {-1, -1} disables.
    CpuRange    game_cpu_range     = {-1, -1};
    // How many consecutive cores each instance is pinned to.
    // 2 = sweet spot for UE3 -nullrhi (1 hot game thread + PhysX/Wine on
    // the second). 0 (or >= range span) = full pool, kernel decides per
    // thread. 1 = strict single-core pinning (highest cache locality but
    // worst tail latency under PhysX load).
    int         cores_per_instance = 2;
    // Whether to widen wineserver's affinity to cover all game_cpu_range
    // cores after each spawn. wineserver is a singleton per WINEPREFIX
    // and inherits the affinity of whichever instance forked it first;
    // without this, every Win32 syscall from instances on other cores
    // round-trips IPC across cores, costing ~25% %sys on the active core.
    // Ignored when per_slot_prefix=true (slot prefixes give each slot
    // its own wineserver, so cross-core IPC simply doesn't exist).
    bool        pin_wineserver     = false;
    // Per-slot WINEPREFIX isolation. When true, each pinning slot gets
    // its own prefix at "<wine_prefix>-slot-<N>", cloned from the base
    // prefix at startup. Each slot's wineserver is dedicated to its
    // cores — no cross-slot contention, no cross-core IPC, no pinning
    // gymnastics. Requires game_cpu_range to be valid.
    bool        per_slot_prefix    = false;
    uint16_t    tcp_port           = 9000;
    uint16_t    chat_port          = 9001;
    uint16_t    ipc_port           = 9010;
    std::string admin_token;
    int         startup_timeout_seconds = 120;
    std::string db_path            = "server.db";
    std::string crash_dir          = "Z:\\home\\zax\\games\\crashes";
    std::string log_dir            = "C:";
    // Logger channel allowlists. The DLL's Logger::EnableChannel and
    // EnableCrashChannel are called once per entry at DLL init — file
    // logging vs. crash-ring recording.
    // Channel names must not contain commas (the wire format joins on ',').
    std::vector<std::string> enabled_channels;
    std::vector<std::string> enabled_crash_channels;
    bool        fix_package_guids  = true;
    bool        wine_debug         = false;  // --wine-debug CLI flag: uses winedbg --command cont
    bool        clear_logs         = false;  // truncate per-channel files at boot for repeated tests
    bool        show_game_console  = false;  // Windows native debug: show spawned game commandlet consoles

    // ---- Docker spawn mode -------------------------------------------------
    // When use_docker is true, InstanceSpawner::Spawn execs `docker run`
    // around the xvfb-run/wine invocation instead of running it directly.
    // The wineprefix, game install, wine binary, control-server CWD, and
    // crash dir all bind-mount from the host at the same paths they have
    // on the host — no path translation, the inner argv is identical to
    // bare-metal mode.
    bool        use_docker         = false;
    std::string docker_image       = "ga-server:latest";
    // Memory cap per instance (docker --memory syntax: "2g", "512m", "0" = unlimited).
    std::string docker_memory      = "2g";
    // Root of the host wine install (e.g. ".../lutris-GE-Proton8-7-x86_64").
    // Bind-mounted RO into the container so cfg.wine_binary works as-is.
    // If empty, falls back to dirname(dirname(cfg.wine_binary)).
    std::string wine_install_dir;
    // Extra bind mounts, each "host_path[:container_path][:ro]". If
    // container_path is omitted it defaults to host_path (mirror layout).
    // Use this for anything the auto-mounts don't cover (e.g. host wine
    // runtime trees referenced by symlinks inside the prefix).
    std::vector<std::string> docker_extra_mounts;
    // When true, drops `--rm`, names the container `ga-inst-<instance_id>`,
    // and stops swallowing docker-client stdio. Inspect a failing spawn
    // with `docker logs ga-inst-<instance_id>` and `docker ps -a`.
    bool        docker_debug       = false;

    // Load config from JSON file at path. Returns defaults if file is absent or invalid.
    static ControlServerConfig Load(const std::string& path);
};
