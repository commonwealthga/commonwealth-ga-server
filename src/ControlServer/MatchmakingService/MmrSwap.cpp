#include "src/ControlServer/MatchmakingService/MmrSwap.hpp"

#include <cmath>

namespace MmrSwap {

int BalanceByMmr(const std::vector<Player>& players,
                 std::unordered_map<std::string, int>& assignment,
                 double seed_diff) {
    double diff = seed_diff;   // sum(tf1) - sum(tf2), live seed + all players
    for (const auto& p : players) {
        auto it = assignment.find(p.guid);
        if (it == assignment.end()) continue;
        diff += (it->second == 1) ? p.mmr : -p.mmr;
    }

    int swaps = 0;
    const int cap = static_cast<int>(players.size());  // safety net
    while (swaps < cap) {
        const Player* best_a = nullptr;   // on tf1
        const Player* best_b = nullptr;   // on tf2
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

}  // namespace MmrSwap
