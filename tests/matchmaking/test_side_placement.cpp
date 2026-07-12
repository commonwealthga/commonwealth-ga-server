#include "check.hpp"
#include "mm_test_util.hpp"
#include "src/ControlServer/MatchmakingService/SidePlacement.hpp"

#include <cmath>

using namespace tu;
using SidePlacement::Group;
using SidePlacement::Slot;
using SidePlacement::Assign;

namespace {

std::vector<Group> Groups(const std::vector<QueuedParty>& parties) {
    std::vector<Group> gs;
    for (const auto& p : parties) {
        Group g; g.party_id = p.party_id;
        for (const auto& m : p.members) g.members.push_back({m.session_guid, m.profile_id});
        gs.push_back(std::move(g));
    }
    return gs;
}

int Tf(const std::unordered_map<std::string,int>& a, const std::string& g) {
    auto it = a.find(g); return it == a.end() ? 0 : it->second;
}

int Count(const std::unordered_map<std::string,int>& a, int tf) {
    int n = 0; for (auto& [g,t] : a) if (t == tf) n++; return n;
}

}  // namespace

TEST(side_pinned1_all_tf1) {
    auto g = Groups({ Solo(A,0), Solo(M,1), Team(1,{A,A},2) });
    auto a = Assign(TaskforcePolicy::Pinned1, TeamSidePolicy::Required, g);
    CHECK_EQ(Count(a, 1), 4);
    CHECK_EQ(Count(a, 2), 0);
}

TEST(side_pinned2_all_tf2) {
    auto g = Groups({ Solo(A,0), Solo(M,1) });
    auto a = Assign(TaskforcePolicy::Pinned2, TeamSidePolicy::Required, g);
    CHECK_EQ(Count(a, 2), 2);
}

TEST(side_balanced_ignore_even_split) {
    auto g = Groups({ Solo(A,0), Solo(A,1), Solo(A,2), Solo(A,3) });
    auto a = Assign(TaskforcePolicy::Balanced, TeamSidePolicy::Ignore, g);
    CHECK_EQ(Count(a, 1), 2);
    CHECK_EQ(Count(a, 2), 2);
}

TEST(side_balancedpvp_ignore_splits_medics) {
    // Two medics + two assault, placed individually -> one medic each side.
    auto g = Groups({ Solo(M,0), Solo(M,1), Solo(A,2), Solo(A,3) });
    auto a = Assign(TaskforcePolicy::BalancedPvp, TeamSidePolicy::Ignore, g);
    int m1 = 0, m2 = 0;
    if (Tf(a,"s0")==1) m1++; else m2++;
    if (Tf(a,"s1")==1) m1++; else m2++;
    CHECK_EQ(m1, 1);
    CHECK_EQ(m2, 1);
}

TEST(side_required_keeps_team_whole) {
    // A team of 3 must land entirely on one side.
    QueuedParty team = Team(1, {A,A,A}, 0);
    auto g = Groups({ team, Solo(A,1), Solo(A,2) });
    auto a = Assign(TaskforcePolicy::BalancedPvp, TeamSidePolicy::Required, g);
    const int t = Tf(a, "t1_0");
    CHECK_EQ(Tf(a, "t1_1"), t);
    CHECK_EQ(Tf(a, "t1_2"), t);
}

TEST(side_preferred_keeps_balanced_teams_apart) {
    // Two assault pairs: class-balanced whichever way -> keep each team whole,
    // one team per side.
    QueuedParty t1 = Team(1, {A,A}, 0);
    QueuedParty t2 = Team(2, {A,A}, 1);
    auto g = Groups({ t1, t2 });
    auto a = Assign(TaskforcePolicy::BalancedPvp, TeamSidePolicy::Preferred, g);
    CHECK_EQ(Tf(a,"t1_0"), Tf(a,"t1_1"));       // team 1 together
    CHECK_EQ(Tf(a,"t2_0"), Tf(a,"t2_1"));       // team 2 together
    CHECK(Tf(a,"t1_0") != Tf(a,"t2_0"));        // opposite sides
}

TEST(side_preferred_splits_for_class_balance) {
    // [M,M] vs [A,A]: keeping whole -> one side all medics (class-imbalanced).
    // Preferred must split so each side gets 1 medic + 1 assault.
    QueuedParty t1 = Team(1, {M,M}, 0);
    QueuedParty t2 = Team(2, {A,A}, 1);
    auto g = Groups({ t1, t2 });
    auto a = Assign(TaskforcePolicy::BalancedPvp, TeamSidePolicy::Preferred, g);
    int medic_tf1 = (Tf(a,"t1_0")==1) + (Tf(a,"t1_1")==1);
    CHECK_EQ(medic_tf1, 1);                      // medics split 1/1
}

TEST(side_preferred_splits_lone_oversized_team) {
    // A 4-assault team alone: whole -> 4v0; preferred splits to balance size
    // (individual placement has equal class balance, smaller size imbalance).
    QueuedParty t1 = Team(1, {A,A,A,A}, 0);
    auto a = Assign(TaskforcePolicy::BalancedPvp, TeamSidePolicy::Preferred, Groups({t1}));
    CHECK_EQ(Count(a, 1), 2);
    CHECK_EQ(Count(a, 2), 2);
}

