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
};

struct PendingMatch {
    int64_t instance_id;
    uint32_t queue_id;
    std::string game_mode;
    std::vector<std::string> session_guids;
    std::unordered_map<std::string, int> task_force_assignments;  // session_guid -> task_force
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

// One (map, game_mode) pair the queue can spawn. A queue's map_pool is a list
// of these — every fresh spawn (initial match-pop or successor) picks one at
// random. Pool of size 1 effectively disables randomization.
struct MapModeSpec {
    std::string map_name;
    std::string game_mode;
};

// ---------------------------------------------------------------------------
// Per-queue config knobs evaluated outside the rule (post-match behavior, etc).
// Pass via RegisterQueue's second overload; defaults are vanilla "spawn one
// match and dump everyone home after."
// ---------------------------------------------------------------------------
struct QueueOptions {
    // When the parent mission of this queue finishes, route survivors into a
    // pre-warmed successor instance of the same queue (instead of home). The
    // successor is spawned at the game-mode-specific pre-warm trigger via
    // MSG_REQUEST_SUCCESSOR and unlocked at MSG_MISSION_ENDED. Falls back to
    // home if no successor exists or it's already full at click time.
    bool continue_in_queue = false;

    // Maps/modes this queue is allowed to spawn. The matchmaker picks one at
    // random whenever a fresh instance is needed (no existing-instance route
    // available, or pre-warming a successor). Empty pool = misconfiguration;
    // spawn attempts will log + abort. Rules can still hardcode a map by
    // setting MatchResult.map_name themselves — pool selection only kicks in
    // when the rule leaves map_name empty.
    std::vector<MapModeSpec> map_pool;
};

// ---------------------------------------------------------------------------
// MatchmakingService -- collects players into queues, evaluates rules
// ---------------------------------------------------------------------------

class MatchmakingService {
public:
    using MatchPopCallback = std::function<void(uint32_t queue_id, MatchResult result)>;
    // queue_id-scoped lookup: the provider returns READY mission instances
    // belonging to this queue only. Filter happens at provider call site so
    // rules can iterate `instances` without re-filtering. Was a no-arg lookup
    // back when rules hardcoded map/mode; now rules join any seat-available
    // instance in their queue regardless of which map it's on.
    using InstanceProvider = std::function<std::vector<RunningInstance>(uint32_t queue_id)>;

    static void Init();

    static void RegisterQueue(uint32_t queue_id, std::unique_ptr<MatchRule> rule);
    static void RegisterQueue(uint32_t queue_id, std::unique_ptr<MatchRule> rule, QueueOptions opts);

    static void AddPlayer(uint32_t queue_id, const QueuedPlayer& player);
    static void RemovePlayer(const std::string& session_guid);

    static void SetMatchPopCallback(MatchPopCallback cb);
    static void SetInstanceProvider(InstanceProvider provider);

    static void AddPendingMatch(int64_t instance_id, PendingMatch match);
    static std::optional<PendingMatch> ConsumePendingMatch(int64_t instance_id);

    // Read-only accessor used by TcpSession's GSC_CHANGE_INSTANCE handler to
    // decide between routing a survivor home vs into a successor instance.
    // Returns the registered options for the queue, or defaults if unknown
    // (defaults = return_home behavior).
    static QueueOptions GetQueueOptions(uint32_t queue_id);

    // Random pick from a queue's map_pool. Returns nullopt if the queue is
    // unknown or its pool is empty (misconfiguration — callers log and abort
    // the spawn). Used by TryPop on "spawn new" and by the SuccessorSpawner
    // closure in main.cpp on pre-warm.
    static std::optional<MapModeSpec> PickRandomMapPoolEntry(uint32_t queue_id);

private:
    struct Queue {
        std::unique_ptr<MatchRule> rule;
        std::vector<QueuedPlayer> players;
        QueueOptions opts;
    };

    static std::unordered_map<uint32_t, Queue> queues_;
    static MatchPopCallback on_match_pop_;
    static InstanceProvider instance_provider_;
    static std::unordered_map<int64_t, PendingMatch> pending_matches_;

    static void TryPop(uint32_t queue_id);
};
