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

// Live-roster context for join-in-progress placement. The batch being placed
// is optimized against these seed totals so mean MMR stays balanced.
struct SeedContext {
    double mmr_tf1 = 0.0;
    double mmr_tf2 = 0.0;
    int    n_tf1   = 0;
    int    n_tf2   = 0;
};

// Repeatedly applies the same-class cross-team swap that most reduces
// |seed_diff + sum(tf1 mmr) - sum(tf2 mmr)| until none strictly improves.
// `assignment`: guid -> 1|2, mutated in place. `seed_diff` is the MMR-sum
// difference (tf1 - tf2) of players NOT in the list — e.g. the live roster
// of an in-progress match the batch is joining. Returns number of swaps.
// Prefer BalanceByMmrOptimal for new code — this greedy pass can stall at
// local minima.
int BalanceByMmr(const std::vector<Player>& players,
                 std::unordered_map<std::string, int>& assignment,
                 double seed_diff = 0.0);

// Globally optimal same-class reassignment: minimizes mean-MMR gap
// |mean(tf1) - mean(tf2)| while preserving per-class team counts and
// respecting unswappable (party) players. Mutates `assignment` in place.
// Returns the number of players whose team changed.
int BalanceByMmrOptimal(const std::vector<Player>& players,
                        std::unordered_map<std::string, int>& assignment,
                        const SeedContext& seed = {});

}  // namespace MmrSwap
