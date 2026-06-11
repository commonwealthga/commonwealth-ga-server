#pragma once
//
// MatchRule — abstract per-queue matching logic. Operates on PARTIES (the unit
// of queueing), returning at most one MatchResult per Evaluate call. The
// orchestrator (MatchmakingService) removes the consumed parties and re-calls
// Evaluate until no further match is produced.
//
// Rules are PURE: a function of (parties, instances, cfg_). All live state the
// rule needs (instance rosters, seats, access mode) arrives pre-gathered on
// RunningInstance via the InstanceProvider. This keeps every rule unit-testable
// with hand-built inputs.
//
#include "src/ControlServer/MatchmakingService/Domain.hpp"

#include <optional>
#include <vector>

class MatchRule {
public:
    virtual ~MatchRule() = default;

    // `parties` are the eligible parties currently queued (pvp-verification
    // and team-policy filtering already applied upstream). `instances` are the
    // queue's candidate instances with live makeup + access mode populated.
    virtual std::optional<MatchResult> Evaluate(
        const std::vector<QueuedParty>& parties,
        const std::vector<RunningInstance>& instances) = 0;
};
