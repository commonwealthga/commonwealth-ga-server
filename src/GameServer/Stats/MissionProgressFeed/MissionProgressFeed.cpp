#include "src/GameServer/Stats/MissionProgressFeed/MissionProgressFeed.hpp"

#include "lib/nlohmann/json.hpp"

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

// Same "Class Pkg.Name" comparison this codebase already uses everywhere
// else to branch on game mode (see TgGame__InitGameRepInfo.cpp's
// GameClassName == "Class TgGame.TgGame_X" checks) -- kept consistent
// rather than inventing a second convention.
//
// TgGame_Ticket/_Escort/_Defense/_PointRotation map 1:1 to the ticket/
// payload/raid/koth categories. TgGame_Mission is the shared base class
// both the "Breach" 3-point sequential-capture maps and a handful of
// unrelated solo/PvE missions run under (confirmed via ga_map_pool_entries
// -- there's no separate Breach class to key off); since the underlying
// ATgMissionObjective priority/lock/capture fields are identical either
// way, mapping the whole class to "breach" is accurate for the maps that
// matter and harmless (an objective list nobody looks at) for the rest.
// City/OpenWorldPVE/OpenWorldPVP/CTF/DualCTF have no requested overlay
// section, so they fall through to "".
std::string ResolveMissionMode(ATgGame* Game) {
    if (!Game || !Game->Class) return "";
    const char* raw = Game->Class->GetFullName();
    const std::string className(raw ? raw : "");
    if (className == "Class TgGame.TgGame_Ticket")        return "ticket";
    if (className == "Class TgGame.TgGame_Escort")        return "payload";
    if (className == "Class TgGame.TgGame_Defense")       return "raid";
    if (className == "Class TgGame.TgGame_PointRotation") return "koth";
    if (className == "Class TgGame.TgGame_Mission")       return "breach";
    return "";
}

void CollectObjectives(TArray<ATgMissionObjective*>& arr, nlohmann::json& out) {
    if (!arr.Data) return;
    for (int i = 0; i < arr.Num(); i++) {
        ATgMissionObjective* obj = arr.Data[i];
        if (!obj) continue;

        nlohmann::json o;
        o["id"]              = obj->nObjectiveId;
        o["priority"]        = obj->nPriority;
        o["locked"]          = (bool)obj->r_bIsLocked;
        o["active"]          = (bool)obj->r_bIsActive;
        o["owner_taskforce"] = obj->r_nOwnerTaskForce;
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

// Boss = the attacker-owned TgMissionObjective_Bot (nDefaultOwnerTaskForce
// == 1), same convention ObjectiveBotOutcome() uses to find the raid boss
// among m_MissionObjectives. Health/max are the same ATgPawn fields the
// pawn-health feed already trusts (r_nHealthMaximum, not HealthMax).
void CollectBossHealth(TArray<ATgMissionObjective*>& arr, int& outHealth, int& outHealthMax) {
    outHealth = 0;
    outHealthMax = 0;
    if (!arr.Data) return;
    for (int i = 0; i < arr.Num(); i++) {
        ATgMissionObjective* obj = arr.Data[i];
        if (!obj || !ObjectClassCache::ClassNameContains((UObject*)obj, "TgMissionObjective_Bot")) continue;
        ATgMissionObjective_Bot* bot = (ATgMissionObjective_Bot*)obj;
        if (bot->nDefaultOwnerTaskForce != 1) continue;
        if (bot->r_ObjectiveBot) {
            outHealth    = bot->r_ObjectiveBot->Health;
            outHealthMax = bot->r_ObjectiveBot->r_nHealthMaximum;
        }
        break;
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
    // showing it, hence the overlay's new "points captured" pip row.
    if (mode == "ticket" || mode == "koth") {
        j["attacker_points"] = GTeamsData.Attackers ? GTeamsData.Attackers->r_nCurrentPointCount : 0;
        j["defender_points"] = GTeamsData.Defenders ? GTeamsData.Defenders->r_nCurrentPointCount : 0;
        j["points_to_win"]   = GRI->r_nPointsToWin;
    }

    if (mode == "raid") {
        ATgGame_Defense* GameDef = (ATgGame_Defense*)Game;
        // s_nRoundNumber is already the 1-based "round in progress" number
        // once a round starts (see MissionVODirector.cpp's `round < 1`
        // pre-round guard) -- shown as-is, not re-derived.
        j["round_current"] = GameDef->s_nRoundNumber;
        j["round_max"]      = GameDef->s_nMaxRoundNumber;

        int bossHealth = 0, bossHealthMax = 0;
        CollectBossHealth(GRI->m_MissionObjectives, bossHealth, bossHealthMax);
        j["boss_health"]     = bossHealth;
        j["boss_health_max"] = bossHealthMax;
    } else {
        // ticket / koth / payload / breach -- all carry the priority-ordered
        // objective-capture list. Ticket's 3 control points are always
        // active simultaneously (rendered as 3 small bars); koth uses
        // whichever single one is currently active/unlocked for its one big
        // tug-of-war bar; payload/breach render every entry as a tile.
        nlohmann::json objectives = nlohmann::json::array();
        CollectObjectives(GRI->m_MissionObjectives, objectives);
        j["objectives"] = objectives;
    }

    IpcClient::Send(j.dump());
}

} // namespace MissionProgressFeed
