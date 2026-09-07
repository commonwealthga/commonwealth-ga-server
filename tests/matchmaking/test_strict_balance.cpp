#include "check.hpp"
#include "mm_test_util.hpp"
#include "src/ControlServer/MatchmakingService/StrictBalance.hpp"
#include "src/ControlServer/MatchmakingService/Rules/CoopMatchRule.hpp"

#include <algorithm>

using tu::A; using tu::M; using tu::R; using tu::B;

namespace {

// Total members across a chosen-party list.
int Members(const std::vector<const QueuedParty*>& v) {
    int n = 0;
    for (auto* p : v) n += (int)p->size();
    return n;
}

// Per-class member count across a chosen-party list.
int ClassCount(const std::vector<const QueuedParty*>& v, uint32_t cls) {
    int n = 0;
    for (auto* p : v)
        for (const auto& m : p->members)
            if (m.profile_id == cls) n++;
    return n;
}

bool Contains(const std::vector<const QueuedParty*>& v, const std::string& guid) {
    for (auto* p : v)
        for (const auto& m : p->members)
            if (m.session_guid == guid) return true;
    return false;
}

}  // namespace

TEST(strict_priority_order_exclusions_then_wait) {
    std::vector<QueuedParty> q;
    q.push_back(tu::Solo(A, 0, "old"));
    q.push_back(tu::Solo(A, 5, "excluded"));
    q.back().members[0].fairness.exclusion_count = 2;
    auto ordered = StrictBalance::PartiesByPriority(q);
    CHECK_EQ(ordered[0]->members[0].session_guid, std::string("excluded"));
    CHECK_EQ(ordered[1]->members[0].session_guid, std::string("old"));
}

TEST(strict_select_all_even_takes_everyone) {
    std::vector<QueuedParty> q;
    q.push_back(tu::Solo(A, 0)); q.push_back(tu::Solo(A, 1));
    q.push_back(tu::Solo(M, 2)); q.push_back(tu::Solo(M, 3));
    auto sel = StrictBalance::SelectClassEqualSubset(q, -1);
    CHECK_EQ(Members(sel), 4);
}

TEST(strict_select_odd_counts_drop_latest_of_class) {
    // 5A + 3M + 2R -> 4A + 2M + 2R; the newest A and newest M sit out.
    std::vector<QueuedParty> q;
    for (int i = 0; i < 5; ++i) q.push_back(tu::Solo(A, i, "a" + std::to_string(i)));
    for (int i = 0; i < 3; ++i) q.push_back(tu::Solo(M, 10 + i, "m" + std::to_string(i)));
    for (int i = 0; i < 2; ++i) q.push_back(tu::Solo(R, 20 + i, "r" + std::to_string(i)));
    auto sel = StrictBalance::SelectClassEqualSubset(q, -1);
    CHECK_EQ(Members(sel), 8);
    CHECK_EQ(ClassCount(sel, A), 4);
    CHECK_EQ(ClassCount(sel, M), 2);
    CHECK_EQ(ClassCount(sel, R), 2);
    CHECK(!Contains(sel, "a4"));
    CHECK(!Contains(sel, "m2"));
}

TEST(strict_select_excluded_player_beats_newer_same_class) {
    // 3 A queued; the newest one carries exclusion priority -> the middle one sits out.
    std::vector<QueuedParty> q;
    q.push_back(tu::Solo(A, 0, "first"));
    q.push_back(tu::Solo(A, 1, "second"));
    q.push_back(tu::Solo(A, 2, "priority"));
    q.back().members[0].fairness.exclusion_count = 1;
    auto sel = StrictBalance::SelectClassEqualSubset(q, -1);
    CHECK_EQ(Members(sel), 2);
    CHECK(Contains(sel, "priority"));
    CHECK(Contains(sel, "first"));
    CHECK(!Contains(sel, "second"));
}

