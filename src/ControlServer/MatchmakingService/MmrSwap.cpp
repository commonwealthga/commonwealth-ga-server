#include "src/ControlServer/MatchmakingService/MmrSwap.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <unordered_map>
#include <vector>

namespace MmrSwap {

namespace {

using Milli = int64_t;
static constexpr Milli kMilli = 1000;

Milli ToMilli(double v) {
    return static_cast<Milli>(std::llround(v * static_cast<double>(kMilli)));
}

double FromMilli(Milli m) {
    return static_cast<double>(m) / static_cast<double>(kMilli);
}

double MeanMmrDiff(double sum1, int n1, double sum2, int n2,
                   const SeedContext& seed) {
    const double t1 = seed.mmr_tf1 + sum1;
    const double t2 = seed.mmr_tf2 + sum2;
    const int    c1 = seed.n_tf1 + n1;
    const int    c2 = seed.n_tf2 + n2;
    if (c1 <= 0 || c2 <= 0) return 0.0;
    return std::fabs(t1 / static_cast<double>(c1) - t2 / static_cast<double>(c2));
}

template <typename K, typename V>
V Lookup(const std::unordered_map<K, V>& m, K key) {
    auto it = m.find(key);
    return it == m.end() ? V{} : it->second;
}

// Mean-MMR gap of ONE class, seed included. 0 when the class is absent from
// a side (no comparison to make).
double ClassGap(uint32_t cls, double sum1, int n1, double sum2, int n2,
                const SeedContext& seed) {
    const double t1 = Lookup(seed.class_mmr_tf1, cls) + sum1;
    const double t2 = Lookup(seed.class_mmr_tf2, cls) + sum2;
    const int    c1 = Lookup(seed.class_n_tf1, cls) + n1;
    const int    c2 = Lookup(seed.class_n_tf2, cls) + n2;
    if (c1 <= 0 || c2 <= 0) return 0.0;
    return std::fabs(t1 / static_cast<double>(c1) - t2 / static_cast<double>(c2));
}

// Per-class aggregates of `players` under `assignment`, keyed by profile id.
struct ClassTally {
    double sum1 = 0.0, sum2 = 0.0;
    int    n1 = 0, n2 = 0;
};

std::unordered_map<uint32_t, ClassTally> Tally(
    const std::vector<Player>& players,
    const std::unordered_map<std::string, int>& assignment) {
    std::unordered_map<uint32_t, ClassTally> out;
    for (const auto& p : players) {
        auto it = assignment.find(p.guid);
        if (it == assignment.end()) continue;
        ClassTally& t = out[p.profile_id];
        if (it->second == 1) { t.sum1 += p.mmr; t.n1 += 1; }
        else                 { t.sum2 += p.mmr; t.n2 += 1; }
    }
    return out;
}

std::vector<Milli> ClassAchievableSums(
    const std::vector<const Player*>& players,
    const std::unordered_map<std::string, int>& assignment,
    int n1_target) {
    int locked1 = 0;
    Milli locked_sum1 = 0;
    std::vector<Milli> pool;
    pool.reserve(players.size());

    for (const Player* p : players) {
        auto it = assignment.find(p->guid);
        if (it == assignment.end()) continue;
        if (!p->swappable) {
            if (it->second == 1) {
                locked1++;
                locked_sum1 += ToMilli(p->mmr);
            }
            continue;
        }
        pool.push_back(ToMilli(p->mmr));
    }

    const int need = n1_target - locked1;
    if (need < 0 || need > static_cast<int>(pool.size())) return {};

    std::vector<Milli> out;
    if (need == 0) {
        out.push_back(locked_sum1);
        return out;
    }

    std::sort(pool.begin(), pool.end());
    const int n = static_cast<int>(pool.size());
    std::vector<int> idx(need);
    for (int i = 0; i < need; ++i) idx[i] = i;

    auto emit = [&]() {
        Milli s = locked_sum1;
        for (int i = 0; i < need; ++i) s += pool[idx[i]];
        out.push_back(s);
    };

    emit();
    while (true) {
        int i = need - 1;
        while (i >= 0 && idx[i] == i + n - need) --i;
        if (i < 0) break;
        ++idx[i];
        for (int j = i + 1; j < need; ++j) idx[j] = idx[j - 1] + 1;
        emit();
    }

    std::sort(out.begin(), out.end());
    out.erase(std::unique(out.begin(), out.end()), out.end());
    return out;
}

bool AssignClassCombination(
    const std::vector<const Player*>& players,
    int n1_target, Milli target_sum1,
    std::unordered_map<std::string, int>& assignment) {
    int locked1 = 0;
    Milli locked_sum1 = 0;
    std::vector<std::pair<std::string, Milli>> pool;
    for (const Player* p : players) {
        auto it = assignment.find(p->guid);
        if (it == assignment.end()) continue;
        if (!p->swappable) {
            if (it->second == 1) {
                locked1++;
                locked_sum1 += ToMilli(p->mmr);
            }
            continue;
        }
        pool.push_back({p->guid, ToMilli(p->mmr)});
    }

    const int need = n1_target - locked1;
    if (need < 0) return false;
    if (need == 0) return locked_sum1 == target_sum1;

    const int n = static_cast<int>(pool.size());
    std::vector<int> idx(need);
    for (int i = 0; i < need; ++i) idx[i] = i;

    auto try_combo = [&]() -> bool {
        Milli s = locked_sum1;
        for (int i = 0; i < need; ++i) s += pool[idx[i]].second;
        if (s != target_sum1) return false;
        for (int i = 0; i < need; ++i) assignment[pool[idx[i]].first] = 1;
        for (int i = 0; i < n; ++i) {
            bool on1 = false;
            for (int j = 0; j < need; ++j)
                if (idx[j] == i) { on1 = true; break; }
            if (!on1) assignment[pool[i].first] = 2;
        }
        return true;
    };

    if (try_combo()) return true;
    while (true) {
        int i = need - 1;
        while (i >= 0 && idx[i] == i + n - need) --i;
        if (i < 0) break;
        ++idx[i];
        for (int j = i + 1; j < need; ++j) idx[j] = idx[j - 1] + 1;
        if (try_combo()) return true;
    }
    return false;
}

}  // namespace

int BalanceByMmr(const std::vector<Player>& players,
                 std::unordered_map<std::string, int>& assignment,
                 double seed_diff) {
    double diff = seed_diff;
    for (const auto& p : players) {
        auto it = assignment.find(p.guid);
        if (it == assignment.end()) continue;
        diff += (it->second == 1) ? p.mmr : -p.mmr;
    }

    int swaps = 0;
    const int cap = static_cast<int>(players.size());
    while (swaps < cap) {
        const Player* best_a = nullptr;
        const Player* best_b = nullptr;
        double best_abs = std::fabs(diff);
        double best_diff = diff;
        for (const auto& a : players) {
            if (!a.swappable) continue;
            auto ia = assignment.find(a.guid);
            if (ia == assignment.end() || ia->second != 1) continue;
            for (const auto& b : players) {
                if (!b.swappable || b.profile_id != a.profile_id) continue;
                auto ib = assignment.find(b.guid);
                if (ib == assignment.end() || ib->second != 2) continue;
                const double nd = diff - 2.0 * a.mmr + 2.0 * b.mmr;
                if (std::fabs(nd) < best_abs - 1e-9) {
                    best_abs = std::fabs(nd);
                    best_diff = nd;
                    best_a = &a;
                    best_b = &b;
                }
            }
        }
        if (!best_a) break;
        assignment[best_a->guid] = 2;
        assignment[best_b->guid] = 1;
        diff = best_diff;
        swaps++;
    }
    return swaps;
}

double ClassMmrGap(const std::vector<Player>& players,
                   const std::unordered_map<std::string, int>& assignment,
                   const SeedContext& seed) {
    const auto tally = Tally(players, assignment);
    double weighted = 0.0;
    int    total    = seed.n_tf1 + seed.n_tf2;
    for (const auto& kv : tally) total += kv.second.n1 + kv.second.n2;
    if (total <= 0) return 0.0;
    for (const auto& kv : tally) {
        const ClassTally& t = kv.second;
        const int heads = Lookup(seed.class_n_tf1, kv.first)
                        + Lookup(seed.class_n_tf2, kv.first) + t.n1 + t.n2;
        weighted += static_cast<double>(heads)
                  * ClassGap(kv.first, t.sum1, t.n1, t.sum2, t.n2, seed);
    }
    return weighted / static_cast<double>(total);
}

double OverallMmrGap(const std::vector<Player>& players,
                     const std::unordered_map<std::string, int>& assignment,
                     const SeedContext& seed) {
    double s1 = 0.0, s2 = 0.0;
    int    n1 = 0,   n2 = 0;
    for (const auto& p : players) {
        auto it = assignment.find(p.guid);
        if (it == assignment.end()) continue;
        if (it->second == 1) { s1 += p.mmr; ++n1; }
        else                 { s2 += p.mmr; ++n2; }
    }
    return MeanMmrDiff(s1, n1, s2, n2, seed);
}

double BalanceCost(const std::vector<Player>& players,
                   const std::unordered_map<std::string, int>& assignment,
                   const SeedContext& seed) {
    return kClassBalanceWeight * ClassMmrGap(players, assignment, seed)
         + OverallMmrGap(players, assignment, seed);
}

int BalanceByMmrOptimal(const std::vector<Player>& players,
                        std::unordered_map<std::string, int>& assignment,
                        const SeedContext& seed) {
    if (players.empty()) return 0;

    const std::unordered_map<std::string, int> before = assignment;

    int n1 = 0, n2 = 0;
    Milli total = 0;
    for (const auto& p : players) {
        auto it = assignment.find(p.guid);
        if (it == assignment.end()) continue;
        total += ToMilli(p.mmr);
        if (it->second == 1) ++n1;
        else                 ++n2;
    }
    if (n1 == 0 || n2 == 0) return 0;

    std::unordered_map<uint32_t, int> n1_target;
    std::unordered_map<uint32_t, std::vector<const Player*>> by_class;
    for (const auto& p : players) {
        auto it = assignment.find(p.guid);
        if (it == assignment.end()) continue;
        by_class[p.profile_id].push_back(&p);
        if (it->second == 1) n1_target[p.profile_id] += 1;
    }

    std::vector<uint32_t> classes;
    classes.reserve(by_class.size());
    for (const auto& kv : by_class) classes.push_back(kv.first);
    std::sort(classes.begin(), classes.end());

    std::vector<std::vector<Milli>> class_sums(classes.size());
    for (size_t ci = 0; ci < classes.size(); ++ci) {
        class_sums[ci] = ClassAchievableSums(
            by_class[classes[ci]], assignment, n1_target[classes[ci]]);
        if (class_sums[ci].empty()) return 0;
    }

    // Per-class MMR totals and headcounts of the batch, plus the overall
    // headcount used to weight each class's contribution.
    const int heads_total = seed.n_tf1 + seed.n_tf2 + n1 + n2;
    std::vector<Milli> class_total(classes.size(), 0);
    std::vector<double> class_weight(classes.size(), 0.0);
    for (size_t ci = 0; ci < classes.size(); ++ci) {
        const uint32_t cls = classes[ci];
        for (const Player* p : by_class[cls]) class_total[ci] += ToMilli(p->mmr);
        const int heads = Lookup(seed.class_n_tf1, cls) + Lookup(seed.class_n_tf2, cls)
                        + static_cast<int>(by_class[cls].size());
        class_weight[ci] = heads_total > 0
            ? static_cast<double>(heads) / static_cast<double>(heads_total) : 0.0;
    }

    // Weighted per-class gap contributed by putting `s1_c` MMR of class `ci`
    // on TF1. Separable across classes, which is what lets the DP carry it.
    auto class_cost = [&](size_t ci, Milli s1_c) {
        const uint32_t cls = classes[ci];
        const int c_n1 = n1_target[cls];
        const int c_n2 = static_cast<int>(by_class[cls].size()) - c_n1;
        return class_weight[ci]
             * ClassGap(cls, FromMilli(s1_c), c_n1,
                        FromMilli(class_total[ci] - s1_c), c_n2, seed);
    };

    // dp[ci][prefix TF1 sum] = { min accumulated class cost, predecessor sum }.
    struct Cell { double cost; Milli prev; };
    std::vector<std::unordered_map<Milli, Cell>> dp(classes.size());
    for (Milli add : class_sums[0]) dp[0][add] = {class_cost(0, add), 0};

    for (size_t ci = 1; ci < classes.size(); ++ci) {
        for (const auto& kv : dp[ci - 1]) {
            for (Milli add : class_sums[ci]) {
                const Milli ns = kv.first + add;
                const double nc = kv.second.cost + class_cost(ci, add);
                auto it = dp[ci].find(ns);
                if (it == dp[ci].end()) dp[ci][ns] = {nc, kv.first};
                else if (nc < it->second.cost - 1e-12) it->second = {nc, kv.first};
            }
        }
        if (dp[ci].empty()) return 0;
    }

    const size_t last = classes.size() - 1;
    Milli best_s1 = 0;
    double best_obj = std::numeric_limits<double>::infinity();
    for (const auto& kv : dp[last]) {
        const Milli s1 = kv.first;
        const Milli s2 = total - s1;
        const double obj = kClassBalanceWeight * kv.second.cost
            + MeanMmrDiff(FromMilli(s1), n1, FromMilli(s2), n2, seed);
        if (obj < best_obj - 1e-12) {
            best_obj = obj;
            best_s1 = s1;
        }
    }

    std::vector<Milli> pick(classes.size());
    Milli cur = best_s1;
    for (int ci = static_cast<int>(classes.size()) - 1; ci >= 0; --ci) {
        auto it = dp[static_cast<size_t>(ci)].find(cur);
        if (it == dp[static_cast<size_t>(ci)].end()) return 0;
        pick[static_cast<size_t>(ci)] = cur - it->second.prev;
        cur = it->second.prev;
    }

    for (size_t ci = 0; ci < classes.size(); ++ci) {
        if (!AssignClassCombination(
                by_class[classes[ci]], n1_target[classes[ci]],
                pick[ci], assignment))
            return 0;
    }

    int moved = 0;
    for (const auto& p : players) {
        if (before.at(p.guid) != assignment.at(p.guid)) ++moved;
    }
    return moved;
}

}  // namespace MmrSwap
