#pragma once
//
// DefenderRotation — PURE defender-side selection for Double Agent
// (.planning/2026-09-07-matchmaking-fairness-log-design.md §7).
// No I/O, no singletons; unit-tested in tests/matchmaking.
//
#include "src/ControlServer/MatchmakingService/Domain.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace DefenderRotation {

// Split `chosen` into attackers (TF1, tf1_n seats) and defenders (TF2,
// tf2_n seats). Defender seats go to the strongest claim: never defended
// first, then least recently, then smallest defender share. A party is never
// split when some party subset sums to exactly tf2_n; when none does, one
// party spills across the boundary and *out_split is set.
// Returns guid -> task force for every member of every chosen party.
std::unordered_map<std::string, int> Assign(
    const std::vector<const QueuedParty*>& chosen,
    int tf1_n, int tf2_n, bool* out_split = nullptr);

}  // namespace DefenderRotation
