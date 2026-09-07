#include "check.hpp"
#include "mm_test_util.hpp"
#include "src/ControlServer/MatchmakingService/DefenderRotation.hpp"

#include <string>
#include <unordered_map>
#include <vector>

using tu::A; using tu::R;

namespace {

std::vector<const QueuedParty*> Ptrs(const std::vector<QueuedParty>& v) {
    std::vector<const QueuedParty*> out;
    for (const auto& p : v) out.push_back(&p);
    return out;
}

int TfOf(const std::unordered_map<std::string, int>& m, const std::string& g) {
    auto it = m.find(g);
    return it == m.end() ? -1 : it->second;
}

int SideSize(const std::unordered_map<std::string, int>& m, int tf) {
    int n = 0;
    for (const auto& [g, t] : m) if (t == tf) n++;
    return n;
}

}  // namespace

TEST(defender_never_defended_goes_first) {
    // TF2 (defenders) is the scarce, wanted side. A player who has never had
    // it outranks both a recent and a long-ago defender.
    std::vector<QueuedParty> q;
    q.push_back(tu::Solo(A, 0, "veteran"));
    q.back().members[0].fairness.last_defender_id = 900;
    q.back().members[0].fairness.played_count     = 5;
    q.back().members[0].fairness.defender_count   = 3;
    q.push_back(tu::Solo(A, 1, "newbie"));           // never defended (0)
    q.push_back(tu::Solo(A, 2, "old_turn"));
    q.back().members[0].fairness.last_defender_id = 100;
    q.back().members[0].fairness.played_count     = 5;
    q.back().members[0].fairness.defender_count   = 1;

    auto m = DefenderRotation::Assign(Ptrs(q), 2, 1, nullptr);
    CHECK_EQ(SideSize(m, 1), 2);
    CHECK_EQ(SideSize(m, 2), 1);
    CHECK_EQ(TfOf(m, "newbie"), 2);
}

TEST(defender_least_recent_wins_among_defended) {
    // Everyone has defended before -> the least recent one takes the seat.
    std::vector<QueuedParty> q;
    q.push_back(tu::Solo(A, 0, "recent"));
    q.back().members[0].fairness.last_defender_id = 900;
    q.back().members[0].fairness.played_count     = 4;
    q.back().members[0].fairness.defender_count   = 2;
    q.push_back(tu::Solo(A, 1, "stale"));
    q.back().members[0].fairness.last_defender_id = 12;
    q.back().members[0].fairness.played_count     = 4;
    q.back().members[0].fairness.defender_count   = 2;

    auto m = DefenderRotation::Assign(Ptrs(q), 1, 1, nullptr);
    CHECK_EQ(TfOf(m, "stale"), 2);
    CHECK_EQ(TfOf(m, "recent"), 1);
}

TEST(defender_ratio_breaks_never_defended_ties) {
    // Both defended at the same point; the smaller defender SHARE wins.
    std::vector<QueuedParty> q;
    q.push_back(tu::Solo(A, 0, "hogged"));
    q.back().members[0].fairness.last_defender_id = 500;
    q.back().members[0].fairness.played_count     = 4;
    q.back().members[0].fairness.defender_count   = 3;   // 0.75
    q.push_back(tu::Solo(A, 1, "starved"));
    q.back().members[0].fairness.last_defender_id = 500;
    q.back().members[0].fairness.played_count     = 20;
    q.back().members[0].fairness.defender_count   = 1;   // 0.05

    auto m = DefenderRotation::Assign(Ptrs(q), 1, 1, nullptr);
    CHECK_EQ(TfOf(m, "starved"), 2);
}

TEST(defender_party_cohesion_exact_subset) {
    // Total 7 -> 4v3. A 3-party and a 4-party land whole on the two sides,
    // which is the coordinated attackers-vs-defenders split (design D6).
    std::vector<QueuedParty> q;
    q.push_back(tu::Team(11, {A, A, A}, 0));
    q.push_back(tu::Team(22, {A, A, A, A}, 1));

    bool split = true;
    auto m = DefenderRotation::Assign(Ptrs(q), 4, 3, &split);
    CHECK(!split);
    CHECK_EQ(SideSize(m, 1), 4);
    CHECK_EQ(SideSize(m, 2), 3);
    CHECK_EQ(TfOf(m, "t11_0"), TfOf(m, "t11_1"));
    CHECK_EQ(TfOf(m, "t11_1"), TfOf(m, "t11_2"));
    CHECK_EQ(TfOf(m, "t22_0"), TfOf(m, "t22_3"));
}

TEST(defender_spill_fallback_keeps_sides_exact) {
    // Two 5-stacks at total 10 -> 6v4. No party subset sums to 4, so exactly
    // one party spills (design D7) and the sides are still exact.
    std::vector<QueuedParty> q;
    q.push_back(tu::Team(33, {A, A, A, A, A}, 0));
    q.push_back(tu::Team(44, {R, R, R, R, R}, 1));

    bool split = false;
    auto m = DefenderRotation::Assign(Ptrs(q), 6, 4, &split);
    CHECK(split);
    CHECK_EQ(SideSize(m, 1), 6);
    CHECK_EQ(SideSize(m, 2), 4);
    CHECK_EQ((int)m.size(), 10);          // everyone placed
}

TEST(defender_solo_pool_never_spills) {
    // All solos: a subset always sums exactly, so cohesion is never at stake.
    std::vector<QueuedParty> q;
    for (int i = 0; i < 8; ++i) q.push_back(tu::Solo(A, i));
    bool split = true;
    auto m = DefenderRotation::Assign(Ptrs(q), 5, 3, &split);
    CHECK(!split);
    CHECK_EQ(SideSize(m, 1), 5);
    CHECK_EQ(SideSize(m, 2), 3);
}

TEST(defender_assignment_is_deterministic) {
    std::vector<QueuedParty> q;
    for (int i = 0; i < 6; ++i) q.push_back(tu::Solo(A, i));
    auto a = DefenderRotation::Assign(Ptrs(q), 4, 2, nullptr);
    auto b = DefenderRotation::Assign(Ptrs(q), 4, 2, nullptr);
    CHECK(a == b);
}
