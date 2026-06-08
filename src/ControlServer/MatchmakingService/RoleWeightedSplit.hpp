#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

// Class-aware team placement for the BalancedPvp policy. Pure helper —
// no static state, no DB access. Callers feed it rosters + team seeds,
// get back guid→team assignments (1 or 2).

namespace RoleWeightedSplit {

// Tunables. Single source of truth for playtest re-tuning. Documented
// values match .planning/2026-06-03-matchmaking-control-server-fixes-design.md.
inline constexpr float kAlpha            = 1.0f;  // heal-imbalance weight
inline constexpr float kBeta             = 0.5f;  // size-imbalance weight
inline constexpr float kGamma            = 0.3f;  // co-aligned penalty
inline constexpr float kRoboticsDefBonus = 0.10f; // robotics TF2 bonus

// Per-class-count imbalance weight. PRIMARY objective: split every class
// evenly. Must dominate the heal/size/co-aligned terms so a 1-player class
// imbalance always outweighs any heal-score or size benefit. Keep it well
// above the max achievable (kAlpha*healDiff + kBeta*sizeDiff + kGamma*coAligned)
// for your largest match (~37 for a 12-player match) — 1000 gives wide margin.
inline constexpr float kDelta            = 1000.0f; // per-class-imbalance weight

// Engine PROFILE_* values (mirror MatchmakingService::GetQueuedProfileCounts).
inline constexpr uint32_t kProfileAssault  = 680;
inline constexpr uint32_t kProfileMedic    = 567;
inline constexpr uint32_t kProfileRecon    = 681;
inline constexpr uint32_t kProfileRobotics = 679;

// Medics 1.0 on both teams; robotics 0.75 attackers / 0.85 defenders;
// everyone else 0.0.
float HealValue(uint32_t profile_id, int task_force);

struct PlayerSlot {
    std::string guid;
    uint32_t    profile_id;
};

struct TeamState {
    float heal_score = 0.0f;
    int   size       = 0;
    // Per-class headcount currently on this team. Drives the per-class balance
    // term. Empty seed = no prior players (fresh match / rebalance).
    std::unordered_map<uint32_t, int> class_counts;
};

// Sort desc by max(HealValue on TF1, HealValue on TF2); same-tier order
// randomized to avoid systematic TF1 bias. Greedy-place each via the
// spec's cost function. Returns guid → task_force (1 or 2) for every
// player in `players`.
std::unordered_map<std::string, int>
ComputeBatchAssignment(const std::vector<PlayerSlot>& players,
                       TeamState seed1,
                       TeamState seed2);

// Single-player placement. Used when no batch context exists (cache
// miss on successor join, or DataDrivenMatchRule with a pool of size 1).
int PlaceSingle(uint32_t profile_id, TeamState team1, TeamState team2);

// A spawned player's identity + the team they are CURRENTLY on (1 or 2).
struct RosterEntry {
    std::string guid;
    uint32_t    profile_id;
    int         current_tf;
};

// Minimal-swap delta. Computes the ideal per-(team,class) target counts, then
// moves ONLY the surplus of each class from its over-target side to the other.
// Returns guid -> new_tf for ONLY the players that must change. Empty when the
// roster is already balanced. This is the WIRED rebalance compute method.
std::unordered_map<std::string, int>
ComputeRebalanceDelta(const std::vector<RosterEntry>& roster);

// Full recompute. Reassigns everyone from scratch (per-class cost), then diffs
// against current teams. Built for testing / future use — NOT wired.
std::unordered_map<std::string, int>
ComputeFullReassignDelta(const std::vector<RosterEntry>& roster);

// Trigger gates. AnyImprovement = act iff the delta is non-empty (sensitive to
// the full cost incl. heal fallback). DiscreteThreshold = act only on integer
// lopsidedness (size diff >= N, or any single class diff >= N). Both built;
// flip kActiveGate to switch.
enum class RebalanceGate { AnyImprovement, DiscreteThreshold };
inline constexpr RebalanceGate kActiveGate        = RebalanceGate::AnyImprovement;
inline constexpr int           kDiscreteThreshold = 2;

// True if the active gate says this roster is imbalanced enough to act.
bool ShouldRebalance(const std::vector<RosterEntry>& roster,
                     const std::unordered_map<std::string, int>& delta);

}  // namespace RoleWeightedSplit
