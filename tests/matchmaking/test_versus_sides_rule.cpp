#include "check.hpp"
#include "mm_test_util.hpp"
#include "src/ControlServer/MatchmakingService/Rules/VersusSidesRule.hpp"

using namespace tu;

static QueueConfig VsCfg() {
    return Cfg("VersusSides", TaskforcePolicy::Pinned1, TeamPolicy::VersusSides, TeamSidePolicy::Required, 8);
}

TEST(vs_one_party_no_match) {
    auto cfg = VsCfg();
    VersusSidesRule rule(&cfg);
    auto r = rule.Evaluate({ Solo(A, 0) }, {});
    CHECK(!r.has_value());            // need two sides
}

TEST(vs_two_solos_make_1v1) {
    auto cfg = VsCfg();
    VersusSidesRule rule(&cfg);
    auto r = rule.Evaluate({ Solo(A, 0), Solo(M, 1) }, {});
    CHECK(r.has_value());
    CHECK(r->access_mode == AccessMode::PartyLocked);
    CHECK_EQ((int)r->owner_party_ids.size(), 2);
    CHECK_EQ(r->task_force_assignments["s0"], 1);    // longest-waiting -> TF1
    CHECK_EQ(r->task_force_assignments["s1"], 2);
}

TEST(vs_team_vs_team_each_whole_side) {
    auto cfg = VsCfg();
    VersusSidesRule rule(&cfg);
    QueuedParty a = Team(1, {A, M}, 0);
    QueuedParty b = Team(2, {A, M, R}, 1);
    auto r = rule.Evaluate({ a, b }, {});
    CHECK(r.has_value());
    CHECK(PartyTogether(*r, a));
    CHECK(PartyTogether(*r, b));
    auto c = CountTf(*r);
    CHECK_EQ(c.tf1, 2);              // side A
    CHECK_EQ(c.tf2, 3);              // side B
}

TEST(vs_solo_vs_team_is_allowed) {
    auto cfg = VsCfg();
    VersusSidesRule rule(&cfg);
    QueuedParty team = Team(2, {A, A, A}, 1);
    auto r = rule.Evaluate({ Solo(A, 0), team }, {});
    CHECK(r.has_value());
    auto c = CountTf(*r);
    CHECK_EQ(c.tf1, 1);             // solo
    CHECK_EQ(c.tf2, 3);            // team
}

TEST(vs_picks_two_longest_waiting) {
    auto cfg = VsCfg();
    VersusSidesRule rule(&cfg);
    // s0 + s1 are longest-waiting; s2 must be left out (only two sides pop).
    auto r = rule.Evaluate({ Solo(A,0), Solo(A,1), Solo(A,2) }, {});
    CHECK(r.has_value());
    CHECK_EQ((int)r->session_guids.size(), 2);
    CHECK(r->task_force_assignments.count("s0") > 0);
    CHECK(r->task_force_assignments.count("s1") > 0);
    CHECK_EQ((int)r->task_force_assignments.count("s2"), 0);   // not pulled
}

TEST(vs_owner_party_ids_are_both_sides) {
    auto cfg = VsCfg();
    VersusSidesRule rule(&cfg);
    auto r = rule.Evaluate({ Solo(A, 0), Solo(M, 1) }, {});
    CHECK(r.has_value());
    CHECK_EQ((int)r->consumed_party_ids.size(), 2);
}
