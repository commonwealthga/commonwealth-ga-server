#include "check.hpp"
#include "mm_test_util.hpp"
#include "src/ControlServer/MatchmakingService/Rules/CoopMatchRule.hpp"

using namespace tu;

namespace {

RunningInstance OpenInst(int64_t id, int seats_used, int cap) {
    RunningInstance ri;
    ri.instance_id = id;
    ri.map_name = "Map_P";
    ri.game_mode = "TgGame_Mission";
    ri.queue_id = 1;
    ri.max_players = cap;
    ri.player_count = seats_used;
    ri.team1.size = seats_used;  // pretend all on TF1
    ri.access_mode = AccessMode::Open;
    return ri;
}

RunningInstance LockedInst(int64_t id, uint64_t owner) {
    RunningInstance ri = OpenInst(id, 1, 8);
    ri.access_mode = AccessMode::PartyLocked;
    ri.owner_party_ids = { owner };
    return ri;
}

}  // namespace

TEST(coop_solo_fresh_spawn) {
    auto cfg = Cfg("Coop", TaskforcePolicy::Pinned1, TeamPolicy::OwnMatch, TeamSidePolicy::Required, 8);
    CoopMatchRule rule(&cfg);
    auto r = rule.Evaluate({ Solo(A, 0) }, {});
    CHECK(r.has_value());
    CHECK(!r->existing_instance_id.has_value());          // fresh spawn
    CHECK(r->access_mode == AccessMode::Open);
    CHECK_EQ((int)r->session_guids.size(), 1);
    CHECK_EQ(r->task_force_assignments["s0"], 1);          // Pinned1
    CHECK_EQ((int)r->consumed_party_ids.size(), 1);
}

TEST(coop_solo_joins_open_instance) {
    auto cfg = Cfg("Coop", TaskforcePolicy::Pinned1, TeamPolicy::OwnMatch, TeamSidePolicy::Required, 8);
    CoopMatchRule rule(&cfg);
    auto r = rule.Evaluate({ Solo(A, 0) }, { OpenInst(42, 1, 8) });
    CHECK(r.has_value());
    CHECK(r->existing_instance_id.has_value());
    CHECK_EQ(*r->existing_instance_id, (int64_t)42);
    CHECK_EQ(r->map_name, std::string("Map_P"));
}

TEST(coop_solo_skips_party_locked_instance) {
    // A PARTY_LOCKED instance owned by someone else must NOT receive a solo —
    // the solo fresh-spawns instead.
    auto cfg = Cfg("Coop", TaskforcePolicy::Pinned1, TeamPolicy::OwnMatch, TeamSidePolicy::Required, 8);
    CoopMatchRule rule(&cfg);
    auto r = rule.Evaluate({ Solo(A, 0) }, { LockedInst(99, /*owner=*/123456) });
    CHECK(r.has_value());
    CHECK(!r->existing_instance_id.has_value());           // did NOT join the locked one
}

TEST(coop_team_gets_own_party_locked_match) {
    auto cfg = Cfg("Coop", TaskforcePolicy::Pinned1, TeamPolicy::OwnMatch, TeamSidePolicy::Required, 8);
    CoopMatchRule rule(&cfg);
    QueuedParty team = Team(7, {A, M}, 0);
    auto r = rule.Evaluate({ team }, {});
    CHECK(r.has_value());
    CHECK(!r->existing_instance_id.has_value());            // always fresh
    CHECK(r->access_mode == AccessMode::PartyLocked);
    CHECK_EQ((int)r->owner_party_ids.size(), 1);
    CHECK_EQ(r->owner_party_ids[0], (uint64_t)7);
    CHECK_EQ((int)r->session_guids.size(), 2);
    CHECK_EQ(r->task_force_assignments["t7_0"], 1);
    CHECK_EQ(r->task_force_assignments["t7_1"], 1);
}

TEST(coop_team_pops_before_solos_in_own_match) {
    auto cfg = Cfg("Coop", TaskforcePolicy::Pinned1, TeamPolicy::OwnMatch, TeamSidePolicy::Required, 8);
    CoopMatchRule rule(&cfg);
    QueuedParty team = Team(7, {A, A}, 5);     // joined later than the solo
    auto r = rule.Evaluate({ Solo(A, 0), team }, {});
    CHECK(r.has_value());
    CHECK(r->access_mode == AccessMode::PartyLocked);       // the team's match
    CHECK_EQ((int)r->consumed_party_ids.size(), 1);
    CHECK_EQ(r->consumed_party_ids[0], (uint64_t)7);
}

TEST(coop_ddr_mixed_all_one_side) {
    // DDR: mixed teams+solos, Pinned1 -> everyone TF1 in one match.
    auto cfg = Cfg("Coop", TaskforcePolicy::Pinned1, TeamPolicy::Mixed, TeamSidePolicy::Required, 8);
    CoopMatchRule rule(&cfg);
    auto r = rule.Evaluate({ Team(3, {A, M}, 0), Solo(R, 1), Solo(B, 2) }, {});
    CHECK(r.has_value());
    auto c = CountTf(*r);
    CHECK_EQ(c.tf1, 4);
    CHECK_EQ(c.tf2, 0);
}

TEST(coop_merc_mixed_balances_classes) {
    // Mercenary: mixed pool, BalancedPvp + preferred. Two medics must end up on
    // opposite sides (class balance is the primary objective).
    auto cfg = Cfg("Coop", TaskforcePolicy::BalancedPvp, TeamPolicy::Mixed, TeamSidePolicy::Preferred, 8);
    CoopMatchRule rule(&cfg);
    auto r = rule.Evaluate({ Solo(M,0), Solo(M,1), Solo(A,2), Solo(A,3) }, {});
    CHECK(r.has_value());
    CHECK_EQ(ClassOnTf(*r, M, 1), 1);
    CHECK_EQ(ClassOnTf(*r, M, 2), 1);
    auto c = CountTf(*r);
    CHECK_EQ(c.tf1, 2);
    CHECK_EQ(c.tf2, 2);
}

TEST(coop_merc_keeps_team_together_when_balanced) {
    // Two equal assault pairs in merc -> each team kept on its own side.
    auto cfg = Cfg("Coop", TaskforcePolicy::BalancedPvp, TeamPolicy::Mixed, TeamSidePolicy::Preferred, 8);
    CoopMatchRule rule(&cfg);
    QueuedParty t1 = Team(1, {A,A}, 0), t2 = Team(2, {A,A}, 1);
    auto r = rule.Evaluate({ t1, t2 }, {});
    CHECK(r.has_value());
    CHECK(PartyTogether(*r, t1));
    CHECK(PartyTogether(*r, t2));
}

TEST(coop_empty_parties_no_match) {
    auto cfg = Cfg("Coop", TaskforcePolicy::Pinned1, TeamPolicy::Mixed, TeamSidePolicy::Required, 8);
    CoopMatchRule rule(&cfg);
    auto r = rule.Evaluate({}, {});
    CHECK(!r.has_value());
}
