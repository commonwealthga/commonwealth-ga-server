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

    std::vector<std::unordered_map<Milli, Milli>> parent(classes.size());
    for (Milli add : class_sums[0]) parent[0][add] = 0;

    for (size_t ci = 1; ci < classes.size(); ++ci) {
        for (const auto& kv : parent[ci - 1]) {
            for (Milli add : class_sums[ci]) {
                const Milli ns = kv.first + add;
                if (!parent[ci].count(ns)) parent[ci][ns] = kv.first;
            }
        }
        if (parent[ci].empty()) return 0;
    }

    const size_t last = classes.size() - 1;
    Milli best_s1 = 0;
    double best_diff = std::numeric_limits<double>::infinity();
    for (const auto& kv : parent[last]) {
        const Milli s1 = kv.first;
        const Milli s2 = total - s1;
        const double d = MeanMmrDiff(FromMilli(s1), n1, FromMilli(s2), n2, seed);
        if (d < best_diff - 1e-12) {
            best_diff = d;
            best_s1 = s1;
        }
    }

    std::vector<Milli> pick(classes.size());
    Milli cur = best_s1;
    for (int ci = static_cast<int>(classes.size()) - 1; ci >= 0; --ci) {
        auto it = parent[static_cast<size_t>(ci)].find(cur);
        if (it == parent[static_cast<size_t>(ci)].end()) return 0;
        pick[static_cast<size_t>(ci)] = cur - it->second;
        cur = it->second;
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
