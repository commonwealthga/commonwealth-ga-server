#pragma once
//
// CoopMatchRule — the general pool rule. Drives the archetypes that share
// "join an open instance, else spawn one":
//   • Standard PvE (medium/high/max/umax): team_policy=own_match — a team pop
//     spawns a fresh PARTY_LOCKED match; solos drop into OPEN instances only.
//   • Mercenary PvP: team_policy=mixed, side_policy=preferred, BalancedPvp.
//   • DDR defense: team_policy=mixed, side_policy=required, Pinned1.
//
// Parameterised entirely by QueueConfig (taskforce_policy + team_policy +
// team_side_policy). Pure — see MatchRule.hpp.
//
#include "src/ControlServer/MatchmakingService/MatchRule.hpp"

class CoopMatchRule : public MatchRule {
public:
    explicit CoopMatchRule(const QueueConfig* cfg) : cfg_(cfg ? *cfg : QueueConfig{}) {}

    std::optional<MatchResult> Evaluate(
        const std::vector<QueuedParty>& parties,
        const std::vector<RunningInstance>& instances) override;

private:
    // Place solo+team parties into the shared pool: join the first OPEN
    // instance with room, else signal a fresh OPEN spawn.
    std::optional<MatchResult> PlacePool(
        const std::vector<QueuedParty>& parties,
        const std::vector<RunningInstance>& instances) const;

    // A team's dedicated PARTY_LOCKED fresh match (standard-PvE own_match).
    MatchResult BuildOwnMatch(const QueuedParty& team) const;

    QueueConfig cfg_;
};
