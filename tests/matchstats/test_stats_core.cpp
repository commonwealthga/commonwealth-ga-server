#include "check.hpp"
#include "src/GameServer/Stats/StatsCore.hpp"

using Stats::UserMatchStats;
using Stats::StintKey;
using Stats::kNumScores;

namespace {
void SetScores(int* a, int kills, int deaths, int dmg) {
    for (int i = 0; i < kNumScores; i++) a[i] = 0;
    a[1] = kills; a[8] = deaths; a[4] = dmg;
}
}

TEST(single_stint_banks_delta) {
    UserMatchStats u;
    int pri[kNumScores]; SetScores(pri, 0, 0, 0);
    u.BeginStint({100, 1}, pri, 10.0f);
    SetScores(pri, 5, 2, 900);
    u.Bank(pri, 70.0f);
    const auto& s = u.Stints().at({100, 1});
    CHECK_EQ(s.scores[1], 5);
    CHECK_EQ(s.scores[8], 2);
    CHECK_EQ(s.scores[4], 900);
    CHECK(s.seconds_played > 59.9f && s.seconds_played < 60.1f);
}

TEST(class_change_splits_stints) {
    UserMatchStats u;
    int pri[kNumScores]; SetScores(pri, 0, 0, 0);
    u.BeginStint({100, 1}, pri, 0.0f);
    SetScores(pri, 3, 1, 300);            // 3 kills as char 100
    u.BeginStint({200, 1}, pri, 60.0f);   // banks char-100 stint
    SetScores(pri, 7, 1, 800);            // +4 kills as char 200
    u.Bank(pri, 120.0f);
    CHECK_EQ(u.Stints().at({100, 1}).scores[1], 3);
    CHECK_EQ(u.Stints().at({200, 1}).scores[1], 4);
    CHECK_EQ(u.Stints().at({200, 1}).scores[4], 500);
}

TEST(team_change_same_char_splits_stints) {
    UserMatchStats u;
    int pri[kNumScores]; SetScores(pri, 0, 0, 0);
    u.BeginStint({100, 1}, pri, 0.0f);
    SetScores(pri, 2, 0, 200);
    u.BeginStint({100, 2}, pri, 30.0f);
    SetScores(pri, 6, 0, 500);
    u.Bank(pri, 90.0f);
    CHECK_EQ(u.Stints().at({100, 1}).scores[1], 2);
    CHECK_EQ(u.Stints().at({100, 2}).scores[1], 4);
}

TEST(reconnect_restore_no_double_count) {
    UserMatchStats u;
    int pri[kNumScores]; SetScores(pri, 0, 0, 0);
    u.BeginStint({100, 1}, pri, 0.0f);
    SetScores(pri, 4, 1, 400);
    u.CloseStint(pri, 50.0f);             // player leaves

    // Rejoin: fresh PRI (zeros). Restore sum, write into PRI, re-anchor.
    int fresh[kNumScores]; SetScores(fresh, 0, 0, 0);
    u.BeginStint({100, 1}, fresh, 80.0f);
    int restored[kNumScores];
    u.SumScores(restored);
    CHECK_EQ(restored[1], 4);             // scoreboard shows 4 kills again
    u.RebaselineAfterRestore(restored);

    // 2 more kills post-restore — stint must gain exactly 2, not 6.
    int pri2[kNumScores];
    for (int i = 0; i < kNumScores; i++) pri2[i] = restored[i];
    pri2[1] += 2;
    u.Bank(pri2, 140.0f);
    CHECK_EQ(u.Stints().at({100, 1}).scores[1], 6);  // 4 banked + 2 new
}

TEST(bank_without_open_stint_is_noop) {
    UserMatchStats u;
    int pri[kNumScores]; SetScores(pri, 9, 9, 9);
    u.Bank(pri, 10.0f);
    CHECK(!u.HasAnyStint());
}

TEST(open_stint_accumulators_visible_in_stints) {
    UserMatchStats u;
    int pri[kNumScores]; SetScores(pri, 0, 0, 0);
    u.BeginStint({100, 1}, pri, 0.0f);
    u.OpenStint()->capture_seconds += 2.5f;
    u.OpenStint()->beacon_spawns_used += 1;
    CHECK(u.Stints().at({100, 1}).capture_seconds > 2.4f);
    CHECK_EQ(u.Stints().at({100, 1}).beacon_spawns_used, 1);
}
