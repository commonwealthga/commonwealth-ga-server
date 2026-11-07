#include "src/GameServer/Stats/InstanceRestore/InstanceRestore.hpp"

#include "src/GameServer/Engine/World/GetWorldInfo/World__GetWorldInfo.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/GameServer/Stats/DeployableState/DeployableState.hpp"
#include "src/GameServer/Stats/PetState/PetState.hpp"
#include "src/GameServer/Stats/EnemyState/EnemyState.hpp"
#include "src/GameServer/Stats/EscortState/EscortState.hpp"
#include "src/GameServer/Stats/MatchStats.hpp"
#include "src/GameServer/Stats/PlayerState/PlayerState.hpp"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/GameServer/TgGame/TgGame/IsFinalObjective/TgGame__IsFinalObjective.hpp"
#include "src/GameServer/TgGame/TgMissionObjective/SetObjectivePending/TgMissionObjective__SetObjectivePending.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <algorithm>
#include <cstring>
#include <vector>

namespace InstanceRestore {

namespace {

nlohmann::json g_pending;
bool g_hasPending = false;
DWORD g_appliedAt = 1;   // nothing pending yet
int  g_rotationIndex = -1;

// raid/pve: boss + defended-NPC health as a fraction of max, applied when the
// bot pawn exists (the raid boss only spawns in the last wave). <= 0 = none.
float g_bossFrac = 0.0f;
float g_friendlyFrac = 0.0f;

// ATgGame::m_eTimerState (ETgMissionTimerState): 2 running, 3 overtime.
bool TimerRunning(ATgGame* Game) {
    const int state = (int)Game->m_eTimerState;
    return state == 2 || state == 3;
}

void ApplyTimer(ATgGame* Game, float remaining) {
    // Same re-arm sequence MissionAlerts uses for its endless-timer maps:
    // the pending one-shot MissionTimer must be re-armed, not just the fields.
    AWorldInfo* WorldInfo = World__GetWorldInfo::CallOriginal((UWorld*)Globals::Get().GWorld, nullptr, 0);
    if (!WorldInfo) return;
    Game->s_fMissionTimerStartedAt = WorldInfo->TimeSeconds;
    Game->m_fGameMissionTime       = remaining;
    Game->m_fPausedAtTime          = 0.0f;
    Game->eventSetMissionTime(remaining);
    Game->eventMissionTimerStart();
}

void ApplyObjectives(ATgRepInfo_Game* GRI, const nlohmann::json& list) {
    // Matched by array position, the same order the snapshot was written in
    // (ids/names are not unique per point, see IpcProtocol.hpp).
    if (!list.is_array() || !GRI->m_MissionObjectives.Data) return;
    const int n = GRI->m_MissionObjectives.Num();
    if ((int)list.size() != n) {
        Logger::Log("restore", "[Restore] objective count differs (snapshot %d, map %d) - objectives skipped\n",
            (int)list.size(), n);
        return;
    }
    for (int i = 0; i < n; i++) {
        ATgMissionObjective* Obj = GRI->m_MissionObjectives.Data[i];
        const nlohmann::json& o = list[i];
        if (!Obj || !o.is_object()) continue;
        if (o.value("id", -1) != Obj->nObjectiveId) continue;
        const int owner = o.value("owner_taskforce", Obj->r_nOwnerTaskForce);
        if (owner < 0 || owner > 2) continue;
        Obj->r_nOwnerTaskForce    = owner;
        Obj->m_nCurrOwnerTaskforce = owner;
        if (o.contains("captured_once")) Obj->r_bHasBeenCapturedOnce = o.value("captured_once", false) ? 1 : 0;
        float pct = o.value("capture_pct", 0.0f);
        if (pct < 0.0f) pct = 0.0f;
        if (pct > 1.0f) pct = 1.0f;
        if (Obj->m_fTimeToCapture > 0.0f) {
            Obj->m_fCurrCaptureTime = pct * Obj->m_fTimeToCapture;
            Obj->r_fCurrCaptureTime = Obj->m_fCurrCaptureTime;
        }
        Obj->bNetDirty       = 1;
        Obj->bForceNetUpdate = 1;
    }
}

// payload: the escort segments unlock one after another; bring each
// objective's lock state in line with the snapshot before its fields are set.
void ApplyLocks(ATgRepInfo_Game* GRI, const nlohmann::json& list) {
    if (!list.is_array() || !GRI->m_MissionObjectives.Data || (int)list.size() != GRI->m_MissionObjectives.Num()) return;
    for (int i = 0; i < GRI->m_MissionObjectives.Num(); i++) {
        ATgMissionObjective* Obj = GRI->m_MissionObjectives.Data[i];
        const nlohmann::json& o = list[i];
        if (!Obj || !o.is_object() || o.value("id", -1) != Obj->nObjectiveId) continue;
        const bool locked = o.value("locked", (bool)Obj->r_bIsLocked);
        if (locked == (bool)Obj->r_bIsLocked) continue;
        if (locked) Obj->eventLockObjective();
        else        Obj->eventUnlockObjective(o.value("active", true) ? 1 : 0);
        Logger::Log("restore", "[Restore] objective %d %s\n", i, locked ? "locked" : "unlocked");
    }
}

// MissionObjectiveStatus TGMOS_ATTACKER_CAPTURED. The base TgMissionObjective
// UpdateObjectiveStatus returns early unless bEnabled, so dispatch to the
// subclass UFunction (same pattern as TgGame_PointRotation__UnlockObjective).
void SetObjectiveStatus(ATgMissionObjective* Obj, unsigned char status) {
    if (ObjectClassCache::ClassNameContains((UObject*)Obj, "TgMissionObjective_Escort"))
        ((ATgMissionObjective_Escort*)Obj)->eventUpdateObjectiveStatus(status);
    else if (ObjectClassCache::ClassNameContains((UObject*)Obj, "TgMissionObjective_Proximity"))
        ((ATgMissionObjective_Proximity*)Obj)->eventUpdateObjectiveStatus(status);
    else if (ObjectClassCache::ClassNameContains((UObject*)Obj, "TgMissionObjective_Kismet"))
        ((ATgMissionObjective_Kismet*)Obj)->eventUpdateObjectiveStatus(status);
    else if (ObjectClassCache::ClassNameContains((UObject*)Obj, "TgMissionObjective_Bot"))
        ((ATgMissionObjective_Bot*)Obj)->eventUpdateObjectiveStatus(status);
    else
        Obj->eventUpdateObjectiveStatus(status);
}

// breach: objectives are taken one after another and each capture runs the
// map's own follow-up (next objective unlocked, doors, forward spawns,
// Kismet outputs). Replay every objective the snapshot shows as taken through
// the game's capture status, lowest priority first, so the new instance goes
// through the same steps. The final objective is never replayed (that would
// end the match); objective bots are only marked, their pawn stays.
int ApplyCaptures(ATgGame* Game, ATgRepInfo_Game* GRI, const nlohmann::json& list) {
    if (!list.is_array() || !GRI->m_MissionObjectives.Data || (int)list.size() != GRI->m_MissionObjectives.Num()) return 0;
    std::vector<int> order;
    for (int i = 0; i < GRI->m_MissionObjectives.Num(); i++) {
        ATgMissionObjective* Obj = GRI->m_MissionObjectives.Data[i];
        const nlohmann::json& o = list[i];
        if (!Obj || !o.is_object() || o.value("id", -1) != Obj->nObjectiveId) continue;
        const int status = o.value("status", 0);
        if (!o.value("captured_once", false) && status != 8) continue;
        if (Obj->r_bHasBeenCapturedOnce || Obj->r_eStatus == 8) continue;
        order.push_back(i);
    }
    std::stable_sort(order.begin(), order.end(), [&](int a, int b) {
        return GRI->m_MissionObjectives.Data[a]->nPriority < GRI->m_MissionObjectives.Data[b]->nPriority;
    });
    int done = 0;
    for (int i : order) {
        if (Game->bGameEnded) break;
        ATgMissionObjective* Obj = GRI->m_MissionObjectives.Data[i];
        if (TgGame__IsFinalObjective::Call(Game, nullptr, Obj)) {
            Logger::Log("restore", "[Restore] objective %d is the final one - capture not replayed\n", i);
            continue;
        }
        if (Obj->r_bIsLocked) Obj->eventUnlockObjective(1);
        const int owner = list[i].value("owner_taskforce", 1);
        if (owner >= 1 && owner <= 2) { Obj->r_nOwnerTaskForce = owner; Obj->m_nCurrOwnerTaskforce = owner; }
        SetObjectiveStatus(Obj, 8);
        Obj->r_bHasBeenCapturedOnce = 1;
        Obj->bNetDirty = 1;
        Obj->bForceNetUpdate = 1;
        done++;
        Logger::Log("restore", "[Restore] objective %d (id %d, prio %d) capture replayed -> status %d owner %d\n",
            i, Obj->nObjectiveId, Obj->nPriority, (int)Obj->r_eStatus, Obj->r_nOwnerTaskForce);
    }
    return done;
}

// koth: the first rotation pick usually happens before the restore arrives.
// While that pick is still pending (not unlocked yet) switch it to the saved
// point; once it is live leave it alone (next rotation is random as usual).
void ApplyRotation(ATgGame* Game, ATgRepInfo_Game* GRI) {
    if (g_rotationIndex < 0) return;
    if (!ObjectClassCache::ClassNameContains((UObject*)Game, "TgGame_PointRotation")) { g_rotationIndex = -1; return; }
    ATgGame_PointRotation* Rot = (ATgGame_PointRotation*)Game;
    ATgMissionObjective* Cur = Rot->m_NextObjective;
    if (!Cur) return;  // no pick yet: CalcNextObjective takes the index
    const int want = g_rotationIndex;
    g_rotationIndex = -1;
    if (!GRI->m_MissionObjectives.Data || want >= GRI->m_MissionObjectives.Num()) return;
    ATgMissionObjective* Want = GRI->m_MissionObjectives.Data[want];
    if (!Want || Want == Cur) return;
    if (!Cur->r_bIsPending) {
        Logger::Log("restore", "[Restore] koth point already live - rotation index %d not applied\n", want);
        return;
    }
    TgMissionObjective__SetObjectivePending::Call(Cur, nullptr, 0);
    Rot->m_NextObjective     = Want;
    Rot->m_PreviousObjective = Want;
    TgMissionObjective__SetObjectivePending::Call(Want, nullptr, 1);
    Logger::Log("restore", "[Restore] koth pending point switched to index %d\n", want);
}

float HealthFrac(const nlohmann::json& p, const char* hp, const char* max) {
    const int h = p.value(hp, 0), m = p.value(max, 0);
    if (h <= 0 || m <= 0 || h >= m) return 0.0f;  // dead, unknown or untouched
    return (float)h / (float)m;
}

// ownerTf 1 = raid boss, 2 = raid defended NPC, 0 = first bot (pve/superagent);
// same picks as MissionProgressFeed's CollectRaidBots/CollectSoloBoss.
ATgPawn* FindObjectiveBot(ATgRepInfo_Game* GRI, int ownerTf) {
    if (!GRI->m_MissionObjectives.Data) return nullptr;
    for (int i = 0; i < GRI->m_MissionObjectives.Num(); i++) {
        ATgMissionObjective* Obj = GRI->m_MissionObjectives.Data[i];
        if (!Obj || !ObjectClassCache::ClassNameContains((UObject*)Obj, "TgMissionObjective_Bot")) continue;
        ATgMissionObjective_Bot* Bot = (ATgMissionObjective_Bot*)Obj;
        if (ownerTf != 0 && Bot->nDefaultOwnerTaskForce != ownerTf) continue;
        return Bot->r_ObjectiveBot;
    }
    return nullptr;
}

// Lowers a live bot to the saved fraction (never raises it, never kills it).
// Returns true once done (or nothing to do), false while the bot is not there.
bool ApplyBotHealth(ATgRepInfo_Game* GRI, int ownerTf, float& frac, const char* what) {
    if (frac <= 0.0f) return true;
    ATgPawn* Pawn = FindObjectiveBot(GRI, ownerTf);
    if (!Pawn || Pawn->Health <= 0 || Pawn->r_nHealthMaximum <= 0) return false;
    int want = (int)(frac * (float)Pawn->r_nHealthMaximum + 0.5f);
    if (want < 1) want = 1;
    if (want < Pawn->Health) {
        Pawn->Health = want;
        Pawn->bNetDirty = 1;
        Pawn->bForceNetUpdate = 1;
    }
    Logger::Log("restore", "[Restore] %s health %d/%d\n", what, Pawn->Health, Pawn->r_nHealthMaximum);
    frac = 0.0f;
    return true;
}

void SetGameTimer(ATgGame* Game, FName name, float secs) {
    if (Game->IsTimerActive(name, nullptr)) Game->SetTimer(secs, 0, name, nullptr);
}

// raid (TgGame_Defense): jump forward to the saved wave once the first wave
// is in progress; the wave spawners key on s_nRoundNumber. The untimed boss
// wave is reached through the game's own transition (saved wave - 1 with a
// short timer) so the boss spawns the normal way; its HP is then lowered by
// ApplyBotHealth. Returns false while the raid has not started a wave yet.
bool ApplyRaidWave(ATgGame* Game, ATgRepInfo_Game* GRI, const nlohmann::json& p) {
    ATgGame_Defense* Def = (ATgGame_Defense*)Game;
    const char* st = Game->GetStateName().GetName();
    if (!st || strcmp(st, "RoundInProgress") != 0 || Def->s_nRoundNumber < 1) return false;
    const int maxRound = Def->s_nMaxRoundNumber;
    const int saved = p.value("round_current", 0);
    if (saved <= Def->s_nRoundNumber || maxRound <= 0) return true;
    const bool bossWave = saved >= maxRound;
    const int round = bossWave ? maxRound - 1 : saved;
    if (round > Def->s_nRoundNumber) {
        Def->s_nRoundNumber = round;
        GRI->r_nRoundNumber = round;
        GRI->bNetDirty = 1;
    }
    float secs = bossWave ? 5.0f : p.value("mission_remaining_seconds", 0.0f);
    if (secs > 1.0f) {
        SetGameTimer(Game, FName("RoundTimer"), secs);
        SetGameTimer(Game, FName("MissionTimer"), secs);
    }
    Logger::Log("restore", "[Restore] raid wave %d -> %d (%s, %.0f s)\n", saved, round,
        bossWave ? "boss wave next" : "timer", secs);
    return true;
}

}  // namespace

DWORD AppliedAt() {
    return g_hasPending ? 0 : (g_appliedAt ? g_appliedAt : 1);
}

int TakeRotationIndex() {
    const int idx = g_rotationIndex;
    g_rotationIndex = -1;
    return idx;
}

void SetPending(const nlohmann::json& progress) {
    if (!progress.is_object()) return;
    g_pending    = progress;
    g_hasPending = true;
    g_appliedAt  = 0;
    const std::string mode = progress.value("mode", std::string());
    g_rotationIndex = mode == "koth" ? progress.value("active_index", -1) : -1;
    const bool boss = mode == "raid" || mode == "pve" || mode == "superagent";
    g_bossFrac     = boss ? HealthFrac(progress, "boss_health", "boss_health_max") : 0.0f;
    g_friendlyFrac = mode == "raid" ? HealthFrac(progress, "friendly_health", "friendly_health_max") : 0.0f;
    MatchStats::SetCarriedScores(progress.value("players", nlohmann::json::array()));
    PlayerState::SetCarried(progress.value("players", nlohmann::json::array()));
    EnemyState::SetCarried(progress);
    DeployableState::SetCarried(progress);
    PetState::SetCarried(progress);
    Logger::Log("restore", "[Restore] state received (mode=%s, %.0f s left)\n",
        progress.value("mode", std::string()).c_str(), progress.value("mission_remaining_seconds", 0.0f));
}

void MaybeApply(ATgGame* Game) {
    if (!Game || Game->bGameEnded) { g_bossFrac = g_friendlyFrac = 0.0f; return; }
    ATgRepInfo_Game* GRI = (ATgRepInfo_Game*)Game->GameReplicationInfo;
    if (!GRI) return;
    const bool raid = ObjectClassCache::ClassNameContains((UObject*)Game, "TgGame_Defense");

    // Bot health waits for its pawn (any later tick, until the game ends).
    if (!g_hasPending && (g_bossFrac > 0.0f || g_friendlyFrac > 0.0f)) {
        ApplyBotHealth(GRI, raid ? 1 : 0, g_bossFrac, "boss");
        if (raid) ApplyBotHealth(GRI, 2, g_friendlyFrac, "defended NPC");
    }
    if (!g_hasPending) return;

    const nlohmann::json p = g_pending;
    const std::string mode = p.value("mode", std::string());

    if (mode == "raid") {
        if (!raid || !ApplyRaidWave(Game, GRI, p)) return;
        g_hasPending = false;
        g_appliedAt  = GetTickCount();
        return;
    }
    if (!TimerRunning(Game)) return;
    g_hasPending = false;
    g_appliedAt  = GetTickCount();

    const float remaining = p.value("mission_remaining_seconds", 0.0f);
    if (remaining > 1.0f) ApplyTimer(Game, remaining);

    if (mode == "ticket" || mode == "koth") {
        // Never at or above the win line: a restore must not end the match.
        const int cap = GRI->r_nPointsToWin > 1 ? GRI->r_nPointsToWin - 1 : 0x7fffffff;
        if (GTeamsData.Attackers) GTeamsData.Attackers->r_nCurrentPointCount = (std::min)(cap, (std::max)(0, p.value("attacker_points", 0)));
        if (GTeamsData.Defenders) GTeamsData.Defenders->r_nCurrentPointCount = (std::min)(cap, (std::max)(0, p.value("defender_points", 0)));
        if (GTeamsData.Attackers) GTeamsData.Attackers->bNetDirty = 1;
        if (GTeamsData.Defenders) GTeamsData.Defenders->bNetDirty = 1;
    }
    if (mode == "ticket" && p.contains("objectives")) ApplyObjectives(GRI, p["objectives"]);
    if (mode == "payload" && p.contains("objectives")) {
        ApplyLocks(GRI, p["objectives"]);
        ApplyObjectives(GRI, p["objectives"]);
        const int moved = EscortState::ApplyMatinee(GRI->m_MissionObjectives, p.value("matinee", nlohmann::json::array()));
        Logger::Log("restore", "[Restore] payload: %d matinee(s) seeked\n", moved);
    }
    if (mode == "breach" && p.contains("objectives")) {
        const int taken = ApplyCaptures(Game, GRI, p["objectives"]);
        // each replayed capture adds the map's capture time bonus; the
        // snapshot timer already contains the real ones
        if (taken > 0 && remaining > 1.0f) ApplyTimer(Game, remaining);
        ApplyLocks(GRI, p["objectives"]);
        ApplyObjectives(GRI, p["objectives"]);
        Logger::Log("restore", "[Restore] breach: %d capture(s) replayed\n", taken);
    }
    if (mode == "koth") ApplyRotation(Game, GRI);

    Logger::Log("restore", "[Restore] applied mode=%s timer=%.0f att=%d def=%d\n", mode.c_str(), remaining,
        GTeamsData.Attackers ? GTeamsData.Attackers->r_nCurrentPointCount : -1,
        GTeamsData.Defenders ? GTeamsData.Defenders->r_nCurrentPointCount : -1);
}

}  // namespace InstanceRestore
