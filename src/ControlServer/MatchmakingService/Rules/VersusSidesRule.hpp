#pragma once
//
// VersusSidesRule — "1v1" private-match maker. A side is a whole party (a solo
// player or a team). Pairs the two longest-waiting parties against each other:
// party A -> TF1, party B -> TF2. The match is PARTY_LOCKED to both parties, so
// matchmaking never routes a stranger in — but the players can still invite
// outsiders mid-match (handled via the team-invite path, not here).
//
// Map/mode left empty -> filled from the queue's weighted map_pool by total
// player count.
//
#include "src/ControlServer/MatchmakingService/MatchRule.hpp"

class VersusSidesRule : public MatchRule {
public:
    explicit VersusSidesRule(const QueueConfig* cfg) : cfg_(cfg ? *cfg : QueueConfig{}) {}

    std::optional<MatchResult> Evaluate(
        const std::vector<QueuedParty>& parties,
        const std::vector<RunningInstance>& instances) override;

private:
    QueueConfig cfg_;
};
