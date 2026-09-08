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

TEST(mmr_swap_seed_diff_steers_join) {
    // The incoming pair alone is balanced either way (1200 vs 800 on some
    // side); a live match already favoring tf1 by 500 pulls the stronger
    // newcomer to tf2.
    std::vector<Player> ps = {
        {"strong", 680, 1200.0, true}, {"weak", 680, 800.0, true},
    };
    std::unordered_map<std::string, int> asn = {{"strong", 1}, {"weak", 2}};
    MmrSwap::BalanceByMmr(ps, asn, /*seed_diff=*/500.0);
    CHECK(asn.at("strong") == 2);   // |500 - 400| beats |500 + 400|
    CHECK(asn.at("weak") == 1);
}

TEST(mmr_swap_optimal_reaches_global_minimum) {
    // Greedy stalls at +/-200 on this fixture; optimal reaches ~0.
    std::vector<Player> ps = {
        {"m0", 567, 1200.0, true}, {"m1", 567, 1100.0, true},
        {"m2", 567, 1000.0, true}, {"m3", 567, 900.0, true},
        {"a0", 680, 1300.0, true}, {"a1", 680, 700.0, true},
        {"a2", 680, 1000.0, true}, {"a3", 680, 1000.0, true},
    };
    std::unordered_map<std::string, int> asn;
    for (const auto& p : ps) asn[p.guid] = (p.guid[1] < '2') ? 1 : 2;

    MmrSwap::BalanceByMmrOptimal(ps, asn);
    double s1 = 0.0, s2 = 0.0;
    int n1 = 0, n2 = 0;
    for (const auto& p : ps) {
        if (asn.at(p.guid) == 1) { s1 += p.mmr; ++n1; }
        else                     { s2 += p.mmr; ++n2; }
    }
    const double mean_diff = std::fabs(s1 / n1 - s2 / n2);
    CHECK(mean_diff < 1e-6);
}

TEST(mmr_swap_optimal_respects_party_lock) {
    std::vector<Player> ps = {
        {"p1", 680, 1500.0, false},
        {"p2", 680,  500.0, false},
        {"s1", 680, 1400.0, true},
        {"s2", 680,  600.0, true},
    };
    std::unordered_map<std::string, int> asn =
        {{"p1", 1}, {"s1", 1}, {"p2", 2}, {"s2", 2}};
    MmrSwap::BalanceByMmrOptimal(ps, asn);
    CHECK(asn.at("p1") == 1);
    CHECK(asn.at("p2") == 2);
    CHECK(asn.at("s1") == 2);
    CHECK(asn.at("s2") == 1);
}

}  // namespace

TEST(mmr_swap_optimal_fixes_per_class_skew_at_equal_totals) {
    // The reported failure mode: both sides sum to 4000, but TF1 owns both
    // good medics and TF2 both good assaults. Totals say "balanced"; the
    // match is not. The optimizer must break this up.
    std::vector<Player> ps = {
        {"m_hi1", 567, 1500.0, true}, {"m_hi2", 567, 1500.0, true},
        {"m_lo1", 567,  500.0, true}, {"m_lo2", 567,  500.0, true},
        {"a_hi1", 680, 1500.0, true}, {"a_hi2", 680, 1500.0, true},
        {"a_lo1", 680,  500.0, true}, {"a_lo2", 680,  500.0, true},
    };
    std::unordered_map<std::string, int> asn = {
        {"m_hi1", 1}, {"m_hi2", 1}, {"m_lo1", 2}, {"m_lo2", 2},
        {"a_lo1", 1}, {"a_lo2", 1}, {"a_hi1", 2}, {"a_hi2", 2},
    };
    CHECK(std::fabs(Diff(ps, asn)) < 1e-6);                 // totals already level
    CHECK(MmrSwap::ClassMmrGap(ps, asn) > 900.0);           // classes are not

    MmrSwap::BalanceByMmrOptimal(ps, asn);

    CHECK(MmrSwap::ClassMmrGap(ps, asn) < 1e-6);            // every class level
    CHECK(std::fabs(Diff(ps, asn)) < 1e-6);                 // totals still level
    CHECK(asn.at("m_hi1") != asn.at("m_hi2"));              // good medics split
    CHECK(asn.at("a_hi1") != asn.at("a_hi2"));              // good assaults split
}

TEST(mmr_swap_optimal_prefers_class_balance_over_total_balance) {
    // The only arrangement with dead-equal totals (3000/3000) stacks the good
    // medics on one side and the good assault on the other. Class balance
    // leads, so the optimizer takes an even medic split and eats a 266-point
    // mean-MMR gap instead.
    std::vector<Player> ps = {
        {"m1", 567, 1400.0, true}, {"m2", 567, 1000.0, true},
        {"m3", 567, 1000.0, true}, {"m4", 567,  600.0, true},
        {"a1", 680, 1400.0, true}, {"a2", 680,  600.0, true},
    };
    std::unordered_map<std::string, int> asn = {
        {"m1", 1}, {"m2", 1}, {"m3", 2}, {"m4", 2},
        {"a2", 1}, {"a1", 2},
    };
    CHECK(std::fabs(Diff(ps, asn)) < 1e-6);          // 3000 vs 3000 to start

    MmrSwap::BalanceByMmrOptimal(ps, asn);

    double med1 = 0.0, med2 = 0.0;
    for (const auto& p : ps) {
        if (p.profile_id != 567) continue;
        if (asn.at(p.guid) == 1) med1 += p.mmr; else med2 += p.mmr;
    }
    CHECK(std::fabs(med1 - med2) < 1e-6);            // medics now even...
    CHECK(MmrSwap::OverallMmrGap(ps, asn) > 1.0);    // ...at the totals' cost
    CHECK(MmrSwap::ClassMmrGap(ps, asn) < 300.0);    // was 800 in the old plan
}

TEST(mmr_swap_optimal_corrects_a_live_matchs_class_skew) {
    // Join-in-progress: the live match already has the strong medic on TF1.
    // The arriving medic pair must send its stronger half to TF2.
    MmrSwap::SeedContext seed;
    seed.n_tf1 = 1;  seed.mmr_tf1 = 1600.0;
    seed.n_tf2 = 1;  seed.mmr_tf2 =  400.0;
    seed.class_n_tf1[567] = 1;  seed.class_mmr_tf1[567] = 1600.0;
    seed.class_n_tf2[567] = 1;  seed.class_mmr_tf2[567] =  400.0;

    std::vector<Player> ps = {
        {"new_hi", 567, 1200.0, true}, {"new_lo", 567, 800.0, true},
    };
    std::unordered_map<std::string, int> asn = {{"new_hi", 1}, {"new_lo", 2}};
    MmrSwap::BalanceByMmrOptimal(ps, asn, seed);
    CHECK_EQ(asn.at("new_hi"), 2);
    CHECK_EQ(asn.at("new_lo"), 1);
}
