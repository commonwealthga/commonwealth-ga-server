#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct MapModeEntry;

// Mercenary "choose game types".
// The client's queue screen (UTgUIMissionScene, Mercenary tab) sends MATCH_FILTERS (0x0334)
// on MATCH_JOIN with one bit per UNTICKED game type:
//   0x02 Control (TgGame_Ticket)   0x04 Breach (TgGame_Mission)   0x08 Demolition/Acquisition (TgGame_DualCTF)
//   0x10 Payload (TgGame_Escort)   0x20 Scramble (TgGame_PointRotation)
// (all five unticked -> the client sends 0 = everything). Other bits (0x01/0x100/0x200/0x400) are team
// options and are ignored here.
// At a pop the map pool is narrowed to the entries whose mode every popped player allows; when no entry
// fits everyone (low population) it keeps the entries allowed by the most players, and when nobody
// filtered nothing changes.
class MatchModeFilter {
public:
    static constexpr uint32_t kModeMask = 0x3E;

    // MATCH_JOIN: remember the mask for the joiner (and every team member when a leader queues a team).
    static void OnJoin(const std::string& session_guid, uint32_t match_filters);

    // Bit of a game mode ("TgGame.TgGame_Ticket" ...), 0 = mode is never filtered.
    static uint32_t ModeBit(const std::string& game_mode);

    // Options > "Volunteer me for late backfills" (TgPlayerController
    // c_bLateBackfills, default True). The client ORs 0x400 into MATCH_FILTERS on every MATCH_JOIN when it is
    // ticked (exe 0x1092294c; 0x200 = "quick queue"). Remembered from the client's own MATCH_JOIN only
    // (server-made joins send 0 and keep the last value); unknown = volunteer (the client default).
    static constexpr uint32_t kLateBackfillBit = 0x400;
    // Late = the match has run at least this long (TgGame s_fTimeToSeal default 180 s).
    static constexpr int64_t kLateBackfillAfterSecs = 180;
    static void OnClientJoinFlags(const std::string& session_guid, uint32_t match_filters);
    static bool LateBackfillVolunteer(const std::string& session_guid);

    // Narrow `base` (in place) for the players of one pop. No-op when guids is null/empty or nobody filtered.
    static void Narrow(std::vector<const MapModeEntry*>& base, const std::vector<std::string>* guids,
                       uint32_t queue_id);
};