TEST(strict_select_all_odd_returns_empty) {
    std::vector<QueuedParty> q;
    q.push_back(tu::Solo(A, 0));
    q.push_back(tu::Solo(M, 1));
    auto sel = StrictBalance::SelectClassEqualSubset(q, -1);
    CHECK_EQ(Members(sel), 0);
}

TEST(strict_select_respects_cap) {
    std::vector<QueuedParty> q;
    for (int i = 0; i < 10; ++i) q.push_back(tu::Solo(A, i));
    auto sel = StrictBalance::SelectClassEqualSubset(q, 6);
    CHECK_EQ(Members(sel), 6);
}

TEST(strict_select_party_kept_whole_solo_repaired) {
    // Team of 2A + solos 1A + 2M -> drop the solo A, keep the team.
    std::vector<QueuedParty> q;
    q.push_back(tu::Team(1, {A, A}, 0));
    q.push_back(tu::Solo(A, 1, "solo_a"));
    q.push_back(tu::Solo(M, 2)); q.push_back(tu::Solo(M, 3));
    auto sel = StrictBalance::SelectClassEqualSubset(q, -1);
    CHECK_EQ(Members(sel), 4);
    CHECK_EQ(ClassCount(sel, A), 2);
    CHECK_EQ(ClassCount(sel, M), 2);
    CHECK(!Contains(sel, "solo_a"));
}

TEST(strict_select_party_removed_when_no_solo_of_odd_class) {
    // Team {A,M} + solo M: A odd, no solo A -> team removed -> M odd -> solo
    // removed -> empty.
    std::vector<QueuedParty> q;
    q.push_back(tu::Team(1, {A, M}, 0));
    q.push_back(tu::Solo(M, 1));
    auto sel = StrictBalance::SelectClassEqualSubset(q, -1);
    CHECK_EQ(Members(sel), 0);
}

namespace {

// A live BACKFILL_ONLY instance with the given per-side class counts and
// MMR sums. Sizes derive from the counts.
RunningInstance Inst(std::vector<std::pair<uint32_t,int>> t1,
                     std::vector<std::pair<uint32_t,int>> t2,
                     double mmr1, double mmr2, int max_players = 0) {
    RunningInstance ri;
    ri.instance_id = 77;
    ri.max_players = max_players;
    ri.access_mode = AccessMode::BackfillOnly;
    for (auto& [cls, n] : t1) { ri.team1.class_counts[cls] += n; ri.team1.size += n; }
    for (auto& [cls, n] : t2) { ri.team2.class_counts[cls] += n; ri.team2.size += n; }
    ri.team1.mmr_sum = mmr1;
    ri.team2.mmr_sum = mmr2;
    ri.player_count = ri.team1.size + ri.team2.size;
    return ri;
}

}  // namespace

TEST(backfill_fills_single_class_deficit_on_short_side) {
    auto inst = Inst({{A,2},{M,1}}, {{A,1},{M,1}}, 3000, 2000);
    std::vector<QueuedParty> q;
    q.push_back(tu::Solo(M, 0, "medic"));
    q.push_back(tu::Solo(A, 1, "assault"));
    auto inv = StrictBalance::PlanDeficitBackfill(q, inst);
    CHECK_EQ((int)inv.size(), 1);
    CHECK_EQ(inv[0].party->members[0].session_guid, std::string("assault"));
    CHECK_EQ(inv[0].tf, 2);
}

TEST(backfill_fills_two_seat_deficit) {
    auto inst = Inst({{A,3}}, {{A,1}}, 3000, 1000);
    std::vector<QueuedParty> q;
    q.push_back(tu::Solo(A, 0)); q.push_back(tu::Solo(A, 1)); q.push_back(tu::Solo(A, 2));
    auto inv = StrictBalance::PlanDeficitBackfill(q, inst);
    CHECK_EQ((int)inv.size(), 2);
    CHECK_EQ(inv[0].tf, 2);
    CHECK_EQ(inv[1].tf, 2);
}

