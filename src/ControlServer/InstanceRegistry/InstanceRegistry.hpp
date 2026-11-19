#pragma once

#include <string>
#include <map>
#include <optional>
#include <cstdint>
#include <mutex>
#include <vector>
#include <utility>

// InstanceRegistry.hpp -- Tracks known game instances in the ga_instances SQLite table.
// One row per running (or recently stopped) UE3 game server process.

struct InstanceInfo {
    int64_t     id           = 0;
    std::string map_name;
    std::string game_mode;
    std::string state;          // "STARTING" | "DRAFTING" | "READY" | "DRAINING" | "STOPPED"
    int         pid           = 0;
    uint16_t    udp_port      = 0;
    //std::string ip_address    = "127.0.0.1";
    int         player_count  = 0;
    int64_t     started_at    = 0;
    int64_t     sealed_at     = 0;  // 0 if not sealed
    int64_t     instance_id   = 0;  // unique ID assigned at spawn time (rowid)
    bool        is_home_map   = false; // true if spawned for home map
    int         max_players   = 0;  // filled by INSTANCE_READY

    // Owning node. 0 = this machine (the master is node 0), so single-node
    // deployments and every pre-multi-node row stay valid as-is. Non-zero
    // means the process runs elsewhere and `pid` is only meaningful for logs.
    int         node_id       = 0;

    // Continuous-queue plumbing (NULL/0 for everything else).
    uint32_t    queue_id                  = 0;  // 0 = not from a matchmade queue
    int64_t     predecessor_instance_id   = 0;  // 0 = not a successor
    int64_t     end_mission_at            = 0;  // 0 = BeginEndMission not yet fired

    // Team-aware access (2026-06-11). "OPEN" | "PARTY_LOCKED" | "SEALED".
    // owner_party_ids is a CSV of QueuedParty ids permitted into a PARTY_LOCKED
    // instance (matchmaking-side; invites bypass it). Empty/OPEN = drop-in.
    std::string access_mode = "OPEN";
    std::string owner_party_ids;  // CSV of uint64 party ids
};

class InstanceRegistry {
public:
    // No-op -- mutex is default-constructed; here for symmetry with PlayerSessionStore.
    static void Init();

    // Set the external IP that instances are reachable at (from config).
    //static void SetHost(const std::string& host);//skal: unused


    // Return the first READY instance for the given map name, or nullopt if none.
    static std::optional<InstanceInfo> GetReadyInstance(const std::string& map_name);

    // Oldest STARTING-or-READY instance for the map. STARTING counts as live,
    // which is what keeps open-world travel to a single shared instance while
    // a spawn is still in flight.
    static std::optional<InstanceInfo> GetLiveInstanceByMapName(const std::string& map_name);

    // Mark all non-STOPPED instances from a previous run as STOPPED, then insert
    // a fresh READY row for the home map at the given UDP port.
    static void SeedHomeMapInstance(const std::string& map_name, uint16_t udp_port);

    // Insert a STARTING instance. Returns the SQLite rowid (used as instance_id).
    // queue_id=0 → not from a matchmade queue (home map, ad-hoc instance).
    // predecessor_instance_id=0 → not a successor. When non-zero, MarkReady
    // will transition the row to DRAFTING (instead of READY) until
    // MarkMissionEnded(predecessor) promotes it.
    static int64_t InsertStarting(const std::string& map_name, const std::string& game_mode,
                                   uint16_t udp_port, int pid, bool is_home_map,
                                   uint32_t queue_id = 0,
                                   int64_t predecessor_instance_id = 0,
                                   const std::string& access_mode = "OPEN",
                                   const std::string& owner_party_ids = "",
                                   int node_id = 0);

    // Mirror a master-owned instance row, keeping the master's instance id.
    //
    // A node uses this so an instance has one identity everywhere (logs, IPC,
    // ga_instance_players, the DLL's own queries). Idempotent: a re-dispatch or a
    // reconnect for the same id resets the existing row to STARTING instead of
    // failing on the unique index, which also makes a node and master sharing one
    // database (the single-machine test setup) work.
    //
    // victory_bonus_lives is passed explicitly because the master derives it from
    // the queue at spawn time; the node's own database may not even have that
    // queue row. The DLL reads this column in InitGameRepInfo.
    // Returns the instance_id, or 0 on failure.
    static int64_t InsertStartingWithId(int64_t instance_id,
                                        const std::string& map_name,
                                        const std::string& game_mode,
                                        uint16_t udp_port, int pid, bool is_home_map,
                                        uint32_t queue_id = 0,
                                        int64_t predecessor_instance_id = 0,
                                        const std::string& access_mode = "OPEN",
                                        const std::string& owner_party_ids = "",
                                        int node_id = 0,
                                        uint64_t match_seed = 0,
                                        int victory_bonus_lives = 0);

