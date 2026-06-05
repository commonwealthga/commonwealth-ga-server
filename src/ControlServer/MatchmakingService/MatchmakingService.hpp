#pragma once

#include <asio.hpp>

#include <string>
#include <vector>
#include <optional>
#include <functional>
#include <unordered_map>
#include <chrono>
#include <memory>
#include <cstdint>

// ---------------------------------------------------------------------------
// Data structures
// ---------------------------------------------------------------------------

struct QueuedPlayer {
    std::string session_guid;
    uint32_t profile_id;  // ASSAULT, MEDIC, etc.
    std::chrono::steady_clock::time_point joined_at;
};

struct RunningInstance {
    int64_t instance_id;
    std::string map_name;
    std::string game_mode;
    int player_count;
    int max_players;
    uint32_t queue_id;
};

struct MatchResult {
    std::string map_name;
    std::string game_mode;
    std::vector<std::string> session_guids;
    std::optional<int64_t> existing_instance_id;  // if set, skip spawn
    std::unordered_map<std::string, int> task_force_assignments;  // session_guid -> task_force
    std::unordered_map<std::string, uint32_t> profile_ids;        // session_guid -> profile_id (ASSAULT etc.)

    // When set, overrides QueueConfig::max_players_per_instance for the
    // resulting PendingMatch.cap. Used by rules that want to seal a
    // pending match against further coalesce regardless of the queue's
    // nominal cap (e.g. DoubleAgentRule sets this to the popped roster
    // size at pop time — late joiners then bounce off the coalesce
    // size>=cap check). nullopt -> main.cpp falls back to queue config.
    std::optional<uint32_t> cap_override;
};

struct PendingMatch {
    int64_t instance_id;
    uint32_t queue_id;
    std::string game_mode;
    std::vector<std::string> session_guids;
    std::unordered_map<std::string, int> task_force_assignments;  // session_guid -> task_force
    std::unordered_map<std::string, uint32_t> profile_ids;        // session_guid -> profile_id; powers
                                                                  // GetQueuedProfileCounts so popped-
                                                                  // but-not-yet-registered players still
                                                                  // show on the queue card.
    uint32_t cap = 0;  // 0 = unlimited; otherwise hard ceiling on session_guids.size()
                       // — coalesce search skips when reached, append loop breaks.
};

// ---------------------------------------------------------------------------
// MatchRule -- base class for per-queue matching logic
// ---------------------------------------------------------------------------

class MatchRule {
public:
    virtual ~MatchRule() = default;

    virtual std::optional<MatchResult> Evaluate(
        const std::vector<QueuedPlayer>& players,
        const std::vector<RunningInstance>& instances) = 0;
};

// One weighted (map, game_mode) pair. weight is 1-based; weight=1 across all
// rows is uniform random. A queue's map_pool is a list of these — every fresh
// spawn (initial match-pop or successor) picks one via weighted random.
struct MapModeEntry {
    std::string map_name;
    std::string game_mode;
    int weight = 1;
    std::optional<int> min_players;  // NULL in DB → unbounded below
    std::optional<int> max_players;  // NULL in DB → unbounded above
};

// Legacy alias kept for callers outside MatchmakingService that just want
// (map_name, game_mode). Weight is dropped on conversion.
struct MapModeSpec {
    std::string map_name;
    std::string game_mode;
};

// ---------------------------------------------------------------------------
// QueueConfig -- one row of ga_queues plus the joined map_pool. Owned by
// MatchmakingService; passed by const-pointer to the rule so DataDrivenMatchRule
// can read its policy without an unordered_map lookup at Evaluate time.
// ---------------------------------------------------------------------------

enum class TaskforcePolicy : uint8_t {
    Pinned1,      // everyone -> TF 1 (SpecOps / Solo coop)
    Pinned2,      // everyone -> TF 2 (Defense)
    Balanced,     // place each player on the smaller of {team1, team2}
    BalancedPvp,  // class-aware role-weighted variant; see RoleWeightedSplit
};

// Per-queue pop-delay wait shape. Governs MaybeResetDelayedPop's re-arm
// behaviour while the delay timer is running. Cancel-on-below-min is
// independent of this enum and always fires.
enum class PopDelayPolicy : uint8_t {
    HalveOnJoin,   // existing: next_duration halves on each join, floor 0.5s
    Fixed,         // timer set once; ignores joins (cancels only on leave below min)
    ResetOnJoin,   // every join re-arms timer to cfg.pop_delay_seconds
};

struct QueueConfig {
    uint32_t queue_id = 0;
    uint32_t map_pool_id = 0;                         // 0 = no pool assigned
    std::string name;                                 // log/debug only
    std::string rule_class;                           // empty => DataDriven
    TaskforcePolicy taskforce_policy = TaskforcePolicy::Pinned1;
    bool continue_in_queue = false;
    bool enabled = true;

