#include "check.hpp"
#include "mm_test_util.hpp"
#include "src/ControlServer/MatchmakingService/Rules/DoubleAgentRule.hpp"

using namespace tu;

static QueueConfig DaCfg() {
    return Cfg("DoubleAgent", TaskforcePolicy::Pinned1, TeamPolicy::Mixed, TeamSidePolicy::Preferred, 10);
}

TEST(da_below_min_no_match) {
    auto cfg = DaCfg();
    DoubleAgentRule rule(&cfg);
    auto r = rule.Evaluate({ Solo(A, 0) }, {});
    CHECK(!r.has_value());            // a single body can't fill the 2-player floor
}

TEST(da_total2_is_1v1) {
    auto cfg = DaCfg();
    DoubleAgentRule rule(&cfg);
    auto r = rule.Evaluate({ Solo(A,0), Solo(M,1) }, {});
    CHECK(r.has_value());
    auto c = CountTf(*r);
    CHECK_EQ(c.tf1, 1);
    CHECK_EQ(c.tf2, 1);
}

TEST(da_total5_is_3v2) {
    auto cfg = DaCfg();
    DoubleAgentRule rule(&cfg);
    auto r = rule.Evaluate({ Solo(A,0), Solo(A,1), Solo(M,2), Solo(R,3), Solo(B,4) }, {});
    CHECK(r.has_value());
    auto c = CountTf(*r);
    CHECK_EQ(c.tf1, 3);
    CHECK_EQ(c.tf2, 2);
}

TEST(da_sealed_with_cap_override) {
    auto cfg = DaCfg();
    DoubleAgentRule rule(&cfg);
    auto r = rule.Evaluate({ Solo(A,0), Solo(A,1), Solo(A,2) }, {});  // total 3
    CHECK(r.has_value());
    CHECK(r->access_mode == AccessMode::Sealed);
    CHECK(r->cap_override.has_value());
    CHECK_EQ(*r->cap_override, (uint32_t)3);
}

TEST(da_never_joins_existing_instance) {
    auto cfg = DaCfg();
    DoubleAgentRule rule(&cfg);
    RunningInstance ri; ri.instance_id = 1; ri.max_players = 10; ri.access_mode = AccessMode::Open;
    auto r = rule.Evaluate({ Solo(A,0), Solo(M,1) }, { ri });
    CHECK(r.has_value());
    CHECK(!r->existing_instance_id.has_value());     // always fresh + sealed
}

TEST(da_team_kept_same_side_when_shape_allows) {
    // total 4 -> shape (2,2). A team of 2 + two solos: the team fits a side
    // whole, so it stays together.
    auto cfg = DaCfg();
    DoubleAgentRule rule(&cfg);
    QueuedParty t = Team(5, {A,A}, 0);
    auto r = rule.Evaluate({ t, Solo(M,1), Solo(R,2) }, {});
    CHECK(r.has_value());
    CHECK(PartyTogether(*r, t));
    auto c = CountTf(*r);
    CHECK_EQ(c.tf1, 2);
    CHECK_EQ(c.tf2, 2);
}

TEST(da_team_splits_when_shape_forces) {
    // total 4 -> shape (2,2). A team of 3 can't fit either size-2 side whole,
    // so it spills: 2 on its primary side, 1 on the other.
    auto cfg = DaCfg();
    DoubleAgentRule rule(&cfg);
    QueuedParty t = Team(6, {A,A,A}, 0);
    auto r = rule.Evaluate({ t, Solo(M,1) }, {});
    CHECK(r.has_value());
    auto c = CountTf(*r);
    CHECK_EQ(c.tf1, 2);
    CHECK_EQ(c.tf2, 2);
    CHECK(!PartyTogether(*r, t));      // forced split
}

TEST(da_caps_total_at_ten) {
    auto cfg = DaCfg();
    DoubleAgentRule rule(&cfg);
    std::vector<QueuedParty> parties;
    for (int i = 0; i < 14; ++i) parties.push_back(Solo(A, i));
    auto r = rule.Evaluate(parties, {});
    CHECK(r.has_value());
    CHECK_EQ((int)r->session_guids.size(), 10);    // shape table tops out at 10
    auto c = CountTf(*r);
    CHECK_EQ(c.tf1, 6);
    CHECK_EQ(c.tf2, 4);
}
