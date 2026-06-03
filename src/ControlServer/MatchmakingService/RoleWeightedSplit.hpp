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

}  // namespace RoleWeightedSplit