TEST(backfill_ignores_wrong_class_and_teams) {
    auto inst = Inst({{A,2}}, {{A,1}}, 2000, 1000);
    std::vector<QueuedParty> q;
    q.push_back(tu::Solo(M, 0));            // wrong class
    q.push_back(tu::Team(1, {A, A}, 1));    // team — never backfilled
    CHECK(StrictBalance::PlanDeficitBackfill(q, inst).empty());
}

TEST(backfill_empty_when_sides_class_equal) {
    auto inst = Inst({{A,2},{M,1}}, {{A,2},{M,1}}, 3000, 3000);
    std::vector<QueuedParty> q;
    q.push_back(tu::Solo(A, 0));
    CHECK(StrictBalance::PlanDeficitBackfill(q, inst).empty());
}

TEST(backfill_prefers_priority_candidate) {
    auto inst = Inst({{A,2}}, {{A,1}}, 2000, 1000);
    std::vector<QueuedParty> q;
    q.push_back(tu::Solo(A, 0, "older"));
    q.push_back(tu::Solo(A, 1, "priority"));
    q.back().members[0].fairness.exclusion_count = 3;
    auto inv = StrictBalance::PlanDeficitBackfill(q, inst);
    CHECK_EQ((int)inv.size(), 1);
    CHECK_EQ(inv[0].party->members[0].session_guid, std::string("priority"));
}

TEST(pair_rejected_when_sides_not_equal) {
    auto uneven_size  = Inst({{A,2}}, {{A,1}}, 2000, 1000);
    auto uneven_class = Inst({{A,1},{M,1}}, {{A,2}}, 2000, 2000);
    std::vector<QueuedParty> q;
    q.push_back(tu::Solo(A, 0)); q.push_back(tu::Solo(A, 1));
    CHECK(StrictBalance::PlanPairJoin(q, uneven_size).empty());
    CHECK(StrictBalance::PlanPairJoin(q, uneven_class).empty());
}

TEST(pair_admitted_orientation_narrows_diff) {
    // Sides 2v2, means 1100 vs 900 (diff 200). Solos 900 + 1100 admitted:
    // best orientation puts the 900 on TF1 -> means 1033.3 vs 966.7.
    auto inst = Inst({{A,2}}, {{A,2}}, 2200, 1800);
    std::vector<QueuedParty> q;
    q.push_back(tu::Solo(A, 0, "low"));  q.back().members[0].mmr = 900.0;
    q.push_back(tu::Solo(A, 1, "high")); q.back().members[0].mmr = 1100.0;
    auto inv = StrictBalance::PlanPairJoin(q, inst);
    CHECK_EQ((int)inv.size(), 2);
    int low_tf = 0, high_tf = 0;
    for (auto& iv : inv) {
        if (iv.party->members[0].session_guid == "low")  low_tf = iv.tf;
        if (iv.party->members[0].session_guid == "high") high_tf = iv.tf;
    }
    CHECK_EQ(low_tf, 1);
    CHECK_EQ(high_tf, 2);
}

TEST(pair_rejected_when_it_would_widen) {
    // Balanced 1000-mean sides (diff 0); a 2000 + 1000 pair widens the diff
    // to ~333 in either orientation -> rejected. (An equal-MMR pair keeps
    // the diff unchanged and IS accepted — don't test with symmetric MMRs.)
    auto inst = Inst({{A,2}}, {{A,2}}, 2000, 2000);
    std::vector<QueuedParty> q;
    q.push_back(tu::Solo(A, 0)); q.back().members[0].mmr = 2000.0;
    q.push_back(tu::Solo(A, 1)); q.back().members[0].mmr = 1000.0;
    CHECK(StrictBalance::PlanPairJoin(q, inst).empty());
}

