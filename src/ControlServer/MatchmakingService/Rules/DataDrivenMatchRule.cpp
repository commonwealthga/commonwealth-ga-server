#include "src/ControlServer/MatchmakingService/Rules/DataDrivenMatchRule.hpp"
#include "src/ControlServer/InstanceRegistry/InstanceRegistry.hpp"

namespace {

// Assign each new player a task force according to the queue's policy.
// For Balanced, team1/team2 are seeded from the target instance's current
// roster (or 0/0 when spawning fresh) and incremented as we place each
// player so the placement order matches what the original PvP rule did.
void AssignTaskForces(const QueueConfig& cfg,
                      const std::vector<QueuedPlayer>& players,
                      int seed_team1, int seed_team2,
                      MatchResult& out)
{
    int team1 = seed_team1, team2 = seed_team2;
    for (const auto& p : players) {
        int tf = 1;
        switch (cfg.taskforce_policy) {
            case TaskforcePolicy::Pinned1:  tf = 1; break;
            case TaskforcePolicy::Pinned2:  tf = 2; break;
            case TaskforcePolicy::Balanced:
                tf = (team1 <= team2) ? 1 : 2;
                break;
        }
        out.session_guids.push_back(p.session_guid);
        out.task_force_assignments[p.session_guid] = tf;
        out.profile_ids[p.session_guid] = p.profile_id;  // for queue-card class breakdowns
        if (tf == 1) team1++; else team2++;
    }
}

}  // namespace

std::optional<MatchResult> DataDrivenMatchRule::Evaluate(
    const std::vector<QueuedPlayer>& players,
    const std::vector<RunningInstance>& instances)
{
    if (players.empty()) return std::nullopt;

    // First READY instance in this queue (provider already filtered by
    // queue_id). Behaviour matches the original Simple* rules: no seat
    // cap, the game decides fills.
    for (const auto& inst : instances) {
        MatchResult result;
        result.map_name  = inst.map_name;
        result.game_mode = inst.game_mode;
        result.existing_instance_id = inst.instance_id;

        int t1 = 0, t2 = 0;
        if (cfg_.taskforce_policy == TaskforcePolicy::Balanced) {
            auto counts = InstanceRegistry::GetTeamCounts(inst.instance_id);
            t1 = counts.first;
            t2 = counts.second;
        }
        AssignTaskForces(cfg_, players, t1, t2, result);
        return result;
    }

    // No joinable instance — leave map/mode empty so MatchmakingService
    // fills them via PickRandomMapPoolEntry. Balanced seeds team counts
    // at 0/0 since the instance hasn't started yet.
    MatchResult result;
    AssignTaskForces(cfg_, players, 0, 0, result);
    return result;
}
