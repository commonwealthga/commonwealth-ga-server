#pragma once

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
    Pinned1,   // everyone -> TF 1 (SpecOps / Solo coop)
    Pinned2,   // everyone -> TF 2 (Defense)
    Balanced,  // place each player on the smaller of {team1, team2}
};

struct QueueConfig {
    uint32_t queue_id = 0;
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
    uint64_t access_flags = 0;
    bool active_flag = true;
    bool locked_flag = false;
    std::optional<uint32_t> remaining_seconds;  // nullopt -> field omitted

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

    // Wipe and re-load every queue from ga_queues + ga_queue_map_pool.
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

    static void AddPendingMatch(int64_t instance_id, PendingMatch match);
    static std::optional<PendingMatch> ConsumePendingMatch(int64_t instance_id);

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

    // Random pick from a queue's map_pool, weighted by MapModeEntry.weight.
    // Returns nullopt if the queue is unknown or its pool is empty (matches
    // legacy nullopt-means-misconfig behaviour). Used by TryPop on "spawn
    // new" and by the SuccessorSpawner closure in main.cpp on pre-warm.
    static std::optional<MapModeSpec> PickRandomMapPoolEntry(uint32_t queue_id);

private:
    struct Queue {
        QueueConfig config;
        std::unique_ptr<MatchRule> rule;
        std::vector<QueuedPlayer> players;
    };

    static std::unordered_map<uint32_t, Queue> queues_;
    static MatchPopCallback on_match_pop_;
    static InstanceProvider instance_provider_;
    static std::unordered_map<int64_t, PendingMatch> pending_matches_;

    static void TryPop(uint32_t queue_id);

    // Internal: build a Queue (config + rule via factory) from a config row.
    // Caller is responsible for inserting the result into queues_.
    static Queue BuildQueue(QueueConfig cfg);
};