TEST(pair_requires_same_class_and_two_seats) {
    auto inst = Inst({{A,2}}, {{A,2}}, 2000, 2000);
    std::vector<QueuedParty> q;
    q.push_back(tu::Solo(A, 0)); q.push_back(tu::Solo(M, 1));
    CHECK(StrictBalance::PlanPairJoin(q, inst).empty());   // classes differ

    auto full = Inst({{A,2}}, {{A,2}}, 2000, 2000, /*max_players=*/5);
    std::vector<QueuedParty> q2;
    q2.push_back(tu::Solo(A, 0)); q2.push_back(tu::Solo(A, 1));
    CHECK(StrictBalance::PlanPairJoin(q2, full).empty());  // 1 seat free
}

namespace {

QueueConfig StrictCfg() {
    QueueConfig cfg;
    cfg.queue_id = 2;
    cfg.taskforce_policy = TaskforcePolicy::BalancedPvp;
    cfg.team_policy = TeamPolicy::Mixed;
    cfg.team_side_policy = TeamSidePolicy::Preferred;
    cfg.min_players_to_pop = 2;
    cfg.max_players_per_side = 20;
    cfg.strict_class_balance = true;
    cfg.late_join_policy = LateJoinPolicy::BackfillOnly;
    cfg.pair_backfill = true;
    return cfg;
}

}  // namespace

TEST(rule_strict_pop_is_class_equal_sealed_and_capped) {
    auto cfg = StrictCfg();
    CoopMatchRule rule(&cfg);
    std::vector<QueuedParty> q;
    for (int i = 0; i < 3; ++i) q.push_back(tu::Solo(A, i));      // one A excluded
    for (int i = 0; i < 2; ++i) q.push_back(tu::Solo(M, 10 + i));
    auto r = rule.Evaluate(q, {});
    CHECK(r.has_value());
    CHECK_EQ((int)r->session_guids.size(), 4);
    CHECK(r->access_mode == AccessMode::BackfillOnly);
    CHECK(r->cap_override.has_value());
    CHECK_EQ((int)*r->cap_override, 4);
    // Exact per-class split across sides.
    int a1 = 0, a2 = 0, m1 = 0, m2 = 0;
    for (const auto& [guid, tf] : r->task_force_assignments) {
        uint32_t cls = r->profile_ids.at(guid);
        if (cls == A) (tf == 1 ? a1 : a2)++;
        if (cls == M) (tf == 1 ? m1 : m2)++;
    }
    CHECK_EQ(a1, 1); CHECK_EQ(a2, 1);
    CHECK_EQ(m1, 1); CHECK_EQ(m2, 1);
}

TEST(rule_strict_no_valid_subset_returns_nothing) {
    auto cfg = StrictCfg();
    CoopMatchRule rule(&cfg);
    std::vector<QueuedParty> q;
    q.push_back(tu::Solo(A, 0));
    q.push_back(tu::Solo(M, 1));
    auto r = rule.Evaluate(q, {});
    CHECK(!r.has_value());
}

TEST(rule_backfill_routes_into_backfill_only_instance) {
    auto cfg = StrictCfg();
    CoopMatchRule rule(&cfg);
    RunningInstance inst;
    inst.instance_id = 42;
    inst.map_name = "pvp_map"; inst.game_mode = "tdm";
    inst.access_mode = AccessMode::BackfillOnly;
    inst.team1.class_counts[A] = 2; inst.team1.size = 2; inst.team1.mmr_sum = 2000;
    inst.team2.class_counts[A] = 1; inst.team2.size = 1; inst.team2.mmr_sum = 1000;
    inst.player_count = 3;
    std::vector<QueuedParty> q;
    q.push_back(tu::Solo(A, 0, "fill"));
    auto r = rule.Evaluate(q, {inst});
    CHECK(r.has_value());
    CHECK(r->existing_instance_id.has_value());
    CHECK_EQ((long long)*r->existing_instance_id, 42ll);
    CHECK_EQ(r->task_force_assignments.at("fill"), 2);
    CHECK_EQ(r->map_name, std::string("pvp_map"));
}

