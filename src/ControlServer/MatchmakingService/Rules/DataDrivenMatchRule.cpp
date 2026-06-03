#include "src/ControlServer/MatchmakingService/Rules/DataDrivenMatchRule.hpp"
#include "src/ControlServer/MatchmakingService/RoleWeightedSplit.hpp"
#include "src/ControlServer/InstanceRegistry/InstanceRegistry.hpp"

namespace {

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
        if (r.task_force == 1) { s1.heal_score += v; s1.size += 1; }
        else                   { s2.heal_score += v; s2.size += 1; }
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
        MatchResult result;
        result.map_name  = inst.map_name;
        result.game_mode = inst.game_mode;
        result.existing_instance_id = inst.instance_id;

        if (cfg_.taskforce_policy == TaskforcePolicy::BalancedPvp) {
            auto seeds = SeedFromInstance(inst.instance_id);
            AssignTaskForcesBalancedPvp(players, seeds.first, seeds.second, result);
        } else {
            int t1 = 0, t2 = 0;
            if (cfg_.taskforce_policy == TaskforcePolicy::Balanced) {
                auto counts = InstanceRegistry::GetTeamCounts(inst.instance_id);
                t1 = counts.first;
                t2 = counts.second;
            }
            AssignTaskForcesLegacy(cfg_, players, t1, t2, result);
        }
        return result;
    }

    // No joinable instance — fresh spawn. Leave map/mode empty so
    // MatchmakingService fills them via PickRandomMapPoolEntryForCount.
    MatchResult result;
    if (cfg_.taskforce_policy == TaskforcePolicy::BalancedPvp) {
        AssignTaskForcesBalancedPvp(players, {}, {}, result);
    } else {
        AssignTaskForcesLegacy(cfg_, players, 0, 0, result);
    }
    return result;
}
