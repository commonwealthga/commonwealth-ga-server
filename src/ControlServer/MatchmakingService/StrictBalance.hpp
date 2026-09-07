#pragma once
//
// StrictBalance — PURE helpers for strict class-equal matchmaking
// (merc tightening, .planning/2026-08-16-merc-strict-matchmaking-design.md).
// No I/O, no singletons; unit-tested in tests/matchmaking.
//
#include "src/ControlServer/MatchmakingService/Domain.hpp"

#include <vector>

namespace StrictBalance {

// One planned admission into a live BACKFILL_ONLY instance.
struct Invite {
    const QueuedParty* party = nullptr;  // always a solo party
    int tf = 0;                          // side to join (1/2)
};

// Priority order used by every strict-mode decision: exclusion_count desc,
// then exclusion_rate desc (spread the cost proportionally), then joined_at
// asc. Party value = max over members on both counters. Stable.
std::vector<const QueuedParty*> PartiesByPriority(
    const std::vector<QueuedParty>& parties);

// Largest party-atomic subset whose per-class counts are all even, total
// <= cap (cap <= 0 = unlimited). Greedy add in priority order, then odd-class
// repair (drop lowest-priority solo of the odd class, else lowest-priority
// party containing it). Exact for solo-only pools; heuristic with parties.
std::vector<const QueuedParty*> SelectClassEqualSubset(
    const std::vector<QueuedParty>& parties, int cap);

// Class-repair backfill: for each class one side has fewer of, one queued
// SOLO per missing seat (priority order), pre-assigned to the short side.
// Empty when the sides are class-equal. Respects inst.seats_free().
std::vector<Invite> PlanDeficitBackfill(
    const std::vector<QueuedParty>& parties, const RunningInstance& inst);

// Pair admission: only when sides are class- AND size-equal. First (by
// priority order) same-class solo pair whose better orientation either does
// not widen the sides' mean-MMR difference, or leaves it under the 100-point
// absolute slack; both invited, one per side. Empty when no pair qualifies or
// fewer than 2 seats are free.
std::vector<Invite> PlanPairJoin(
    const std::vector<QueuedParty>& parties, const RunningInstance& inst);

}  // namespace StrictBalance