    // How many non-STOPPED instances this node currently has. Used to enforce
    // nodes[].capacity before dispatching more work to it.
    static int CountActiveInstancesForNode(int node_id);

    // Every non-STOPPED instance whose ga_instances.node_id is this node.
    // A node uses this to find instances it still owns after a restart or a lost
    // in-memory entry -- anything here that is not running locally is an orphan.
    static std::vector<InstanceInfo> GetInstancesForNode(int node_id);

    // ga_instances.victory_bonus_lives for an instance, 0 if the row is gone.
    // The master forwards it in INSTANCE_SPAWN so a remote node's mirrored row
    // carries the same challenge-bonus threshold.
    static int GetVictoryBonusLives(int64_t instance_id);

    // Store the real PID after the process has been spawned.
    static void UpdatePid(int64_t instance_id, int pid);

    // Store the UDP port an instance actually bound. A remote node picks the port
    // itself, so the master inserts the row with 0 and fills it in when the node
    // reports INSTANCE_SPAWNING -- which always precedes READY, so nothing can
    // hand out a GO_PLAY for a port-0 instance.
    static void UpdateUdpPort(int64_t instance_id, uint16_t udp_port);

    // Match seed for an instance (ga_instances.match_seed), or 0 when the row
    // predates seeding / doesn't exist. Minted by the Insert* calls below and
    // handed to the instance as -seed=, so the whole spawn/objective/variant
    // rollout replays under it. See src/Shared/MatchSeed.hpp for the scheme.
    static uint64_t GetMatchSeed(int64_t instance_id);

    // Overwrite the stored seed (used when the DLL reports the seed it actually
    // used — an instance started without -seed= generates its own, and we still
    // want the row to be replay-referenceable).
    static void SetMatchSeed(int64_t instance_id, uint64_t seed, int seed_scheme);

    // Transition STARTING -> READY when INSTANCE_READY IPC arrives.
    static void MarkReady(int64_t instance_id, int max_players);
    static void SetMaxPlayers(int64_t instance_id, int max_players);

    // Find first READY instance with is_home_map=1.
    // Sites this registry can place into. Derived rather than stored per row: a
    // local instance (node id 0) belongs to the master's site, a remote one to the
    // site its node declares. Set once at startup; changing config later re-homes
    // nothing that is already running, which is the behaviour we want for a
    // placement decision that was already logged.
    static void SetSiteSources(int master_site, const std::map<int, int>& node_sites);
    static int  SiteOfInstance(const InstanceInfo& inst);

    // site 0 = "any site" (the historical behaviour); a specific site prefers an
    // instance in that site, then the master's site, then anything ready.
    static std::optional<InstanceInfo> GetReadyHomeInstance(int site = 0);

    // Look up any instance by instance_id (any state).
    static std::optional<InstanceInfo> GetInstanceById(int64_t instance_id);

    // Mark an instance STOPPED (IPC disconnect or crash).
    static void MarkStopped(int64_t instance_id);

    // Get next available UDP port from range. Returns nullopt if pool exhausted.
    static std::optional<uint16_t> AllocatePort(uint16_t lo, uint16_t hi);

    // Clear all non-STOPPED instances (startup recovery).
    static void ClearStaleInstances();

    // profile_id is the class the player was queued as (PROFILE_ASSAULT etc.,
    // or 0 if unknown — non-matchmade joins / home map). Drives the per-class
    // breakdown shown on GET_TICKET_INFO queue cards.
    static void InsertInstancePlayer(int64_t instance_id, const std::string& session_guid,
                                     int64_t character_id, int task_force,
                                     uint32_t profile_id = 0);
    static void UpdateInstancePlayerTaskForce(int64_t instance_id, const std::string& session_guid,
                                             int task_force);

