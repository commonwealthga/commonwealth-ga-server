#include "src/ControlServer/MatchmakingService/RoleWeightedSplit.hpp"

#include "src/ControlServer/MatchmakingService/MmrSwap.hpp"
#include "src/ControlServer/Logger.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <random>
#include <set>

namespace RoleWeightedSplit {

namespace {

std::mt19937& Rng() {
    static std::mt19937 eng{std::random_device{}()};
    return eng;
}

// Headcount of `profile_id` currently on team `s` (0 if none).
int ClassCountOn(const TeamState& s, uint32_t profile_id) {
    auto it = s.class_counts.find(profile_id);
    return it == s.class_counts.end() ? 0 : it->second;
}

// Cost of placing a `profile_id` player onto team `t` given the pre-placement
// states s1/s2. Per-class term dominates (kDelta); heal/size/co-aligned only
// break ties among equal per-class outcomes.
float Cost(int t, uint32_t profile_id,
           const TeamState& s1, const TeamState& s2)
{
    const float v_on_1 = HealValue(profile_id, 1);
    const float v_on_2 = HealValue(profile_id, 2);

    const float post_h1 = s1.heal_score + (t == 1 ? v_on_1 : 0.0f);
    const float post_h2 = s2.heal_score + (t == 2 ? v_on_2 : 0.0f);
    const int   post_s1 = s1.size + (t == 1 ? 1 : 0);
    const int   post_s2 = s2.size + (t == 2 ? 1 : 0);
    const int   post_c1 = ClassCountOn(s1, profile_id) + (t == 1 ? 1 : 0);
    const int   post_c2 = ClassCountOn(s2, profile_id) + (t == 2 ? 1 : 0);

    const float heal_diff_signed = post_h1 - post_h2;
    const int   size_diff_signed = post_s1 - post_s2;
    const float co_aligned =
        std::max(0.0f, heal_diff_signed * static_cast<float>(size_diff_signed));

    return kDelta * static_cast<float>(std::abs(post_c1 - post_c2))
         + kAlpha * std::fabs(heal_diff_signed)
         + kBeta  * static_cast<float>(std::abs(size_diff_signed))
         + kGamma * co_aligned;
}

int GreedyPlaceOne(uint32_t profile_id, const TeamState& s1, const TeamState& s2)
{
    const float c1 = Cost(1, profile_id, s1, s2);
    const float c2 = Cost(2, profile_id, s1, s2);

    if (c1 < c2) return 1;
    if (c2 < c1) return 2;
    // Exact cost tie: nudge toward the lower-MMR side, then smaller side.
    if (s1.mmr_sum < s2.mmr_sum) return 1;
    if (s2.mmr_sum < s1.mmr_sum) return 2;
    if (s1.size < s2.size) return 1;
    if (s2.size < s1.size) return 2;
    return 1;
}

}  // namespace

float HealValue(uint32_t profile_id, int task_force) {
    switch (profile_id) {
        case kProfileMedic:    return 1.00f;
        case kProfileRobotics: return task_force == 2 ? (0.75f + kRoboticsDefBonus) : 0.75f;
        default:               return 0.00f;
    }
}

int PlaceSingle(uint32_t profile_id, TeamState team1, TeamState team2) {
    const int tf = GreedyPlaceOne(profile_id, team1, team2);
    Logger::Log("team-balance",
        "[RoleWeightedSplit] PlaceSingle profile=%u team1=(%.2f,%d) team2=(%.2f,%d) -> tf=%d\n",
        profile_id, team1.heal_score, team1.size,
        team2.heal_score, team2.size, tf);
    return tf;
}

std::unordered_map<std::string, int>
ComputeBatchAssignment(const std::vector<PlayerSlot>& players,
                       TeamState seed1,
                       TeamState seed2)
{
    std::unordered_map<std::string, int> out;
    out.reserve(players.size());
    if (players.empty()) return out;

    std::vector<PlayerSlot> sorted = players;
    std::shuffle(sorted.begin(), sorted.end(), Rng());
    std::stable_sort(sorted.begin(), sorted.end(),
        [](const PlayerSlot& a, const PlayerSlot& b) {
            const float va = std::max(HealValue(a.profile_id, 1),
                                      HealValue(a.profile_id, 2));
            const float vb = std::max(HealValue(b.profile_id, 1),
                                      HealValue(b.profile_id, 2));
            return va > vb;
        });

    TeamState s1 = seed1;
    TeamState s2 = seed2;

    Logger::Log("team-balance",
        "[RoleWeightedSplit] ComputeBatchAssignment players=%zu seed1=(%.2f,%d) seed2=(%.2f,%d)\n",
        players.size(), seed1.heal_score, seed1.size,
        seed2.heal_score, seed2.size);

    for (const auto& p : sorted) {
        const int tf = GreedyPlaceOne(p.profile_id, s1, s2);
        if (tf == 1) {
            s1.heal_score += HealValue(p.profile_id, 1);
            s1.size += 1;
            s1.class_counts[p.profile_id] += 1;
            s1.mmr_sum += p.mmr;
        } else {
            s2.heal_score += HealValue(p.profile_id, 2);
            s2.size += 1;
            s2.class_counts[p.profile_id] += 1;
            s2.mmr_sum += p.mmr;
        }
        out[p.guid] = tf;
        Logger::Log("team-balance",
            "[RoleWeightedSplit]   guid=%s profile=%u -> tf=%d state=(%.2f,%d)/(%.2f,%d)\n",
            p.guid.c_str(), p.profile_id, tf,
            s1.heal_score, s1.size, s2.heal_score, s2.size);
    }

    // MMR post-pass: same-class swaps only, so the class/heal/size balance
    // above is invariant. No party context on this path — all swappable.
    // Seeded joins compensate for the live rosters' imbalance.
    std::vector<MmrSwap::Player> mp;
    mp.reserve(players.size());
    for (const auto& p : players) mp.push_back({p.guid, p.profile_id, p.mmr, true});
    const int swaps = MmrSwap::BalanceByMmr(mp, out,
                                            seed1.mmr_sum - seed2.mmr_sum);
    double sum1 = 0.0, sum2 = 0.0;
    for (const auto& p : players) {
        if (out[p.guid] == 1) sum1 += p.mmr;
        else                  sum2 += p.mmr;
    }
    Logger::Log("team-balance",
        "[RoleWeightedSplit]   mmr post-pass swaps=%d sums=(%.1f)/(%.1f)\n",
        swaps, sum1, sum2);

    return out;
}

std::unordered_map<std::string, int>
ComputeRebalanceDelta(const std::vector<RosterEntry>& roster) {
    std::unordered_map<std::string, int> moves;
    if (roster.empty()) return moves;

    // 1) Ideal aggregate assignment (per-class + heal + size). We use ONLY its
    //    per-class tf1 target counts — never its specific guid->team — so we
    //    don't churn players who are already on a correct side.
    std::vector<PlayerSlot> slots;
    slots.reserve(roster.size());
    std::unordered_map<std::string, uint32_t> class_of;
    for (const auto& r : roster) {
        slots.push_back({r.guid, r.profile_id});
        class_of[r.guid] = r.profile_id;
    }
    const auto ideal = ComputeBatchAssignment(slots, TeamState{}, TeamState{});

    std::unordered_map<uint32_t, int> target1;  // class -> #wanted on tf1
    for (const auto& kv : ideal) {
        if (kv.second == 1) target1[class_of[kv.first]] += 1;
    }

    // 2) Current per-class tf1 counts, the guid+mmr lists per side per class,
    //    and the current MMR diff (sum1 - sum2) for mover selection.
    std::unordered_map<uint32_t, int> cur1, total;
    std::unordered_map<uint32_t, std::vector<std::pair<std::string, double>>> on1, on2;
    double diff = 0.0;
    for (const auto& r : roster) {
        total[r.profile_id] += 1;
        if (r.current_tf == 1) { cur1[r.profile_id] += 1; on1[r.profile_id].push_back({r.guid, r.mmr}); diff += r.mmr; }
        else                   {                          on2[r.profile_id].push_back({r.guid, r.mmr}); diff -= r.mmr; }
    }

    // 3) Move exactly the per-class surplus, choosing WHICH members move by
    //    what best closes the MMR gap (was: first-N list order).
    for (const auto& kv : total) {
        const uint32_t cls = kv.first;
        const int want1 = target1.count(cls) ? target1[cls] : 0;
        const int have1 = cur1.count(cls)    ? cur1[cls]    : 0;
        auto pick_movers = [&](std::vector<std::pair<std::string, double>>& side,
                               int count, int to_tf) {
            for (int n = 0; n < count; ++n) {
                int best = -1;
                double best_abs = 0.0;
                for (size_t i = 0; i < side.size(); ++i) {
                    if (moves.count(side[i].first)) continue;
                    const double nd = (to_tf == 2) ? diff - 2.0 * side[i].second
                                                   : diff + 2.0 * side[i].second;
                    if (best < 0 || std::fabs(nd) < best_abs) {
                        best = static_cast<int>(i);
                        best_abs = std::fabs(nd);
                    }
                }
                if (best < 0) break;
                Logger::Log("team-balance",
                    "[RoleWeightedSplit]   rebalance mover %s (mmr=%.1f) -> tf%d\n",
                    side[best].first.c_str(), side[best].second, to_tf);
                moves[side[best].first] = to_tf;
                diff = (to_tf == 2) ? diff - 2.0 * side[best].second
                                    : diff + 2.0 * side[best].second;
            }
        };
        if (have1 > want1)      pick_movers(on1[cls], have1 - want1, 2);
        else if (have1 < want1) pick_movers(on2[cls], want1 - have1, 1);
    }

    Logger::Log("team-balance",
        "[RoleWeightedSplit] ComputeRebalanceDelta roster=%zu moves=%zu\n",
        roster.size(), moves.size());
    return moves;
}

std::unordered_map<std::string, int>
ComputeFullReassignDelta(const std::vector<RosterEntry>& roster) {
    std::unordered_map<std::string, int> moves;
    if (roster.empty()) return moves;

    std::vector<PlayerSlot> slots;
    slots.reserve(roster.size());
    for (const auto& r : roster) slots.push_back({r.guid, r.profile_id});
    const auto assignment = ComputeBatchAssignment(slots, TeamState{}, TeamState{});

    for (const auto& r : roster) {
        auto it = assignment.find(r.guid);
        if (it == assignment.end()) continue;
        if (it->second != r.current_tf) moves[r.guid] = it->second;
    }
    return moves;
}

bool ShouldRebalance(const std::vector<RosterEntry>& roster,
                     const std::unordered_map<std::string, int>& delta) {
    switch (kActiveGate) {
        case RebalanceGate::AnyImprovement:
            return !delta.empty();
        case RebalanceGate::DiscreteThreshold: {
            int size1 = 0, size2 = 0;
            std::unordered_map<uint32_t, int> c1, c2;
            for (const auto& r : roster) {
                if (r.current_tf == 1) { size1++; c1[r.profile_id]++; }
                else                   { size2++; c2[r.profile_id]++; }
            }
            if (std::abs(size1 - size2) >= kDiscreteThreshold) return true;
            std::set<uint32_t> classes;
            for (const auto& kv : c1) classes.insert(kv.first);
            for (const auto& kv : c2) classes.insert(kv.first);
            for (uint32_t cls : classes) {
                const int a = c1.count(cls) ? c1[cls] : 0;
                const int b = c2.count(cls) ? c2[cls] : 0;
                if (std::abs(a - b) >= kDiscreteThreshold) return true;
            }
            return false;
        }
    }
    return false;
}

}  // namespace RoleWeightedSplit
