#pragma once

#include "src/ControlServer/MatchmakingService/MatchmakingService.hpp"

// DataDrivenMatchRule — one rule class parameterised entirely by ga_queues:
//   • Join any seat-available instance in this queue (same behaviour as the
//     four ex-Simple* rules — all of them did this identically).
//   • Assign task force per QueueConfig::taskforce_policy:
//       Pinned1  -> every player to TF 1 (SpecOps / Solo coop)
//       Pinned2  -> every player to TF 2 (Defense)
//       Balanced -> place each player on the smaller of {team1, team2}
//                   (READY-instance team counts come from
//                    InstanceRegistry::GetTeamCounts).
//   • If no existing instance is joinable, leave map_name empty so the
//     MatchmakingService fills it from the queue's weighted map_pool.
class DataDrivenMatchRule : public MatchRule {
public:
    explicit DataDrivenMatchRule(const QueueConfig* cfg) : cfg_(cfg ? *cfg : QueueConfig{}) {}

    std::optional<MatchResult> Evaluate(
        const std::vector<QueuedPlayer>& players,
        const std::vector<RunningInstance>& instances) override;

private:
    QueueConfig cfg_;
};