TEST(rule_never_drops_into_backfill_only_without_vacancy) {
    // Class-equal live match, pair_backfill=false -> nothing enters.
    auto cfg = StrictCfg();
    cfg.pair_backfill = false;
    CoopMatchRule rule(&cfg);
    RunningInstance inst;
    inst.instance_id = 42;
    inst.access_mode = AccessMode::BackfillOnly;
    inst.team1.class_counts[A] = 2; inst.team1.size = 2; inst.team1.mmr_sum = 2000;
    inst.team2.class_counts[A] = 2; inst.team2.size = 2; inst.team2.mmr_sum = 2000;
    inst.player_count = 4;
    std::vector<QueuedParty> q;
    q.push_back(tu::Solo(A, 0));   // single A: no pair, no fresh pop (odd)
    auto r = rule.Evaluate(q, {inst});
    CHECK(!r.has_value());
}

TEST(rule_pair_join_admits_two_via_instances) {
    auto cfg = StrictCfg();
    CoopMatchRule rule(&cfg);
    RunningInstance inst;
    inst.instance_id = 42;
    inst.map_name = "pvp_map"; inst.game_mode = "tdm";
    inst.access_mode = AccessMode::BackfillOnly;
    inst.team1.class_counts[A] = 2; inst.team1.size = 2; inst.team1.mmr_sum = 2000;
    inst.team2.class_counts[A] = 2; inst.team2.size = 2; inst.team2.mmr_sum = 2000;
    inst.player_count = 4;
    std::vector<QueuedParty> q;
    q.push_back(tu::Solo(A, 0, "p1"));
    q.push_back(tu::Solo(A, 1, "p2"));
    auto r = rule.Evaluate(q, {inst});
    CHECK(r.has_value());
    CHECK(r->existing_instance_id.has_value());
    CHECK_EQ((int)r->session_guids.size(), 2);
    const int tf1 = r->task_force_assignments.at("p1");
    const int tf2 = r->task_force_assignments.at("p2");
    CHECK(tf1 != tf2);
}

TEST(rule_open_policy_unchanged_defaults) {
    // Defaults off: behaves exactly as today (drop-in Open, no cap_override).
    QueueConfig cfg;
    cfg.queue_id = 2;
    cfg.taskforce_policy = TaskforcePolicy::BalancedPvp;
    cfg.team_policy = TeamPolicy::Mixed;
    cfg.max_players_per_side = 20;
    CoopMatchRule rule(&cfg);
    std::vector<QueuedParty> q;
    q.push_back(tu::Solo(A, 0));
    q.push_back(tu::Solo(M, 1));   // odd counts — loose mode pops anyway
    auto r = rule.Evaluate(q, {});
    CHECK(r.has_value());
    CHECK_EQ((int)r->session_guids.size(), 2);
    CHECK(r->access_mode == AccessMode::Open);
    CHECK(!r->cap_override.has_value());
}

// --- Fairness rotation (design 2026-09-07) ---------------------------------

TEST(strict_tiebreak_exclusion_rate_spares_the_worse_off) {
    // Three recons, all excluded once since their last match. The tie-break
    // spares whoever has borne the largest SHARE of exclusions historically.
    std::vector<QueuedParty> q;
    q.push_back(tu::Solo(R, 0, "veteran"));
    q.back().members[0].fairness.exclusion_count     = 1;
    q.back().members[0].fairness.lifetime_exclusions = 8;
    q.back().members[0].fairness.played_count        = 2;   // rate 0.80
    q.push_back(tu::Solo(R, 1, "middle"));
    q.back().members[0].fairness.exclusion_count     = 1;
    q.back().members[0].fairness.lifetime_exclusions = 2;
    q.back().members[0].fairness.played_count        = 8;   // rate 0.20
    q.push_back(tu::Solo(R, 2, "lucky"));
    q.back().members[0].fairness.exclusion_count     = 1;
    q.back().members[0].fairness.lifetime_exclusions = 0;
    q.back().members[0].fairness.played_count        = 9;   // rate 0.00

    auto chosen = StrictBalance::SelectClassEqualSubset(q, -1);
    CHECK_EQ(Members(chosen), 2);          // 3 recons -> parity drops exactly one
    CHECK(Contains(chosen, "veteran"));
    CHECK(Contains(chosen, "middle"));
    CHECK(!Contains(chosen, "lucky"));     // lowest rate pays this time
}

