#pragma once
#include <cstdint>
#include <string>

// Agenda Points earned from match / AvA / endgame sources. Base amounts are
// in the config block "ap_rewards"; every grant is logged and kept in
// ga_ap_grants. An optional "ap_grant" service may set the amount; without
// one the base amount is paid up to a per-agent daily cap.
namespace ApRewards {

struct Settings {
    // PvP (merc / arena / PvP queues).
    int64_t pvp_finish = 20, pvp_win = 40, pvp_rank1 = 30, pvp_rank2 = 20, pvp_rank3 = 10;
    int     pvp_min_presence_percent = 50;
    int     pvp_min_real_per_side = 0;  // 0 = off: solo + bots pays (user 11-06); presence + activity rules still apply
    // AvA / endgame events.
    int64_t ava_win = 75, ava_capture = 100, ava_defend = 100;
    int64_t season_rank1 = 1000, season_rank2 = 600, season_rank3 = 300;
    int64_t raid_min = 150, raid_max = 300;
    // Per-agent per-UTC-day cap on capped sources.
    int64_t daily_cap = 2000;
    int     bot_match_percent = 50;   // most of the other side is bots
};

void Configure(const Settings& s);
const Settings& Get();
void EnsureSchema();

// Grants AP for `base` to the account of `character_id`. `scaled` and
// `capped` are passed to the ap_grant service; built-in rule: `capped`
// grants stop at the daily cap.
// Returns the AP actually added (0 if none).
int64_t Grant(int64_t character_id, int64_t level, const std::string& source,
              int64_t base, bool scaled, bool capped);

}  // namespace ApRewards
