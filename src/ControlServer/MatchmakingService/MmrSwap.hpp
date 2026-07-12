#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

// MMR post-pass for BalancedPvp placement. Pure — no statics, no DB.
// Only same-class swaps between swappable players, so class counts, team
// sizes and heal scores are invariant: MMR can never trade against them.
namespace MmrSwap {

struct Player {
    std::string guid;
    uint32_t    profile_id = 0;
    double      mmr = 1000.0;
    bool        swappable = true;   // false = party member (never split)
};

// Repeatedly applies the same-class cross-team swap that most reduces
// |sum(tf1 mmr) - sum(tf2 mmr)| until none strictly improves.
// `assignment`: guid -> 1|2, mutated in place. Returns number of swaps.
int BalanceByMmr(const std::vector<Player>& players,
                 std::unordered_map<std::string, int>& assignment);

}  // namespace MmrSwap
