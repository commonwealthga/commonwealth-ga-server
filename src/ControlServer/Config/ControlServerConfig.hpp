#pragma once
#include "src/ControlServer/Config/EventCalendar.hpp"
#include <map>
#include "src/ControlServer/ApRewards/ApRewards.hpp"
#include "src/ControlServer/MarketBots/MarketBots.hpp"
#include "src/ControlServer/ServiceHooks/ServiceHooks.hpp"

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
    //std::string hostdns            = "77.237.240.162";    // skal deprecated
    std::string nat_networks       = "";
    std::string nat_ip             = "";
    std::string dll_overrides      = "version=n,b";
    std::string home_map_name      = "Dome3_VR_Arena_P";
    std::string home_map_game_mode = "TgGame.TgGame_Mission";
    // New agents start in the intro/training mission (solo
    // instance) instead of the home map. The client sends the map it wants as
    // TRAINING_MAP_GAME_ID on ADD_PLAYER_CHARACTER; if map_game_info knows that
    // id it wins, otherwise these defaults are used. Config: "intro_mission":
    // {"enabled":true,"map_name":"Inception_ALL","game_mode":"TgGame.TgGame_Mission"}
    bool        intro_mission_enabled   = false;
    std::string intro_mission_map_name  = "Inception_ALL";
    std::string intro_mission_game_mode = "TgGame.TgGame_Mission";
    // Tutorial chapters. The original tutorial was four
    // chapters: Inception -> Adrenaline -> Skylark -> Agency Zero -> Dome
    // City (each map's Kismet TgSeqAct_RequestMission names the next one).
    // "chain": maps played after map_name, in order; "chain_user_ids":
    // only these accounts get the chain (empty = everyone).
    std::vector<std::string> intro_chain_maps;
    std::vector<int64_t>     intro_chain_user_ids;
    // Lowest level an agent has when it enters the home map (0 = off).
    // Config: "home_arrival_min_level": 5
    uint32_t    home_arrival_min_level  = 0;
    // Token limits per agent (0 = off). Config: "token_limits":
    // {"weekly_earn": 50000, "bank": 50000} - weeks start Monday 00:00 UTC.
    int64_t     token_weekly_earn       = 0;
    int64_t     token_bank              = 0;
    // Quest reward tables that give ONE row of their list (the player's pick,
    // else a random row) instead of rolling each row. Config:
    // "quest_reward_choice_tables": [288]
    std::vector<int> quest_reward_choice_tables;
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
    // When true, game_cpu_range counts positions in the CPUs this process
    // may run on (sched_getaffinity at startup) instead of CPU numbers.
    // For containers whose cpuset is not 0..N-1 (e.g. 0-1,11,13-15).
    bool        game_cpu_relative  = false;
    std::vector<int> cpu_map;   // filled at startup when game_cpu_relative
    int CpuAt(int i) const {
        return (game_cpu_relative && i >= 0 && i < (int)cpu_map.size()) ? cpu_map[i] : i;
    }
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
    // DEV/TEST shortcuts (admin actions dev-end-mission ...).
    // Off by default; still needs admin_token + a local/private source.
    bool        dev_controls = false;
    // New agents get only a level-1 class starter kit (lowest
    // level class item per slot up to starter_kit_max_level, common, no mods)
    // instead of upstream's free gear/armor/cosmetic/dye collection.
    bool        starter_kit_only = true;
    int         starter_kit_max_level = 5;
    // Read-only HTTP JSON endpoint for the spectator broadcast overlay
    // (live per-instance health/effect state, see SpectatorOverlayState).
    // 0 = disabled — no listener is started. Gated by overlay_token (query
    // param "token") the same way admin actions are gated by admin_token;
    // empty token means the endpoint is open to anyone who can reach the
    // port, so set one before exposing this beyond localhost.
    uint16_t    overlay_http_port  = 0;
    std::string overlay_token;
    // Bind address for the overlay listener. Default keeps the original
    // all-interfaces behavior (LAN OBS setups); set "127.0.0.1" when the
    // public face is the docker/spectator-overlay proxy on the same host,
    // so the token-gated port is not directly reachable from outside.
    std::string overlay_bind       = "0.0.0.0";
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
    bool        allow_duplicate_account_logins = false;
    // Per-instance stats recording toggles, forwarded to game instances in
    // INSTANCE_HELLO_ACK. device_stats = the server-side mirror of the
    // client Device Stats tab counters (ga_match_device_stats baseline);
    // effectiveness = everything layered on top (cleanse/boost/power/savior
    // tracking, buff windows, uses, DEVICE_USED events). The client's own
    // tab is unaffected by either. Flip + restart control server; no rebuild.
    bool        device_stats_enabled  = true;
    bool        effectiveness_enabled = true;
    // When true (default), the login handler verifies the RC4 credential blob
    // against the account's stored verifier (first login registers it). Set
    // false to fall back to the old username-only behavior — an emergency
    // rollback knob in case the GUID-string reconstruction needs field tuning.
    bool        require_password_verification = true;
    // Only log in accounts provisioned out-of-band (ga_users row with a
    // verifier); never auto-register, names match case-insensitively.
    bool        require_provisioned_accounts = false;
    // Optional external services (ServiceHooks.hpp). Empty = built-in rules only.
    std::vector<ServiceHooks::Service> services;

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

    // ---- Moderation -------------------------------------------------------
    // Login-bug spoof for banned logins. Default "silent" mimics the
    // historical login bug — no response, no socket close; client times
    // out after ~10s and surfaces "Unable to connect to server".
    // "garbage" sends 32 random bytes then closes; fallback if "silent"
    // ever produces a visibly different client surface.
    // fallback_close_sec: 0 = never auto-close silent spoof; >0 = backstop.
    struct BanSpoofConfig {
        std::string mode               = "silent";
        int         fallback_close_sec = 0;
    };
    BanSpoofConfig ban_spoof;

    // Live-kick of an in-game banned player via bogus GSC_GO_PLAY. If the
    // client hasn't dropped the TCP socket within fallback_close_sec, the
    // server force-closes it as a safety backstop.
    struct KickConfig {
        int fallback_close_sec = 30;
    };
    KickConfig kick;

    // Player names allowed to use "-announce <text>" (case-insensitive).
    // Empty (the default) = nobody can. Chat channel 20 is unfilterable, so
    // this is deliberately not open to everyone.
    // ponytail: a config list, not an ga_users column — swap to a DB flag if
    // this ever needs per-account revocation or more than a couple of names.
    std::vector<std::string> announcers;
    // One system line sent to each player when they join chat
    // (e.g. "TEST SERVER - may restart"). Empty = none.
    std::string login_notice;
    // Global News stand feed (Dome City). JSON {"title", "posts": [{"date", "text"}]},
    // newest first; re-read on every use so admins can edit it live. Empty = stand silent.
    std::string news_file = "news.json";
    // Limited launch. max_online_players: agents past the login
    // queue at once (0 = no cap); extra logins wait in a FIFO queue on the
    // client's own "Server Is Currently Full" panel. max_instances: game
    // instances at once incl. the hub (0 = no cap); the hub always keeps one
    // slot, missions past the cap wait in their matchmaking queue.
    int max_online_players = 24;
    int max_instances = 6;
    // Player cap per Dome City (hub) / open-world zone instance, passed to the
    // instance as -maxplayers (0 = the map's own ini value, 32).
    int dome_cap = 48;
    int zone_cap = 48;
    // One-player matches offered to the "external_instance" service (if one is
    // configured) wait this long for INSTANCE_READY, then start here instead.
    int external_ready_timeout_s = 45;
    // Botted Auction House ("market_bots": {...} - field names as in MarketBots::Settings)
    MarketBots::Settings market_bots;
    // AP earning ("ap_rewards": {...} - field names as in ApRewards::Settings)
    ApRewards::Settings ap_rewards;
    // Dated content windows ("calendar": {...}, see EventCalendar.hpp)
    std::map<std::string, EventCalendar::Window> calendar;

    // Load config from JSON file at path. Returns defaults if file is absent or invalid.
    static ControlServerConfig Load(const std::string& path);
};
