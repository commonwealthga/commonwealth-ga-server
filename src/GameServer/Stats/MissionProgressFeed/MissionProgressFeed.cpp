#include "src/GameServer/Stats/MissionProgressFeed/MissionProgressFeed.hpp"

#include "lib/nlohmann/json.hpp"

#include "src/Config/Config.hpp"
#include "src/GameServer/Engine/MapGameInfo/MapGameInfo.hpp"
#include "src/GameServer/Engine/ObjectiveNames/ObjectiveNames.hpp"
#include "src/GameServer/GameModes/SuperAgent/SuperAgent.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/GameServer/Storage/ActiveSpectatorCount/ActiveSpectatorCount.hpp"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/IpcClient/IpcClient.hpp"
#include "src/Shared/IpcProtocol.hpp"

namespace MissionProgressFeed {

namespace {

// Once/sec is plenty -- mission state (tickets, capture timers, round
// number) changes far slower than pawn health, and there's only one push
// per match (not per-pawn) either way.
constexpr DWORD kPushIntervalMs = 1000;

DWORD g_lastPushMs = 0;

// gameplay_type_value_id -> mode. Confirmed against a live map_game_info
// export (2026-07-30) -- this is the REAL differentiator between modes that
// share one TgGame class (TgGame_Mission covers both Breach and solo/PvE
// missions; TgGame_PointRotation covers both standard and small-map koth).
// This repo's own dev server.db predates the column entirely (map_game_info
// doesn't even exist there), so MapGameInfo::LookupByNameAndGameMode returns
// nullopt and ResolveMissionMode falls back to the original class-name-only
// guess for that case.
//   1544 = Breach            1545 = Ticket/Control      1547 = Payload/Push
//   1548 = koth (standard, 5 points)   1569 = koth (small map, 3 points) --
//     mechanically identical, same "koth" mode either way
//   1550 = Raid (boss + a defended "friendly" NPC/reactor + wave count)
//   1553 = PvE mission (boss bar) -- EXCEPT Super Agent, which is a boss
//     then two sequential captures; detected via SuperAgent::IsActive(),
//     not a distinct gameplay_type value of its own
//   1542 = Home (not spectatable), 1546 = stock CTR/DualCTF (not played) --
//     both intentionally fall through to "" (no overlay section)
std::string ModeForGameplayType(int gameplayTypeValueId) {
    switch (gameplayTypeValueId) {
        case 1544: return "breach";
        case 1545: return "ticket";
        case 1547: return "payload";
        case 1548:
        case 1569: return "koth";
        case 1550: return "raid";
        case 1553: return SuperAgent::IsActive() ? "superagent" : "pve";
        default:   return "";
    }
}

// Same "Class Pkg.Name" comparison this codebase already uses everywhere
// else to branch on game mode (see TgGame__InitGameRepInfo.cpp's
// GameClassName == "Class TgGame.TgGame_X" checks).
std::string ResolveMissionMode(ATgGame* Game) {
    if (!Game || !Game->Class) return "";
    const char* raw = Game->Class->GetFullName();
    const std::string className(raw ? raw : "");
    const std::string classStripped =
        (className.rfind("Class ", 0) == 0) ? className.substr(6) : className;

    const std::string mapName = Config::GetMapNameChar();
    const auto mapRow = MapGameInfo::LookupByNameAndGameMode(mapName, classStripped);
    if (mapRow && mapRow->gameplay_type_value_id != 0) {
        return ModeForGameplayType(mapRow->gameplay_type_value_id);
    }

    // No map_game_info row (older/local DB predating the column) -- fall
    // back to the class-name-only behavior this feed originally shipped
    // with. City/OpenWorldPVE/OpenWorldPVP/CTF/DualCTF have no requested
    // overlay section, so they (and anything else unrecognized) fall
    // through to "".
    if (classStripped == "TgGame.TgGame_Ticket")        return "ticket";
    if (classStripped == "TgGame.TgGame_Escort")        return "payload";
    if (classStripped == "TgGame.TgGame_Defense")       return "raid";
    if (classStripped == "TgGame.TgGame_PointRotation") return "koth";
    if (classStripped == "TgGame.TgGame_Mission")       return SuperAgent::IsActive() ? "superagent" : "breach";
    return "";
}

void CollectObjectives(TArray<ATgMissionObjective*>& arr, nlohmann::json& out) {
    if (!arr.Data) return;
    for (int i = 0; i < arr.Num(); i++) {
        ATgMissionObjective* obj = arr.Data[i];
        if (!obj) continue;

        nlohmann::json o;
        o["id"]              = obj->nObjectiveId;
        // May be "" (no DB row/translation, or this DB predates those
        // tables) -- the overlay falls back to numbering in that case, and
        // must ALSO fall back when multiple concurrently-shown points
        // resolve to the SAME name, since custom-spawned pools (koth, at
        // least some Payload maps) reuse one nObjectiveId across every
        // point in the pool. See ObjectiveNames.hpp.
        o["name"]             = ObjectiveNames::LookupFriendlyName(obj->nObjectiveId);
        o["priority"]         = obj->nPriority;
        o["locked"]           = (bool)obj->r_bIsLocked;
        o["active"]           = (bool)obj->r_bIsActive;
        o["owner_taskforce"]  = obj->r_nOwnerTaskForce;
        // Same percent-progress formula TgGame's own SuperAgent capture-bar
        // logic uses (SuperAgent.cpp) -- not clamped there either; the
        // overlay HTML already clamps on the render side (see power/health
        // pill fill math).
        o["capture_pct"] = obj->m_fTimeToCapture > 0.0f
            ? obj->m_fCurrCaptureTime / obj->m_fTimeToCapture
            : 0.0f;
        out.push_back(o);
    }
}

// The FIRST TgMissionObjective_Bot found, no owner filtering -- matches
// SuperAgent::Init()'s own boss-finding loop exactly. Solo PvE/Super Agent
// maps have no second "defended NPC" bot to disambiguate against, unlike
// Raid (see CollectRaidBots below), so there's nothing to filter by.
void CollectSoloBoss(TArray<ATgMissionObjective*>& arr,
        int& outHealth, int& outHealthMax, std::string& outName) {
    outHealth = 0;
    outHealthMax = 0;
    outName.clear();
    if (!arr.Data) return;
    for (int i = 0; i < arr.Num(); i++) {
        ATgMissionObjective* obj = arr.Data[i];
        if (!obj || !ObjectClassCache::ClassNameContains((UObject*)obj, "TgMissionObjective_Bot")) continue;
        ATgMissionObjective_Bot* bot = (ATgMissionObjective_Bot*)obj;
        outName = ObjectiveNames::LookupFriendlyName(obj->nObjectiveId);
        if (bot->r_ObjectiveBot) {
            outHealth    = bot->r_ObjectiveBot->Health;
            outHealthMax = bot->r_ObjectiveBot->r_nHealthMaximum;
        }
        break;
    }
}

// Raid has TWO bots to tell apart: the attacker-owned boss
// (nDefaultOwnerTaskForce == 1) and a defender-owned "friendly" NPC/reactor
// the defenders are protecting (== 2, e.g. "Bancroft" on
// Raid_DomeCityDefense_P -- see ObjectiveBotOutcome.hpp's exact win/loss
// convention, reused here for display purposes). Either can be absent
// (health_max stays 0) on a map that doesn't use that half of the pattern.
void CollectRaidBots(TArray<ATgMissionObjective*>& arr,
        int& bossHealth, int& bossHealthMax, std::string& bossName,
        int& friendlyHealth, int& friendlyHealthMax, std::string& friendlyName) {
    bossHealth = bossHealthMax = friendlyHealth = friendlyHealthMax = 0;
    bossName.clear();
    friendlyName.clear();
    if (!arr.Data) return;
    for (int i = 0; i < arr.Num(); i++) {
        ATgMissionObjective* obj = arr.Data[i];
        if (!obj || !ObjectClassCache::ClassNameContains((UObject*)obj, "TgMissionObjective_Bot")) continue;
        ATgMissionObjective_Bot* bot = (ATgMissionObjective_Bot*)obj;
        if (bot->nDefaultOwnerTaskForce == 1 && bossHealthMax == 0) {
            bossName = ObjectiveNames::LookupFriendlyName(obj->nObjectiveId);
            if (bot->r_ObjectiveBot) {
                bossHealth    = bot->r_ObjectiveBot->Health;
                bossHealthMax = bot->r_ObjectiveBot->r_nHealthMaximum;
            }
        } else if (bot->nDefaultOwnerTaskForce == 2 && friendlyHealthMax == 0) {
            friendlyName = ObjectiveNames::LookupFriendlyName(obj->nObjectiveId);
            if (bot->r_ObjectiveBot) {
                friendlyHealth    = bot->r_ObjectiveBot->Health;
                friendlyHealthMax = bot->r_ObjectiveBot->r_nHealthMaximum;
            }
        }
    }
}

} // namespace

void MaybePushSnapshot(AActor* actor) {
    if (GActiveSpectatorCount <= 0) return;
    if (!actor) return;

    ATgGame* Game = (ATgGame*)Globals::Get().GGameInfo;
    if (!Game || actor != (AActor*)Game) return;

    const DWORD now = GetTickCount();
    if (now - g_lastPushMs < kPushIntervalMs) return;
    g_lastPushMs = now;

    const std::string mode = ResolveMissionMode(Game);
    if (mode.empty()) return;

    ATgRepInfo_Game* GRI = (ATgRepInfo_Game*)Game->GameReplicationInfo;
    if (!GRI) return;

    nlohmann::json j;
    j["type"]        = IpcProtocol::MSG_MISSION_PROGRESS_SNAPSHOT;
    j["instance_id"] = IpcClient::GetInstanceId();
    j["mode"]        = mode;

    // ticket AND koth both track a first-to-N team score the same way
    // (ATgRepInfo_TaskForce::r_nCurrentPointCount vs r_nPointsToWin) --
    // ATgGame_PointRotation extends ATgGame_Arena, the same round-scoring
    // base TgGame_Defense::FinalizeRoundScore increments. Ticket already
    // surfaces this as the X/points_to_win score bar; koth has nowhere else
    // showing it, hence the overlay's "points captured" pip row.
    if (mode == "ticket" || mode == "koth") {
        j["attacker_points"] = GTeamsData.Attackers ? GTeamsData.Attackers->r_nCurrentPointCount : 0;
        j["defender_points"] = GTeamsData.Defenders ? GTeamsData.Defenders->r_nCurrentPointCount : 0;
        j["points_to_win"]   = GRI->r_nPointsToWin;
    }

    if (mode == "raid") {
        ATgGame_Defense* GameDef = (ATgGame_Defense*)Game;
        // s_nRoundNumber is already the 1-based "round in progress" number
        // once a round starts (see MissionVODirector.cpp's `round < 1`
        // pre-round guard) -- shown as-is, not re-derived. Displayed as
        // "Wave" on the overlay, not "Round" -- same underlying counter.
        j["round_current"] = GameDef->s_nRoundNumber;
        j["round_max"]     = GameDef->s_nMaxRoundNumber;

        int bossHealth, bossHealthMax, friendlyHealth, friendlyHealthMax;
        std::string bossName, friendlyName;
        CollectRaidBots(GRI->m_MissionObjectives,
            bossHealth, bossHealthMax, bossName,
            friendlyHealth, friendlyHealthMax, friendlyName);
        j["boss_health"]         = bossHealth;
        j["boss_health_max"]     = bossHealthMax;
        j["boss_name"]           = bossName;
        j["friendly_health"]     = friendlyHealth;
        j["friendly_health_max"] = friendlyHealthMax;
        j["friendly_name"]       = friendlyName;
    } else if (mode == "pve" || mode == "superagent") {
        int bossHealth, bossHealthMax;
        std::string bossName;
        CollectSoloBoss(GRI->m_MissionObjectives, bossHealth, bossHealthMax, bossName);
        j["boss_health"]     = bossHealth;
        j["boss_health_max"] = bossHealthMax;
        j["boss_name"]       = bossName;

        if (mode == "superagent") {
            // A (the 5-min hold) then B (final, back to the dropship) --
            // both regular ATgMissionObjective_Proximity entries with real
            // priority/lock fields (SuperAgent.cpp's SpawnPoint), so the
            // exact same collector breach/ticket/koth use already reads
            // their progress correctly with no special-casing needed.
            nlohmann::json objectives = nlohmann::json::array();
            CollectObjectives(GRI->m_MissionObjectives, objectives);
            j["objectives"] = objectives;
        }
    } else {
        // ticket / koth / payload / breach -- all carry the priority-ordered
        // objective-capture list. Ticket's 3 control points are always
        // active simultaneously (rendered as 3 small bars); koth uses
        // whichever single one is currently active/unlocked for its one big
        // tug-of-war bar; payload/breach render every entry as a
        // tile/segment.
        nlohmann::json objectives = nlohmann::json::array();
        CollectObjectives(GRI->m_MissionObjectives, objectives);
        j["objectives"] = objectives;
    }

    IpcClient::Send(j.dump());
}

} // namespace MissionProgressFeed