    // Look up the (instance_id, task_force_number) of the player's
    // currently-active ga_instance_players row, or nullopt if not in any
    // non-stopped instance. Used by ChatCommand::DispatchChangeTeam so the
    // control server can resolve -changeteam locally.
    static std::optional<std::pair<int64_t, int>>
        GetInstancePlayerTaskForce(const std::string& session_guid);

    // Per-instance roster of players who haven't left yet. Returned in
    // (guid, profile_id, task_force) tuples so the caller can compute
    // per-team heal scores without a second query. Used by
    // IpcServer::MSG_MISSION_ENDED and DataDrivenMatchRule for the
    // BalancedPvp join-existing seed.
    struct ActivePlayerRow {
        std::string guid;
        uint32_t    profile_id;
        int         task_force;
        int64_t     user_id = 0;   // ga_users.id via character row; 0 if unresolvable
    };
    static std::vector<ActivePlayerRow>
        GetActivePlayersForInstance(int64_t instance_id);

    // Players currently inside any running instance, joined with identity
    // fields for QUERY_PLAYERS (player search). One row per active
    // ga_instance_players entry.
    struct SearchablePlayerRow {
        std::string session_guid;
        int64_t     character_id = 0;
        std::string player_name;
        uint32_t    profile_id = 0;      // class value id (680/567/681/679)
        std::string map_name;
        bool        in_mission = false;  // instance is not a home map
    };
    static std::vector<SearchablePlayerRow> GetActiveSearchablePlayers();

    // Every ga_instance_players row that hasn't been marked left yet, across
    // all instances. Used by the periodic ghost-player reconciliation in
    // main.cpp: a row whose session guid has no live control connection is a
    // crashed player whose PLAYER_LEFT was lost.
    struct InstancePlayerRef {
        int64_t     instance_id = 0;
        std::string session_guid;
    };
    static std::vector<InstancePlayerRef> GetAllActiveInstancePlayers();

    static void MarkInstancePlayerLeft(int64_t instance_id, const std::string& session_guid);
    static void MarkAllInstancePlayersLeft(int64_t instance_id);
    static std::pair<int, int> GetTeamCounts(int64_t instance_id);
    static int GetActivePlayerCount(int64_t instance_id);

    // Queue-scoped active counters consumed by RuntimeStats for the
    // GET_TICKET_INFO PLAYER_COUNT / INSTANCE_COUNT / DATA_SET_PROFILE_COUNTS
    // fields. Instance count uses not-ended mission rows; player/class counts
    // include players already inside queue instances.
    static int GetActiveInstanceCountForQueue(uint32_t queue_id);
    static int GetActivePlayerCountForQueue(uint32_t queue_id);
    struct ActiveProfileCounts { uint32_t assault, medic, recon, robotics; };
    static ActiveProfileCounts GetActiveProfileCountsForQueue(uint32_t queue_id);
    static std::vector<InstanceInfo> GetReadyMissionInstances();
    static std::optional<InstanceInfo> GetHomeInstance(int site = 0);
    static std::vector<InstanceInfo> GetIdleInstances(int timeout_seconds);
    // All instances not in STOPPED state with a valid pid. Used by the
    // control-server shutdown path to SIGTERM-then-SIGKILL each game
    // process group before exiting, so systemd doesn't get stuck waiting
    // on orphaned wine processes (winedevice.exe ignores SIGTERM).
    static std::vector<InstanceInfo> GetAllRunningInstances();
    static void SetLastEmptyAtIfEmpty(int64_t instance_id);

    // Continuous-queue helpers ----------------------------------------------

    // Stamp end_mission_at=now on the parent row AND promote any DRAFTING
    // successor (matching predecessor_instance_id) to READY in the same
    // transaction. Returns the successor's instance_id (or 0 if none / not
    // DRAFTING). Idempotent on the parent stamp.
    static int64_t MarkMissionEnded(int64_t parent_instance_id);

    // Orphan fallback: if `parent_instance_id` died (IPC disconnect / crash)
    // before it called MarkMissionEnded, unlock its successor so the
    // matchmaker can use it normally. Returns successor instance_id or 0.
    static int64_t PromoteSuccessor(int64_t parent_instance_id);

    // Lookup the (non-STOPPED) successor of a parent, or nullopt.
    static std::optional<InstanceInfo> GetSuccessor(int64_t parent_instance_id);

private:
    static std::mutex mutex_;
    //static std::string s_host_;//skal: unused
};
