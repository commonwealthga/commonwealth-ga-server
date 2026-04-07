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

// ---------------------------------------------------------------------------
// MatchmakingService -- collects players into queues, evaluates rules
// ---------------------------------------------------------------------------

class MatchmakingService {
public:
    using MatchPopCallback = std::function<void(uint32_t queue_id, MatchResult result)>;
    using InstanceProvider = std::function<std::vector<RunningInstance>()>;

    static void Init();

    static void RegisterQueue(uint32_t queue_id, std::unique_ptr<MatchRule> rule);

    static void AddPlayer(uint32_t queue_id, const QueuedPlayer& player);
    static void RemovePlayer(const std::string& session_guid);

    static void SetMatchPopCallback(MatchPopCallback cb);
    static void SetInstanceProvider(InstanceProvider provider);

    static void AddPendingMatch(int64_t instance_id, PendingMatch match);
    static std::optional<PendingMatch> ConsumePendingMatch(int64_t instance_id);

private:
    struct Queue {
        std::unique_ptr<MatchRule> rule;
        std::vector<QueuedPlayer> players;
    };

    static std::unordered_map<uint32_t, Queue> queues_;
    static MatchPopCallback on_match_pop_;
    static InstanceProvider instance_provider_;
    static std::unordered_map<int64_t, PendingMatch> pending_matches_;

    static void TryPop(uint32_t queue_id);
};