TEST(strict_recent_exclusion_outranks_rate) {
    // exclusion_count is still the primary key: a player excluded last pop is
    // spared even though his lifetime rate is the lowest in the pool.
    std::vector<QueuedParty> q;
    q.push_back(tu::Solo(R, 0, "heavy_history"));
    q.back().members[0].fairness.exclusion_count     = 0;
    q.back().members[0].fairness.lifetime_exclusions = 9;
    q.back().members[0].fairness.played_count        = 1;   // rate 0.90
    q.push_back(tu::Solo(R, 1, "just_excluded"));
    q.back().members[0].fairness.exclusion_count     = 1;
    q.back().members[0].fairness.lifetime_exclusions = 1;
    q.back().members[0].fairness.played_count        = 20;  // rate 0.05
    q.push_back(tu::Solo(R, 2, "fresh"));

    auto chosen = StrictBalance::SelectClassEqualSubset(q, -1);
    CHECK_EQ(Members(chosen), 2);
    CHECK(Contains(chosen, "just_excluded"));
    CHECK(Contains(chosen, "heavy_history"));
    CHECK(!Contains(chosen, "fresh"));
}

TEST(pair_admitted_within_absolute_slack) {
    // Dead-level sides (diff 0). A 1240+1000 pair widens the gap to 80, which
    // the old non-widening gate refused and the 100 slack now admits — this is
    // the case that left excluded players sitting out a whole match.
    auto inst = Inst({{A,2}}, {{A,2}}, 2000, 2000);
    std::vector<QueuedParty> q;
    q.push_back(tu::Solo(A, 0, "hi")); q.back().members[0].mmr = 1240.0;
    q.push_back(tu::Solo(A, 1, "lo")); q.back().members[0].mmr = 1000.0;
    auto inv = StrictBalance::PlanPairJoin(q, inst);
    CHECK_EQ((int)inv.size(), 2);
    CHECK(inv[0].tf != inv[1].tf);        // one per side — parity preserved
}

TEST(pair_refused_beyond_absolute_slack) {
    // Same shape, but 1600+1000 leaves a 200 gap — past the slack.
    auto inst = Inst({{A,2}}, {{A,2}}, 2000, 2000);
    std::vector<QueuedParty> q;
    q.push_back(tu::Solo(A, 0, "hi")); q.back().members[0].mmr = 1600.0;
    q.push_back(tu::Solo(A, 1, "lo")); q.back().members[0].mmr = 1000.0;
    CHECK(StrictBalance::PlanPairJoin(q, inst).empty());
}

TEST(pair_admitted_when_it_narrows_a_wide_gap) {
    // Already lopsided (means 1300 vs 1000). A pair that closes the gap is
    // admitted regardless of the slack, because it does not widen.
    auto inst = Inst({{A,2}}, {{A,2}}, 2600, 2000);
    std::vector<QueuedParty> q;
    q.push_back(tu::Solo(A, 0, "lo")); q.back().members[0].mmr = 1000.0;
    q.push_back(tu::Solo(A, 1, "hi")); q.back().members[0].mmr = 1600.0;
    auto inv = StrictBalance::PlanPairJoin(q, inst);
    CHECK_EQ((int)inv.size(), 2);
}
