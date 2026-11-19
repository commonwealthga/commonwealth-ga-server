#pragma once

#include <map>
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

// One volunteer node, as listed in control-server.json.
//
// `id` is the integer identity used everywhere internally (ga_instances.node_id,
// placement). `name` is the wire identity bound into the handshake transcript and
// used in logs; it defaults to the decimal id. The key is the node's 32-byte
// pre-shared key as 64 hex characters.
// A region the master serves. Retail's client shows these in its site list and
// sends the chosen one back in the credential request (SYS_SITE_ID).
struct SiteEntryConfig {
    int         id          = 0;
    int         name_msg_id = 0;   // asm_data_set_msg_translations id for the client label
    std::string label;             // for logs only; the client shows name_msg_id
    bool        enabled     = true;
};

struct NodeEntryConfig {
    int         id = 0;
    std::string name;
    std::string key;
    int         site = 0;
    std::string address;      // public address its instances are reachable at
    int         capacity = 0; // max concurrent instances; 0 = unlimited
    bool        home_map = false;
    bool        enabled = true;
};

// What a node needs to know about itself, from the "node" block of its config.
struct NodeSelfConfig {
    std::string id;           // wire identity; must match a master nodes[].name
    std::string key;          // 64 hex chars, the same key the master holds
    std::string master_addr;  // "host:port" of the master's node listener

    // How this node treats a build the master offers. Local control, so an
    // operator can refuse updates even if the master insists -- a node you cannot
    // physically reach should not be at the mercy of a master misconfiguration.
    //   "apply" (default) download, verify, write, and for the agent exit so a
    //                     supervisor restarts it on the new build
    //   "stage"           download, verify, write <path>.new, log -- never swap.
    //                     Proves the transfer on a remote machine first.
    //   "off"             ignore offers entirely
    std::string update_mode = "apply";
};

struct ControlServerConfig {
    // ---- Role --------------------------------------------------------------
    // "master" (default) — the control server proper: login, lobby, chat,
    // matchmaking, database authority, instance placement.
    // "node" — the game-host agent for another machine's master: spawns the
    // instances the master asks for, serves them from a local database, and
    // relays their IPC. Only Phase 0 of the multi-node design is implemented
    // today, so a node role currently refuses to start rather than silently
    // behaving like a second master.
    std::string role               = "master";
    bool IsMaster() const { return role == "master"; }
    bool IsNode() const { return role == "node"; }

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
    // Bind address for the IPC listener. Game instances are always spawned on
    // this machine (or in a --network=host container), so loopback is right:
    // INSTANCE_HELLO carries no authentication, and a public IPC port would let
    // anyone on the internet impersonate an instance. Remote nodes reach the
    // master through the node link, never through this port.
    std::string ipc_bind           = "127.0.0.1";

    // ---- Multi-node --------------------------------------------------------
    // Where nodes dial in. Unlike the IPC port this one is necessarily reachable
    // from the internet: the handshake authenticates every peer against its
    // per-node key before anything else happens.
    uint16_t    node_port          = 9020;
    std::string node_bind          = "0.0.0.0";
    std::vector<NodeEntryConfig> nodes;

    // Where content snapshots live. The master builds content-<hash>.db here and
    // serves it; a node stores what it downloads here and serves instances from it.
    std::string content_dir        = "content";

    // Schema bootstrap. The control server is the only process that migrates a
    // database: when db_path has no schema at all it is replaced with base_db,
    // then every compiled-in migration above the recorded version is applied.
    //
    // base_db is generated by scripts/make_dbbase.py from a live database with
    // player data stripped. It is the one bootstrap input that stays a file --
    // it is a database, not a script. The .sql migrations themselves are embedded
    // into the binary at build time (scripts/embed_migrations.py).
    std::string base_db            = "dbbase/base.db";

    // Node role only: whether this node owns its content tables.
    //
    // false (default) -- trust db_path to already contain the game data. That is
    //   the single-machine setup, where the node shares the master's database;
    //   rewriting 5 MB of tables there would disturb live instances the master is
    //   running out of the same file.
    // true -- the node copies the master's content snapshot into its own database
    //   on startup and after every content change, and refuses to spawn until that
    //   has succeeded. This is the mode for a node with its own db_path.
    bool        apply_content      = false;

    // Secret that seals node pre-shared keys at rest in ga_nodes. Lives here and
    // only here: a copy of server.db must not be enough to impersonate a node.
    // Generate with `openssl rand -hex 32`. Losing it means re-provisioning every
    // node key; changing it invalidates the stored ones.
    std::string node_key_secret;

    // Path to the proxy DLL build the master offers to nodes. "" = do not offer
    // one (nodes keep whatever they have). It is a separate Windows-side artifact,
    // so the master can only distribute a copy the operator points it at.
    std::string node_dll;
    // Whether nodes are told to converge to this master's builds. Off = detection
    // only: the master logs a mismatch and offers nothing.
    bool        node_auto_update = true;

    // Sites (regions) this master serves, in preference order. Empty means the
    // historical single-site behaviour: id 2, "Europe", msg 41483.
    //
    // NOTE: sites and nodes are DATABASE rows (ga_sites / ga_nodes). These fields
    // are the in-memory model the rest of the code uses; they are filled by
    // NodeStore::Load from the database, and the values here are only an import
    // source for a database whose tables are still empty.
    //
    // Sites are a PLACEMENT HINT, not a queue scope -- queues stay global
    // (ga_queues.sys_site_id = 0). Splitting the queue list per site would strand
    // players at our population.
    std::vector<SiteEntryConfig> sites;

    // The site the master's OWN instances belong to (node id 0). Nodes declare
    // theirs in nodes[].site; without this the local spawner's site would be
    // undefined and "which region is this instance in?" would have no answer.
    // 0 = unset -> the first enabled site.
    int master_site = 0;

    // site id -> ordered site ids to try when the preferred one cannot place.
    // Missing/empty entry falls back to [itself] + every other enabled site in
    // config order. Kept as data so "NA east never lands in Asia" is a config
    // decision, not code.
    std::map<int, std::vector<int>> fallback_order;

    // Resolved accessors: these apply the defaults above, so callers never have
    // to special-case an empty config.
    const std::vector<SiteEntryConfig>& Sites() const;
    // The site a client gets when it asks for `requested` (0 = no preference):
    // the requested site if configured and enabled, else the first enabled one.
    int ResolveSite(int requested, std::string* why = nullptr) const;
    std::vector<int> FallbackLadder(int preferred) const;

    // Node role only: who this node is and where its master lives. Ignored by the
    // master, which learns the same values from nodes[].
    NodeSelfConfig node;
    std::string admin_token;
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

    // Player settings endpoint (the website's write path for the toggles that
    // used to be chat-command only: -togglebrokensuits, -toggleallsuits,
    // -togglesolomode, -enabledlc/-disabledlc).
    // 0 = disabled. Loopback by default: the only client is the website on the
    // same host. Requires admin_token — see PlayerSettingsHttpServer.
    uint16_t    settings_api_port  = 0;
    std::string settings_api_bind  = "127.0.0.1";
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

    // Load config from JSON file at path. Returns defaults if file is absent or invalid.
    static ControlServerConfig Load(const std::string& path);
};