    // GET_TICKET_INFO wire fields — fed to TicketInfoEncoder verbatim.
    uint32_t queue_type_value_id = 0;
    uint32_t status_msg_id = 0;
    uint32_t name_msg_id = 0;
    uint32_t desc_msg_id = 0;
    uint32_t icon_id = 0;
    uint32_t max_players_per_side = 1;
    uint32_t min_players_per_team = 1;
    uint32_t max_players_per_team = 1;
    uint32_t level_min = 1;
    uint32_t level_max = 200;
    uint32_t tab = 0;
    float map_x = 0.0f;
    float map_y = 0.0f;
    bool map_active_flag = true;
    uint32_t map_icon_texture_res_id = 0;
    uint32_t video_res_id = 0;
    uint32_t location_value_id = 0;
    bool double_agent_flag = false;
    uint32_t sys_site_id = 0;
    uint32_t sort_order = 0;
    bool bonus_queue_flag = false;
    uint32_t difficulty_value_id = 0;
    // Wire-only override consumed by TicketInfoEncoder when set. NULL =>
    // fall back to difficulty_value_id. Lets queues share a real-world
    // difficulty (used for matchmaking + instance spawn) but show under a
    // different UI grouping on the client. Set on DA queues so they don't
    // mangle into the SpecOps difficulty rows.
    std::optional<uint32_t> marshal_difficulty_value_id;
    uint64_t access_flags = 0;
    bool active_flag = true;
    bool locked_flag = false;
    std::optional<uint32_t> remaining_seconds;  // nullopt -> field omitted

    // Queue-pop controls. Defaults preserve pre-feature behavior.
    uint32_t min_players_to_pop       = 1;
    uint32_t max_players_per_instance = 0;   // 0 = unlimited
    float    pop_delay_seconds        = 0.0f;

    // Pop-delay re-arm policy (governs MaybeResetDelayedPop when a
    // player joins/leaves while the delay timer is running).
    PopDelayPolicy pop_delay_policy = PopDelayPolicy::HalveOnJoin;

    // When true and queue.players.size() crosses max_players_per_instance,
    // AddPlayer cancels any running delay timer and pops immediately.
    // Independent of pop_delay_policy. Defaults to true: a benign
    // behavioural diff for queues that had max>0 + a running delay timer
    // — no reason to keep waiting once the lobby is full.
    bool instant_pop_when_full = true;

    std::vector<MapModeEntry> map_pool;
};

// ---------------------------------------------------------------------------
// MatchmakingService -- collects players into queues, evaluates rules
// ---------------------------------------------------------------------------

class MatchmakingService {
public:
    using MatchPopCallback = std::function<void(uint32_t queue_id, MatchResult result)>;
    // queue_id-scoped lookup: the provider returns READY mission instances
    // belonging to this queue only. Filter happens at provider call site so
    // rules can iterate `instances` without re-filtering.
    using InstanceProvider = std::function<std::vector<RunningInstance>(uint32_t queue_id)>;

    static void Init();

    // Wipe and re-load every queue from ga_queues + ga_map_pool_entries.
    // Preserves the player list of any queue whose queue_id still exists in
    // the DB after reload; drops players from removed queues with a log
    // entry. Pending matches keep their cached task_force_assignments and
    // remain unaffected. Safe to call mid-session — the asio io_context is
    // single-threaded so reload runs between TryPop calls.
    static void ReloadQueues();

    static void AddPlayer(uint32_t queue_id, const QueuedPlayer& player);
    static void RemovePlayer(const std::string& session_guid);

    static void SetMatchPopCallback(MatchPopCallback cb);
    static void SetInstanceProvider(InstanceProvider provider);

    // asio io_context used to schedule pop-delay timers. Must be set
    // during init before any AddPlayer call that could trigger a
    // pop_delay_seconds > 0 wait. Stored as a raw pointer because the
    // io_context outlives MatchmakingService — main() owns both.
    static void SetIoContext(asio::io_context* io);

    static void AddPendingMatch(int64_t instance_id, PendingMatch match);
    static std::optional<PendingMatch> ConsumePendingMatch(int64_t instance_id);

    // Pre-decided team assignments for players expected to land in a
    // specific (already-running) instance. Populated by callers that
    // know a player's destination team ahead of the player physically
    // arriving — currently only the MISSION_ENDED handler for the
    // continue_in_queue successor flow, but the keyed-by-instance_id
    // shape is intentionally generic so future "queue waits for
    // players" mechanics can reuse it.
    //
    // SetPreAssignedTeam writes; ConsumePreAssignedTeam reads-and-
    // removes a single entry; DropPreAssignedTeams clears every entry
    // for a dead instance (called from InstanceRegistry::MarkStopped).
    static void SetPreAssignedTeam(int64_t instance_id,
                                   const std::string& guid, int tf);
    static std::optional<int>
        ConsumePreAssignedTeam(int64_t instance_id, const std::string& guid);
    static void DropPreAssignedTeams(int64_t instance_id);

