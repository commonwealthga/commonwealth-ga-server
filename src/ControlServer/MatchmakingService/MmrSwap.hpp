#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

// MMR post-pass for BalancedPvp placement. Pure — no statics, no DB.
// Only same-class swaps between swappable players, so class counts, team
// sizes and heal scores are invariant: MMR can never trade against them.
namespace MmrSwap {

// Relative importance of PER-CLASS mean-MMR balance versus overall mean-MMR
// balance. Equal team totals still lose matches when one side owns all the
// good medics and the other all the good assaults, so the per-class term
// leads. Both terms are in MMR points, so this is a direct ratio.
inline constexpr double kClassBalanceWeight = 2.0;

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
    // Same seed, broken down per class. Drives the per-class term so a batch
    // joining a live match corrects that match's per-class skew instead of
    // only its totals. Empty = fresh match / caller has no breakdown, in
    // which case the per-class term scores the batch alone.
    std::unordered_map<uint32_t, double> class_mmr_tf1;
    std::unordered_map<uint32_t, double> class_mmr_tf2;
    std::unordered_map<uint32_t, int>    class_n_tf1;
    std::unordered_map<uint32_t, int>    class_n_tf2;
};

// Repeatedly applies the same-class cross-team swap that most reduces
// |seed_diff + sum(tf1 mmr) - sum(tf2 mmr)| until none strictly improves.
// `assignment`: guid -> 1|2, mutated in place. `seed_diff` is the MMR-sum
// difference (tf1 - tf2) of players NOT in the list — e.g. the live roster
// of an in-progress match the batch is joining. Returns number of swaps.
// Overall-MMR only; prefer BalanceByMmrOptimal, which is per-class aware and
// cannot stall at a local minimum.
int BalanceByMmr(const std::vector<Player>& players,
                 std::unordered_map<std::string, int>& assignment,
                 double seed_diff = 0.0);

// Headcount-weighted mean of the per-class mean-MMR gaps, in MMR points.
// 0 = every class is as strong on one side as on the other.
double ClassMmrGap(const std::vector<Player>& players,
                   const std::unordered_map<std::string, int>& assignment,
                   const SeedContext& seed = {});

// Overall mean-MMR gap |mean(tf1) - mean(tf2)|, in MMR points.
double OverallMmrGap(const std::vector<Player>& players,
                     const std::unordered_map<std::string, int>& assignment,
                     const SeedContext& seed = {});

// The objective BalanceByMmrOptimal minimises:
//   kClassBalanceWeight * ClassMmrGap + OverallMmrGap.
double BalanceCost(const std::vector<Player>& players,
                   const std::unordered_map<std::string, int>& assignment,
                   const SeedContext& seed = {});

// Globally optimal same-class reassignment: minimises BalanceCost while
// preserving per-class team counts and respecting unswappable (party)
// players. Mutates `assignment` in place. Returns the number of players
// whose team changed.
int BalanceByMmrOptimal(const std::vector<Player>& players,
                        std::unordered_map<std::string, int>& assignment,
                        const SeedContext& seed = {});

}  // namespace MmrSwap
