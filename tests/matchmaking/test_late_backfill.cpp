// "Volunteer me for late backfills" (MATCH_FILTERS 0x400).
#include "check.hpp"
#include "mm_test_util.hpp"
#include "src/ControlServer/MatchmakingService/Rules/CoopMatchRule.hpp"
#include "src/ControlServer/MatchModeFilter/MatchModeFilter.hpp"
#include <ctime>

using namespace tu;

namespace {
RunningInstance MercInst(int64_t id, int64_t age_secs) {
    RunningInstance ri;
    ri.instance_id = id; ri.map_name = "Merc_P"; ri.game_mode = "TgGame.TgGame_Ticket";
    ri.queue_id = 1; ri.max_players = 8; ri.player_count = 2;
    ri.team1.size = 1; ri.team1.class_counts[A] = 1; ri.team2.size = 1; ri.team2.class_counts[A] = 1;
    ri.access_mode = AccessMode::Open;
    ri.started_at = (int64_t)std::time(nullptr) - age_secs;
    return ri;
}
}  // namespace

TEST(late_backfill_volunteer_joins_late_match) {
    auto cfg = Cfg("Coop", TaskforcePolicy::BalancedPvp, TeamPolicy::Mixed, TeamSidePolicy::Preferred, 8);
    CoopMatchRule rule(&cfg);
    MatchModeFilter::OnClientJoinFlags("lb_v", 0x400);
    auto r = rule.Evaluate({ Solo(M, 0, "lb_v") }, { MercInst(42, 300) });
    CHECK(r.has_value());
    CHECK(r->existing_instance_id.has_value());
    CHECK_EQ(*r->existing_instance_id, (int64_t)42);
}

TEST(late_backfill_optout_skips_late_match) {
    auto cfg = Cfg("Coop", TaskforcePolicy::BalancedPvp, TeamPolicy::Mixed, TeamSidePolicy::Preferred, 8);
    CoopMatchRule rule(&cfg);
    MatchModeFilter::OnClientJoinFlags("lb_n", 0x0);
    auto r = rule.Evaluate({ Solo(M, 0, "lb_n") }, { MercInst(42, 300) });
    CHECK(r.has_value());
    CHECK(!r->existing_instance_id.has_value());   // fresh match instead of the running one
}

TEST(late_backfill_optout_still_joins_early_match) {
    auto cfg = Cfg("Coop", TaskforcePolicy::BalancedPvp, TeamPolicy::Mixed, TeamSidePolicy::Preferred, 8);
    CoopMatchRule rule(&cfg);
    MatchModeFilter::OnClientJoinFlags("lb_n2", 0x0);
    auto r = rule.Evaluate({ Solo(M, 0, "lb_n2") }, { MercInst(43, 60) });
    CHECK(r.has_value());
    CHECK(r->existing_instance_id.has_value());
    CHECK_EQ(*r->existing_instance_id, (int64_t)43);
}

TEST(late_backfill_unknown_player_defaults_volunteer) {
    auto cfg = Cfg("Coop", TaskforcePolicy::BalancedPvp, TeamPolicy::Mixed, TeamSidePolicy::Preferred, 8);
    CoopMatchRule rule(&cfg);
    auto r = rule.Evaluate({ Solo(M, 0, "lb_never_joined") }, { MercInst(44, 300) });
    CHECK(r.has_value());
    CHECK(r->existing_instance_id.has_value());
}

TEST(late_backfill_other_flags_dont_count) {
    // Mode boxes (0x3E) + quick queue (0x200) without 0x400 = not a volunteer.
    auto cfg = Cfg("Coop", TaskforcePolicy::BalancedPvp, TeamPolicy::Mixed, TeamSidePolicy::Preferred, 8);
    CoopMatchRule rule(&cfg);
    MatchModeFilter::OnClientJoinFlags("lb_f", 0x23E);
    auto r = rule.Evaluate({ Solo(M, 0, "lb_f") }, { MercInst(45, 300) });
    CHECK(r.has_value());
    CHECK(!r->existing_instance_id.has_value());
}

TEST(late_backfill_pve_coop_unaffected) {
    auto cfg = Cfg("Coop", TaskforcePolicy::Pinned1, TeamPolicy::OwnMatch, TeamSidePolicy::Required, 8);
    CoopMatchRule rule(&cfg);
    MatchModeFilter::OnClientJoinFlags("lb_pve", 0x0);
    auto r = rule.Evaluate({ Solo(M, 0, "lb_pve") }, { MercInst(46, 900) });
    CHECK(r.has_value());
    CHECK(r->existing_instance_id.has_value());
}

TEST(late_backfill_team_needs_every_member) {
    auto cfg = Cfg("Coop", TaskforcePolicy::BalancedPvp, TeamPolicy::Mixed, TeamSidePolicy::Preferred, 8);
    CoopMatchRule rule(&cfg);
    QueuedParty t = Team(77, {M, A}, 0);
    MatchModeFilter::OnClientJoinFlags("t77_0", 0x400);
    MatchModeFilter::OnClientJoinFlags("t77_1", 0x0);
    auto r = rule.Evaluate({ t }, { MercInst(47, 300) });
    CHECK(r.has_value());
    CHECK(!r->existing_instance_id.has_value());
    MatchModeFilter::OnClientJoinFlags("t77_1", 0x400);
    auto r2 = rule.Evaluate({ t }, { MercInst(47, 300) });
    CHECK(r2.has_value());
    CHECK(r2->existing_instance_id.has_value());
}
