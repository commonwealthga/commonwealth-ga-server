#include "check.hpp"
#include "mm_test_util.hpp"
#include "src/ControlServer/MatchmakingService/SidePlacement.hpp"

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
