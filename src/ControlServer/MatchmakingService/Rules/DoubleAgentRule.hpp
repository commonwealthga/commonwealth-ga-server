#pragma once

#include "src/ControlServer/MatchmakingService/MatchmakingService.hpp"

// DoubleAgentRule — asymmetric "Double Agent" coop matchmaker.
//
//   • Total roster ∈ [2, 10]. The shape table maps total -> (TF1, TF2)
//     where TF1 = attackers (PvE coop side) and TF2 = defenders (the
//     agents). TF mapping is the engine convention (TgAIController.uc:
//     1315-1322 — AttackerAssaultNav on TF1, DefenderAssaultNav on TF2).
//
//   • Random assignment per pop. RNG is seeded per Evaluate from
//     steady_clock so back-to-back pops don't correlate.
//
//   • Late-join lockout. Never returns existing_instance_id — every pop
//     spawns a fresh instance. Sets MatchResult::cap_override to the
//     pop_count so MatchmakingService's coalesce loop refuses to append
//     more players to the resulting PendingMatch.
//
//   • Map / mode left empty so MatchmakingService fills from the queue's
//     weighted map_pool via PickRandomMapPoolEntryForCount.
//
//   • Ignores cfg.taskforce_policy — the shape table wins regardless.
class DoubleAgentRule : public MatchRule {
public:
    explicit DoubleAgentRule(const QueueConfig* cfg)
        : cfg_(cfg ? *cfg : QueueConfig{}) {}

    std::optional<MatchResult> Evaluate(
        const std::vector<QueuedPlayer>& players,
        const std::vector<RunningInstance>& instances) override;

private:
    QueueConfig cfg_;
};
