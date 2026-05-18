#include "src/ControlServer/MatchmakingService/MatchmakingService.hpp"
#include "src/ControlServer/Logger.hpp"
#include <algorithm>
#include <random>

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
    RegisterQueue(queue_id, std::move(rule), QueueOptions{});
}

void MatchmakingService::RegisterQueue(uint32_t queue_id, std::unique_ptr<MatchRule> rule, QueueOptions opts) {
    Queue q;
    q.rule = std::move(rule);
    q.opts = opts;
    queues_[queue_id] = std::move(q);
    Logger::Log("matchmaking", "[Matchmaking] Registered queue %u (continue_in_queue=%d)\n",
        queue_id, (int)opts.continue_in_queue);
}

QueueOptions MatchmakingService::GetQueueOptions(uint32_t queue_id) {
    auto it = queues_.find(queue_id);
    if (it == queues_.end()) return QueueOptions{};
    return it->second.opts;
}

// Single shared RNG. Control server runs single-threaded asio so no mutex
// needed; seeded once from random_device on first use.
static std::mt19937& MatchRng() {
    static std::mt19937 eng{std::random_device{}()};
    return eng;
}

std::optional<MapModeSpec> MatchmakingService::PickRandomMapPoolEntry(uint32_t queue_id) {
    auto it = queues_.find(queue_id);
    if (it == queues_.end()) {
        Logger::Log("matchmaking",
            "[Matchmaking] PickRandomMapPoolEntry: queue %u not registered\n", queue_id);
        return std::nullopt;
    }
    const auto& pool = it->second.opts.map_pool;
    if (pool.empty()) {
        Logger::Log("matchmaking",
            "[Matchmaking] PickRandomMapPoolEntry: queue %u has empty map_pool\n", queue_id);
        return std::nullopt;
    }
    std::uniform_int_distribution<size_t> dist(0, pool.size() - 1);
    return pool[dist(MatchRng())];
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

    // Queue-scoped instance list — the provider filters by queue_id so rules
    // don't see (or accidentally route into) instances from other queues.
    std::vector<RunningInstance> instances;
    if (instance_provider_) {
        instances = instance_provider_(queue_id);
    }

    auto result = queue.rule->Evaluate(queue.players, instances);
    if (!result) return;

    // Rule signaled "spawn new" without committing to a map — fill from pool.
    // (Rules can still hardcode a map by setting result.map_name themselves;
    // we only overwrite when both map_name AND existing_instance_id are unset.)
    if (!result->existing_instance_id && result->map_name.empty()) {
        auto picked = PickRandomMapPoolEntry(queue_id);
        if (!picked) {
            Logger::Log("matchmaking",
                "[Matchmaking] Queue %u rule wants new spawn but map_pool is empty/unconfigured — skipping pop\n",
                queue_id);
            return;
        }
        result->map_name  = picked->map_name;
        result->game_mode = picked->game_mode;
    }

    // Coalesce-into-pending check. If the rule wants a fresh spawn but we
    // already have a pending match for this queue (an instance that was
    // spawned by a recent pop and hasn't reported INSTANCE_READY yet), append
    // the new players to that pending match instead of spawning a duplicate.
    //
    // This prevents the classic race: N players hit a cold queue
    // simultaneously, instance_provider_ only returns READY instances, so
    // every TryPop after the first sees an empty instance list and the rule
    // keeps asking for a fresh spawn — ending with N solo matches instead
    // of one populated match.
    //
    // The appended players get invitations together with the original pending
    // players when INSTANCE_READY arrives (IpcServer's MSG_INSTANCE_READY
    // handler iterates session_guids unconditionally). Task force is
    // rebalanced against whatever's already pending on the instance.
    if (!result->existing_instance_id) {
        std::optional<int64_t> pending_iid;
        for (const auto& [iid, pm] : pending_matches_) {
            if (pm.queue_id == queue_id) { pending_iid = iid; break; }
        }
        if (pending_iid) {
            auto& pm = pending_matches_[*pending_iid];

            int team1 = 0, team2 = 0;
            for (const auto& [_, tf] : pm.task_force_assignments) {
                if (tf == 1) team1++; else team2++;
            }

            int appended = 0;
            for (const auto& guid : result->session_guids) {
                if (pm.task_force_assignments.count(guid)) continue;  // defensive
                int tf = (team1 <= team2) ? 1 : 2;
                pm.session_guids.push_back(guid);
                pm.task_force_assignments[guid] = tf;
                if (tf == 1) team1++; else team2++;
                appended++;
            }

            // Remove the appended players from the queue (same as the normal
            // post-rule cleanup below).
            for (const auto& guid : result->session_guids) {
                auto& players = queue.players;
                players.erase(
                    std::remove_if(players.begin(), players.end(),
                        [&](const QueuedPlayer& p) { return p.session_guid == guid; }),
                    players.end());
            }

            Logger::Log("matchmaking",
                "[Matchmaking] Queue %u: coalesced %d player(s) into pending instance %lld "
                "(teams now %d/%d, total %zu) — no new spawn\n",
                queue_id, appended, (long long)*pending_iid, team1, team2, pm.session_guids.size());

            // Do NOT call on_match_pop_ — the spawned instance already exists
            // and its MATCH_INVITATIONs will go out when INSTANCE_READY fires.
            return;
        }
    }

    // Remove matched players from queue
    for (const auto& guid : result->session_guids) {
        auto& players = queue.players;
        players.erase(
            std::remove_if(players.begin(), players.end(),
                [&](const QueuedPlayer& p) { return p.session_guid == guid; }),
            players.end());
    }

    Logger::Log("matchmaking", "[Matchmaking] Queue %u popped: %zu players, map=%s mode=%s\n",
        queue_id, result->session_guids.size(), result->map_name.c_str(), result->game_mode.c_str());

    if (on_match_pop_) {
        on_match_pop_(queue_id, std::move(*result));
    }
}
