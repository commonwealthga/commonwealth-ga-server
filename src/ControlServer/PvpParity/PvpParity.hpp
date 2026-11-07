#pragma once

#include <cstdint>
#include <string>

struct sqlite3;

// PvP queues and rules: data migration, deserter penalty, merc brackets, team challenge.
//
// Migrate(): one-shot data migration (cs_migration_markers
//   'pvp_parity_0510_2026_11_01') so a fresh DB matches the live one:
//   - queue 70 arena_4v4        "4v4 Timed Arena" (msg 41470/41469), lv30, arena pool 70,
//     always open (no timer = our low-pop rule); arena win loot via queue type 1421.
//   - queue 71 arena_4v4_scrim  "4v4 Scrimmage"   (msg 61672/61671), same pool, NO reward.
//   - queue 72 ava_scrimmage    "AVA Scrimmage"   (msg 61104/61103: "scrimmage in AVA maps.
//     NO REWARD", lv10), pool 71 = the HEX_AVA escort maps, which leave the merc pool.
//   - Beta Maps (queue 18): pool 7 gets Colony Caverns (CTR_Recursive_P, 1.5 news) and it
//     leaves the merc pool; operator verification gate off (it is a normal map now).
//   - merc pool 2: the 4v4 arena maps stay for small pops only (max_players 8).
//   "No reward" = reward_level_max 0 on a PvP queue (checked in IpcServer).
// Deserter (msg 56080 "15-minute deserter penalty"): a player who left a running
//   balanced-PvP match before it ended, while the other side had a human, cannot join a
//   PvP queue for cs_settings 'pvp_deserter_seconds' (default 900, 0 = off).
// Merc brackets (msg 41471 "two brackets: level 10-29, and level 30+"):
//   cs_settings 'pvp_merc_brackets' = 'one' (default, low population) | 'original'.
//   'original' routes level 30+ merc joins to hidden queue 73 merc_30 (same maps).
// /challenge (news 4559): an accepted challenge queues both leaders' parties
//   into hidden queue 74 'challenge' (rule VersusSides = party vs party, PARTY_LOCKED,
//   msg 62819 "Team Challenge", 4 per side, pool 72 = the 8 challenge maps of msg 62818,
//   no rewards). At the pop ChallengePop() puts the chosen map and the challenger's
//   Attacker/Defender side on the match.
struct MatchResult;
namespace PvpParity {

constexpr uint32_t kChallengeQueue = 74;
void LaunchChallenge(const std::string& from_guid, const std::string& from_name,
                     const std::string& to_guid, const std::string& to_name,
                     uint32_t map_game_id, bool challenger_defends);
void ChallengePop(MatchResult& r);   // MatchmakingService, queue 74 only
// IpcServer MISSION_ENDED -> challenge participants get metric 1588.
void OnMatchEnded(int64_t instance_id, uint32_t queue_id);

void Migrate(sqlite3* db);

// Called first in TcpSession::handle_match_join. Returns the queue id to
// join (maybe re-routed), or 0 = refused (the player has been told why).
uint32_t GateJoin(const std::string& session_guid, uint32_t queue_id, uint32_t match_filters = 0);

}  // namespace PvpParity
