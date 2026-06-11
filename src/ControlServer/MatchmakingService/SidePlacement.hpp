#pragma once
//
// SidePlacement — PURE, deterministic task-force assignment for a single pop.
//
// Given a TaskforcePolicy (the algorithm) + a TeamSidePolicy (how hard to keep
// a party together) + the groups to place + the seed makeup of each side,
// returns guid -> task_force (1 or 2). No RNG: identical inputs always produce
// identical output, so rule behaviour is unit-testable.
//
// This is the single source of truth for heal scoring on the matchmaking path
// (the reserved-seed code in MatchmakingService feeds RunningInstance.teamN
// using the SAME HealValue, so placement scores a consistent picture).
//
#include "src/ControlServer/MatchmakingService/Domain.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace SidePlacement {

// Engine PROFILE_* ids (mirror Domain / GetQueuedProfileCounts).
inline constexpr uint32_t kProfileAssault  = 680;
inline constexpr uint32_t kProfileMedic    = 567;
inline constexpr uint32_t kProfileRecon    = 681;
inline constexpr uint32_t kProfileRobotics = 679;

// Cost weights. kClassWeight dominates so per-class balance is the primary
// objective (a 1-player class imbalance always outweighs any heal/size benefit);
// heal and size are secondary tie-breakers.
inline constexpr float kClassWeight = 1000.0f;
inline constexpr float kHealWeight  = 1.0f;
inline constexpr float kSizeWeight  = 1.0f;

// Per-class headcount imbalance between two sides (the primary objective,
// independent of heal/size). 0 = every class evenly split.
int ClassImbalance(const TeamSeed& a, const TeamSeed& b);

// Heal weight of a class on a given task force. Medics heal on both sides;
// robotics contribute a small amount (more on defence); everyone else 0.
float HealValue(uint32_t profile_id, int task_force);

// Imbalance cost between two team seeds. 0 = perfectly balanced.
float Imbalance(const TeamSeed& a, const TeamSeed& b);

struct Slot {
    std::string guid;
    uint32_t    profile_id = 0;
};

struct Group {
    uint64_t          party_id = 0;
    std::vector<Slot> members;
};

// Assign every slot in every group to TF1 or TF2.
//   Pinned1/Pinned2          -> everyone one side (side policy irrelevant).
//   Balanced (headcount)     -> greedy on smaller side; side policy controls
//                               whether a party is placed whole or split.
//   BalancedPvp (class-aware)-> cost-minimising; side policy controls grouping.
// seed1/seed2 prime the sides (e.g. the live roster of an instance being
// joined). Defaulted-empty seeds = fresh match.
std::unordered_map<std::string, int> Assign(
    TaskforcePolicy tf_policy,
    TeamSidePolicy  side_policy,
    const std::vector<Group>& groups,
    TeamSeed seed1 = {},
    TeamSeed seed2 = {});

}  // namespace SidePlacement