    // Cancel a pending match because its backing instance died before becoming
    // useful (spawn failure, or crash before INSTANCE_READY). Erases the
    // pending entry and unsticks each matched player via DeliverMatchCancelled
    // so the next GET_TICKET_INFO refresh shows them as not queued. No-op if
    // no pending entry exists for this instance_id — covers the common case
    // of a healthy instance dying mid-mission (already consumed at READY).
    static void DiscardPendingMatchForDeadInstance(int64_t instance_id, const char* reason);

    // Read-only accessor used by TcpSession's GSC_CHANGE_INSTANCE handler to
    // decide between routing a survivor home vs into a successor instance.
    // Returns continue_in_queue=false for unknown queues.
    static bool GetContinueInQueue(uint32_t queue_id);

    // Snapshot copy of all enabled QueueConfigs, sorted by sort_order then
    // queue_id. Used by send_get_ticket_info_response to enumerate cards.
    static std::vector<QueueConfig> GetEnabledQueueConfigs();

    // Look up a single queue's config (any enabled-state). Returns nullopt
    // if the queue_id isn't registered.
    static std::optional<QueueConfig> GetQueueConfig(uint32_t queue_id);

    // Per-profile breakdown of players currently sitting in this queue
    // (waiting for a match-pop). Returns a 4-tuple in the order
    // {ASSAULT, MEDIC, RECON, ROBOTICS}; profiles outside the four
    // dashboard classes don't contribute. RuntimeStats sums this with the
    // in-mission breakdown from InstanceRegistry.
    struct QueuedProfileCounts { uint32_t assault, medic, recon, robotics; };
    static QueuedProfileCounts GetQueuedProfileCounts(uint32_t queue_id);

    // Live countdown in whole seconds (ceiling) until this queue's active
    // pop-delay timer fires. Returns nullopt when the queue has no active
    // delay (or has already passed its fires_at, meaning the timer is
    // about to be processed). Consumed by TicketInfoEncoder to repurpose
    // the existing REMAINING_SECONDS wire field as a live pop-delay
    // surface on the GET_TICKET_INFO queue cards.
    static std::optional<uint32_t>
        GetDelayedPopRemainingSeconds(uint32_t queue_id);

    // Random pick from a queue's map_pool filtered by player count.
    // Prefers entries whose [min_players, max_players] window (NULL =
    // unbounded on that side) contains `count`. Weighted random among the
    // strict subset; if empty, nearest-fit by window distance with
    // weighted random among ties. Returns nullopt only if the queue is
    // unknown or its pool is empty. Used by TryPop on "spawn new" and by
    // the SuccessorSpawner closure in main.cpp on pre-warm.
    static std::optional<MapModeSpec>
        PickRandomMapPoolEntryForCount(uint32_t queue_id, int count);

private:
    // Per-queue wait state for pop_delay_seconds. Lives on Queue. The
    // timer is shared so async_wait completion handlers can keep it alive
    // long enough to safely cancel even after the queue's Queue struct is
    // destroyed (the handler checks for queue+timer identity before firing).
    struct DelayedPop {
        float next_duration = 0.0f;  // halves on each reset (floor 0.5s)
        std::shared_ptr<asio::steady_timer> timer;
        std::chrono::steady_clock::time_point fires_at;
    };

    struct Queue {
        QueueConfig config;
        std::unique_ptr<MatchRule> rule;
        std::vector<QueuedPlayer> players;
        std::optional<DelayedPop> delayed_pop;  // nullopt = no active wait
    };

    static std::unordered_map<uint32_t, Queue> queues_;
    static MatchPopCallback on_match_pop_;
    static InstanceProvider instance_provider_;
    static std::unordered_map<int64_t, PendingMatch> pending_matches_;
    static std::unordered_map<int64_t,
                              std::unordered_map<std::string, int>>
        pre_assigned_teams_;
    static asio::io_context* io_ctx_;

    static void TryPop(uint32_t queue_id, bool delay_elapsed = false);

    // Called from AddPlayer / RemovePlayer when the queue currently has
    // an active delayed_pop. Halves next_duration (floor 0.5s) and re-
    // arms the timer; OR cancels the wait entirely if the join/leave
    // dropped the queue below min_players_to_pop.
    static void MaybeResetDelayedPop(Queue& q,
                                      const char* trigger,
                                      const std::string& guid);

    // Internal: build a Queue (config + rule via factory) from a config row.
    // Caller is responsible for inserting the result into queues_.
    static Queue BuildQueue(QueueConfig cfg);
};
