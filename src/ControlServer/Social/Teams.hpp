#pragma once
// Strike force, raid groups, raid move/promote, raid chat,
// AvA leaderboard (HEX_GET_LEADERBOARD 0x1D9) and team challenge popups (CHALLENGE_REQUEST 0x1F3).
// Client wire format mined from GlobalAgenda.exe:
//  - a strike force / raid group is the SAME team object; TEAM_UPDATE 0x51 roster carries
//    TEAM_MODE (0/1 team, 2 strike force, 3 raid group). Raid sub-teams ("task teams" 1..6) and
//    their leaders come in a second TEAM_UPDATE with SETTING_FLAGS (settings branch 0x1092a360):
//    DATA_SET rows {CHARACTER_ID, TASK_TEAM, LEADER_CHARACTER_ID}.
//  - C->S: 0x48 Convert To Strikeforce (no fields), 0x49 Convert To Raid (no fields),
//    0x13F RAID_MOVE_PLAYER {CHARACTER_ID, TASK_TEAM | TO_CHARACTER_ID}, 0x140 RAID_PROMOTE_PLAYER
//    {CHARACTER_ID}, 0x1D9 {HEX_SEASON_NBR}, 0x1F3 issue {MAP_GAME_ID, PLAYER_NAME, DEFENSE_FLAG,
//    SYS_SITE_ID} / answer {ACCEPT_FLAG}. TEAM_MERGE 0x52/0x53 have no sender in this client.
//  - replies without a dedicated handler = same packet id + MSG_ID (generic status handler).
// State lives here; TeamService / TcpSession / ChatSession only call in (1-3 line hooks).
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_set>

class PacketView;

namespace Social {
namespace Teams {

// ---- TeamService hooks (may be called while TeamService::mutex_ is held: never call back) ----
uint32_t ModeFor(uint64_t team_id);           // TEAM_MODE for the roster (1 = plain team)
size_t   MaxMembers(uint64_t team_id);        // squad / strike force / raid group size cap
size_t   DisbandBelow(uint64_t team_id);      // team disbands when members <= this (1 squad, 0 SF/raid)
void     Forget(uint64_t team_id);            // team disbanded

// ---- called without TeamService locks ----
// After every roster broadcast: reconcile task teams + push the settings-branch rows (mode >= 2).
void AfterRoster(uint64_t team_id);
// handle_match_join: true = this party may not use matchmade queues (strike force rule, msg 32417).
bool QueueBlocked(const std::string& session_guid, uint32_t map_pool_id);

// TcpSession handle_packet default branch: true = handled.
bool HandlePacket(const std::string& session_guid, const std::string& player_name,
                  uint16_t packet_type, const PacketView& pkt);

// ChatSession RecipientsFor: raid chat (6) = whole raid group; team chat (5) inside a raid group =
// own task team. nullopt = not ours (fall through). Empty set = sender may not use the channel.
std::optional<std::unordered_set<std::string>> ChatRecipients(uint32_t channel, const std::string& sender_guid);
// ChatSession broadcast: /rgw /raidw raid warning (ch 6 + DISPLAY_AS_ALERT). true = handled.
bool HandleRaidAlert(const std::string& sender_guid, const std::string& sender_name, const uint8_t* frame, size_t length);

void RaidCommand(const std::string& session_guid);   // chat command -raid

} // namespace Teams
} // namespace Social