TEST(side_balancedpvp_seeded_join_balances_against_existing) {
    // Existing instance already has 2 assault on TF1. A new assault should go
    // TF2 to balance.
    TeamSeed s1; s1.size = 2; s1.class_counts[A] = 2;
    auto a = Assign(TaskforcePolicy::BalancedPvp, TeamSidePolicy::Ignore,
                    Groups({ Solo(A,0) }), s1, {});
    CHECK_EQ(Tf(a, "s0"), 2);
}

TEST(side_balancedpvp_mmr_swap_balances_solos) {
    // 4 solo assaults: class split stays 2/2, MMR pass pairs strong+weak.
    std::vector<Group> groups;
    const double mmrs[4] = {1400.0, 600.0, 1300.0, 700.0};
    for (int i = 0; i < 4; ++i) {
        Group g;
        g.party_id = 100 + i;
        g.members.push_back({"s" + std::to_string(i), A, mmrs[i], true});
        groups.push_back(std::move(g));
    }
    auto a = Assign(TaskforcePolicy::BalancedPvp, TeamSidePolicy::Ignore, groups);
    CHECK_EQ(Count(a, 1), 2);
    CHECK_EQ(Count(a, 2), 2);
    double diff = 0.0;
    for (int i = 0; i < 4; ++i)
        diff += (Tf(a, "s" + std::to_string(i)) == 1) ? mmrs[i] : -mmrs[i];
    CHECK(std::fabs(diff) < 1e-6);   // 1400+600 vs 1300+700
}

TEST(side_balancedpvp_mmr_never_splits_party) {
    // A strong same-class duo vs two weak solos: the duo stays whole even
    // though splitting it would improve the MMR balance.
    std::vector<Group> groups;
    Group duo;
    duo.party_id = 1;
    duo.members.push_back({"d1", A, 1500.0, false});
    duo.members.push_back({"d2", A, 1500.0, false});
    groups.push_back(std::move(duo));
    for (int i = 0; i < 2; ++i) {
        Group g;
        g.party_id = 10 + i;
        g.members.push_back({"s" + std::to_string(i), A, 500.0, true});
        groups.push_back(std::move(g));
    }
    auto a = Assign(TaskforcePolicy::BalancedPvp, TeamSidePolicy::Required, groups);
    CHECK_EQ(Tf(a, "d1"), Tf(a, "d2"));
    CHECK_EQ(Count(a, 1), 2);
    CHECK_EQ(Count(a, 2), 2);
}

TEST(side_balancedpvp_join_compensates_live_imbalance) {
    // Live match: both sides 1 assault each (class-even), tf1 ahead by 500
    // MMR. Two joining solo assaults split 1/1; the stronger one must land
    // on tf2.
    TeamSeed s1; s1.size = 1; s1.class_counts[A] = 1; s1.mmr_sum = 1500.0;
    TeamSeed s2; s2.size = 1; s2.class_counts[A] = 1; s2.mmr_sum = 1000.0;
    std::vector<Group> groups;
    Group g1; g1.party_id = 1;
    g1.members.push_back({"strong", A, 1200.0, true});
    groups.push_back(std::move(g1));
    Group g2; g2.party_id = 2;
    g2.members.push_back({"weak", A, 800.0, true});
    groups.push_back(std::move(g2));
    auto a = Assign(TaskforcePolicy::BalancedPvp, TeamSidePolicy::Ignore,
                    groups, s1, s2);
    CHECK_EQ(Count(a, 1), 1);
    CHECK_EQ(Count(a, 2), 1);
    CHECK_EQ(Tf(a, "strong"), 2);
    CHECK_EQ(Tf(a, "weak"), 1);
}

TEST(side_balancedpvp_party_tie_breaks_to_weaker_side) {
    // Class-symmetric live seeds, tf1 ahead on MMR: a joining duo whose
    // placement is a class/heal/size tie lands whole on the weaker tf2
    // (was: always TF1).
    TeamSeed s1; s1.size = 2; s1.class_counts[A] = 2; s1.mmr_sum = 2600.0;
    TeamSeed s2; s2.size = 2; s2.class_counts[A] = 2; s2.mmr_sum = 1800.0;
    std::vector<Group> groups;
    Group duo; duo.party_id = 7;
    duo.members.push_back({"d1", R, 1100.0, false});
    duo.members.push_back({"d2", R, 1100.0, false});
    groups.push_back(std::move(duo));
    auto a = Assign(TaskforcePolicy::BalancedPvp, TeamSidePolicy::Required,
                    groups, s1, s2);
    CHECK_EQ(Tf(a, "d1"), 2);
    CHECK_EQ(Tf(a, "d2"), 2);
}

TEST(side_balanced_headcount_policy_has_no_mmr_pass) {
    // Plain Balanced (non-PvP): swap pass is gated off; split still 1/1.
    std::vector<Group> groups;
    for (int i = 0; i < 2; ++i) {
        Group g;
        g.party_id = 20 + i;
        g.members.push_back({"b" + std::to_string(i), A,
                             i == 0 ? 2000.0 : 100.0, true});
        groups.push_back(std::move(g));
    }
    auto a = Assign(TaskforcePolicy::Balanced, TeamSidePolicy::Ignore, groups);
    CHECK(Tf(a, "b0") != Tf(a, "b1"));
}
