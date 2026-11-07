#pragma once
// Achievements. Definitions are client-local (assembly.dat);
// the server only stores which ones an agent earned and counts progress.
// Requirement rows = asm_data_set_achievement_reqs.
#include <cstdint>
#include <string>
#include <vector>

namespace AchievementService {

// metric value ids (asm valid values group 132)
constexpr int METRIC_WIN          = 1561;
constexpr int METRIC_TOP_STATS_30 = 1563;
constexpr int METRIC_WIN_VB       = 1564;
constexpr int METRIC_AVA_TOP3     = 1568;
constexpr int METRIC_AVA_FIRST    = 1577;
constexpr int METRIC_RECRUIT      = 1585;
constexpr int METRIC_AVA_MATCH    = 1586;
constexpr int METRIC_CHALLENGE    = 1588;
constexpr int METRIC_STAR_RATING  = 1310;
constexpr int CLASS_ALL           = 1566;  // "across ALL classes" (account-wide)

struct Earned { int achievement_id = 0; int64_t achieved_at = 0; };
struct Unlock { int64_t character_id = 0; int achievement_id = 0; };

void EnsureSchema();
std::vector<Earned> GetEarned(int64_t character_id);
// Achievement loot_table rewards (42 achievements, one appearance helmet each;
// title_msg_id is 0 on all 233 rows, so there are no titles to hand out).
// Called at spawn BEFORE the full inventory push, so the bag never leads the
// client. Returns the number of item stacks granted.
int GrantPendingRewards(int64_t character_id);

// One finished PvE mission: every agent still in the instance on players_tf.
// Win always; Win with Victory Bonus when the team's deaths (ga_instances
// count_deaths_*, written at MISSION_ENDED) stayed under victory_bonus_lives.
std::vector<Unlock> CreditPveMission(int64_t instance_id, int players_tf,
                                     uint32_t queue_difficulty, bool solo,
                                     bool double_agent, bool won);
// PvP Merc match end - "top of the scoreboard in N stats" (metric 1563).
std::vector<Unlock> CreditPvpTopStats(int64_t instance_id, const std::vector<int64_t>& paid);
// AvA season paid out, rank 0..2 (winner / top 3).
std::vector<Unlock> CreditAvaSeason(int64_t character_id, int rank);
// One finished AvA match: AvA Match (Count) for every seated agent.
std::vector<Unlock> CreditAvaMatch(int64_t instance_id);
// +1 on every achievement whose requirement uses metric_value_id `metric` (no map/class filter).
std::vector<Unlock> CreditMetric(int64_t character_id, int metric);
// Star Rating (our combat rating = 1 star / 10 levels) - catch-up check.
std::vector<Unlock> CheckStarRating(int64_t character_id);

} // namespace AchievementService
