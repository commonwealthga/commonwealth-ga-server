#pragma once
//
// DoubleAgentRule — asymmetric "Double Agent" coop. Party-aware.
//
//   • Total roster ∈ [2, 10]; a shape table maps total -> (TF1, TF2) where
//     TF1 = attackers (PvE coop side) and TF2 = defenders (the agents).
//   • Multiple parties + solos are matched into ONE match. Teammates are kept
//     on the same side when the fixed shape allows; a party is split across the
//     TF boundary only when a chunk can't fit whole.
//   • Selection is longest-waiting-first (deterministic; fair to early joiners).
//   • SEALED: every pop spawns a fresh instance and nobody — not even an
//     invitee — joins it afterwards (cap_override = popped size + Sealed).
//   • Map/mode left empty -> filled from the queue's weighted map_pool.
//   • Ignores cfg.taskforce_policy — the shape table wins.
//
#include "src/ControlServer/MatchmakingService/MatchRule.hpp"

class DoubleAgentRule : public MatchRule {
public:
    explicit DoubleAgentRule(const QueueConfig* cfg) : cfg_(cfg ? *cfg : QueueConfig{}) {}

    std::optional<MatchResult> Evaluate(
        const std::vector<QueuedParty>& parties,
        const std::vector<RunningInstance>& instances) override;

private:
    QueueConfig cfg_;
};
