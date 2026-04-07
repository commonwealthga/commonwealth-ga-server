#include "src/ControlServer/MatchmakingService/MatchmakingService.hpp"
#include "src/ControlServer/Logger.hpp"
#include <algorithm>

// ---------------------------------------------------------------------------
// Static members
// ---------------------------------------------------------------------------

std::unordered_map<uint32_t, MatchmakingService::Queue> MatchmakingService::queues_;
MatchmakingService::MatchPopCallback MatchmakingService::on_match_pop_;
MatchmakingService::InstanceProvider MatchmakingService::instance_provider_;
std::unordered_map<int64_t, PendingMatch> MatchmakingService::pending_matches_;

// ---------------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------------

void MatchmakingService::Init() {
    queues_.clear();
    pending_matches_.clear();
    on_match_pop_ = nullptr;
    instance_provider_ = nullptr;
    Logger::Log("matchmaking", "[Matchmaking] Initialized\n");
}

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------

void MatchmakingService::RegisterQueue(uint32_t queue_id, std::unique_ptr<MatchRule> rule) {
    Queue q;
    q.rule = std::move(rule);
    queues_[queue_id] = std::move(q);
    Logger::Log("matchmaking", "[Matchmaking] Registered queue %u\n", queue_id);
}

void MatchmakingService::SetMatchPopCallback(MatchPopCallback cb) {
    on_match_pop_ = std::move(cb);
}

void MatchmakingService::SetInstanceProvider(InstanceProvider provider) {
    instance_provider_ = std::move(provider);
}

// ---------------------------------------------------------------------------
// Player management
// ---------------------------------------------------------------------------

void MatchmakingService::AddPlayer(uint32_t queue_id, const QueuedPlayer& player) {
    auto it = queues_.find(queue_id);
    if (it == queues_.end()) {
        Logger::Log("matchmaking", "[Matchmaking] AddPlayer: unknown queue %u\n", queue_id);
        return;
    }

    it->second.players.push_back(player);
    Logger::Log("matchmaking", "[Matchmaking] Player %s joined queue %u (%zu players)\n",
        player.session_guid.c_str(), queue_id, it->second.players.size());

    TryPop(queue_id);
}

void MatchmakingService::RemovePlayer(const std::string& session_guid) {
    for (auto& [queue_id, queue] : queues_) {
        auto& players = queue.players;
        auto it = std::find_if(players.begin(), players.end(),
            [&](const QueuedPlayer& p) { return p.session_guid == session_guid; });
        if (it != players.end()) {
            Logger::Log("matchmaking", "[Matchmaking] Player %s removed from queue %u\n",
                session_guid.c_str(), queue_id);
            players.erase(it);
            return;
        }
    }
}

// ---------------------------------------------------------------------------
// Pending match management
// ---------------------------------------------------------------------------

void MatchmakingService::AddPendingMatch(int64_t instance_id, PendingMatch match) {
    Logger::Log("matchmaking", "[Matchmaking] Pending match added for instance %lld (%zu players)\n",
        (long long)instance_id, match.session_guids.size());
    pending_matches_[instance_id] = std::move(match);
}

std::optional<PendingMatch> MatchmakingService::ConsumePendingMatch(int64_t instance_id) {
    auto it = pending_matches_.find(instance_id);
    if (it == pending_matches_.end()) return std::nullopt;
    PendingMatch match = std::move(it->second);
    pending_matches_.erase(it);
    return match;
}

// ---------------------------------------------------------------------------
// Core logic
// ---------------------------------------------------------------------------

void MatchmakingService::TryPop(uint32_t queue_id) {
    auto it = queues_.find(queue_id);
    if (it == queues_.end()) return;

    auto& queue = it->second;
    if (!queue.rule || queue.players.empty()) return;

    // Gather running instance state
    std::vector<RunningInstance> instances;
    if (instance_provider_) {
        instances = instance_provider_();
    }

    auto result = queue.rule->Evaluate(queue.players, instances);
    if (!result) return;

    // Remove matched players from queue
    for (const auto& guid : result->session_guids) {
        auto& players = queue.players;
        players.erase(
            std::remove_if(players.begin(), players.end(),
                [&](const QueuedPlayer& p) { return p.session_guid == guid; }),
            players.end());
    }

    Logger::Log("matchmaking", "[Matchmaking] Queue %u popped: %zu players, map=%s\n",
        queue_id, result->session_guids.size(), result->map_name.c_str());

    if (on_match_pop_) {
        on_match_pop_(queue_id, std::move(*result));
    }
}
