#include "src/ControlServer/MatchmakingService/Rules/DataDrivenMatchRule.hpp"
#include "src/ControlServer/MatchmakingService/RoleWeightedSplit.hpp"
#include "src/ControlServer/InstanceRegistry/InstanceRegistry.hpp"
#include <algorithm>

namespace {

std::vector<QueuedPlayer> FirstN(const std::vector<QueuedPlayer>& players, size_t count) {
    count = std::min(count, players.size());
    return std::vector<QueuedPlayer>(players.begin(), players.begin() + count);
}

// Legacy placement: greedy-by-size or pinned. Unchanged behavior for
// Pinned1/Pinned2/Balanced. BalancedPvp is routed through the role-
// weighted batch path in Evaluate and never reaches this function.
void AssignTaskForcesLegacy(const QueueConfig& cfg,
                            const std::vector<QueuedPlayer>& players,
                            int seed_team1, int seed_team2,
                            MatchResult& out)
{
    int team1 = seed_team1, team2 = seed_team2;
    for (const auto& p : players) {
        int tf = 1;
        switch (cfg.taskforce_policy) {
            case TaskforcePolicy::Pinned1:     tf = 1; break;
            case TaskforcePolicy::Pinned2:     tf = 2; break;
            case TaskforcePolicy::Balanced:    tf = (team1 <= team2) ? 1 : 2; break;
            case TaskforcePolicy::BalancedPvp: tf = (team1 <= team2) ? 1 : 2; break;  // unreachable
        }
        out.session_guids.push_back(p.session_guid);
        out.task_force_assignments[p.session_guid] = tf;
        out.profile_ids[p.session_guid] = p.profile_id;
        if (tf == 1) team1++; else team2++;
    }
}

void AssignTaskForcesBalancedPvp(const std::vector<QueuedPlayer>& players,
                                 RoleWeightedSplit::TeamState seed1,
                                 RoleWeightedSplit::TeamState seed2,
                                 MatchResult& out)
{
    std::vector<RoleWeightedSplit::PlayerSlot> slots;
    slots.reserve(players.size());
    for (const auto& p : players) {
        slots.push_back({p.session_guid, p.profile_id});
    }
    auto assignment = RoleWeightedSplit::ComputeBatchAssignment(slots, seed1, seed2);
    for (const auto& p : players) {
        auto it = assignment.find(p.session_guid);
        const int tf = it != assignment.end() ? it->second : 1;
        out.session_guids.push_back(p.session_guid);
        out.task_force_assignments[p.session_guid] = tf;
        out.profile_ids[p.session_guid] = p.profile_id;
    }
}

// Seed per-team heal_score + size from a live instance's roster. Used
// when joining players into an already-running instance under BalancedPvp.
std::pair<RoleWeightedSplit::TeamState, RoleWeightedSplit::TeamState>
SeedFromInstance(int64_t instance_id)
{
    RoleWeightedSplit::TeamState s1, s2;
    auto roster = InstanceRegistry::GetActivePlayersForInstance(instance_id);
    for (const auto& r : roster) {
        const float v = RoleWeightedSplit::HealValue(r.profile_id, r.task_force);
        if (r.task_force == 1) {
            s1.heal_score += v; s1.size += 1; s1.class_counts[r.profile_id] += 1;
        } else {
            s2.heal_score += v; s2.size += 1; s2.class_counts[r.profile_id] += 1;
        }
    }
    return {s1, s2};
}

}  // namespace

std::optional<MatchResult> DataDrivenMatchRule::Evaluate(
    const std::vector<QueuedPlayer>& players,
    const std::vector<RunningInstance>& instances)
{
    if (players.empty()) return std::nullopt;

    // Try to join the first READY instance for this queue (provider
    // already filtered by queue_id).
    for (const auto& inst : instances) {
        if (inst.max_players > 0 && inst.player_count >= inst.max_players) continue;
        const size_t seats = inst.max_players > 0
            ? (size_t)std::max(0, inst.max_players - inst.player_count)
            : players.size();
        auto routed_players = FirstN(players, seats);
        if (routed_players.empty()) continue;

        MatchResult result;
        result.map_name  = inst.map_name;
        result.game_mode = inst.game_mode;
        result.existing_instance_id = inst.instance_id;

        if (cfg_.taskforce_policy == TaskforcePolicy::BalancedPvp) {
            auto seeds = SeedFromInstance(inst.instance_id);
            seeds.first.size += inst.reserved_team1_size;
            seeds.first.heal_score += inst.reserved_team1_heal_score;
            seeds.second.size += inst.reserved_team2_size;
            seeds.second.heal_score += inst.reserved_team2_heal_score;
            AssignTaskForcesBalancedPvp(routed_players, seeds.first, seeds.second, result);
        } else {
            int t1 = 0, t2 = 0;
            if (cfg_.taskforce_policy == TaskforcePolicy::Balanced) {
                auto counts = InstanceRegistry::GetTeamCounts(inst.instance_id);
                t1 = counts.first + inst.reserved_team1_size;
                t2 = counts.second + inst.reserved_team2_size;
            }
            AssignTaskForcesLegacy(cfg_, routed_players, t1, t2, result);
        }
        return result;
    }

    // No joinable instance — fresh spawn. Leave map/mode empty so
    // MatchmakingService fills them via PickRandomMapPoolEntryForCount.
    size_t seats = players.size();
    if (uint32_t cap = MatchmakingService::GetQueueInstanceCap(cfg_); cap > 0) {
        seats = std::min(seats, (size_t)cap);
    }
    auto routed_players = FirstN(players, seats);
    if (routed_players.empty()) return std::nullopt;

    MatchResult result;
    if (cfg_.taskforce_policy == TaskforcePolicy::BalancedPvp) {
        AssignTaskForcesBalancedPvp(routed_players, {}, {}, result);
    } else {
        AssignTaskForcesLegacy(cfg_, routed_players, 0, 0, result);
    }
    return result;
}
