#include "check.hpp"
#include "src/ControlServer/MatchmakingService/MmrSwap.hpp"

#include <cmath>
#include <unordered_map>
#include <vector>

using MmrSwap::Player;

namespace {

double Diff(const std::vector<Player>& ps,
            const std::unordered_map<std::string, int>& asn) {
    double d = 0.0;
    for (const auto& p : ps) d += (asn.at(p.guid) == 1) ? p.mmr : -p.mmr;
    return d;
}

TEST(mmr_swap_converges_same_class) {
    std::vector<Player> ps = {
        {"a", 680, 1200.0, true}, {"b", 680, 1100.0, true},
        {"c", 680, 1000.0, true}, {"d", 680,  900.0, true},
    };
    std::unordered_map<std::string, int> asn =
        {{"a", 1}, {"b", 1}, {"c", 2}, {"d", 2}};   // 2300 vs 1900
    const int swaps = MmrSwap::BalanceByMmr(ps, asn);
    CHECK(swaps >= 1);
    CHECK(std::fabs(Diff(ps, asn)) < 1e-6);          // 2100 vs 2100 reachable
    int on1 = 0;
    for (const auto& kv : asn) if (kv.second == 1) on1++;
    CHECK(on1 == 2);                                  // counts invariant
}

TEST(mmr_swap_never_crosses_classes) {
    std::vector<Player> ps = {
        {"m1", 567, 1400.0, true}, {"a1", 680, 1000.0, true},
        {"m2", 567,  800.0, true}, {"a2", 680, 1000.0, true},
    };
    std::unordered_map<std::string, int> asn =
        {{"m1", 1}, {"a1", 1}, {"m2", 2}, {"a2", 2}};
    MmrSwap::BalanceByMmr(ps, asn);
    // Only the medic pair can swap; every team keeps 1 medic + 1 assault.
    CHECK(asn.at("m1") != asn.at("m2"));
    CHECK(asn.at("a1") != asn.at("a2"));
}

TEST(mmr_swap_respects_unswappable) {
    std::vector<Player> ps = {
        {"p1", 680, 1500.0, false},   // party member, locked
        {"p2", 680,  500.0, false},
        {"s1", 680, 1400.0, true},
        {"s2", 680,  600.0, true},
    };
    std::unordered_map<std::string, int> asn =
        {{"p1", 1}, {"s1", 1}, {"p2", 2}, {"s2", 2}};   // 2900 vs 1100
    MmrSwap::BalanceByMmr(ps, asn);
    CHECK(asn.at("p1") == 1);                            // locked stayed
    CHECK(asn.at("p2") == 2);
    CHECK(asn.at("s1") == 2);                            // solos swapped
    CHECK(asn.at("s2") == 1);
}

TEST(mmr_swap_noop_on_equal_ratings) {
    std::vector<Player> ps = {
        {"a", 680, 1000.0, true}, {"b", 680, 1000.0, true},
    };
    std::unordered_map<std::string, int> asn = {{"a", 1}, {"b", 2}};
    CHECK(MmrSwap::BalanceByMmr(ps, asn) == 0);
    CHECK(asn.at("a") == 1);
}

}  // namespace
