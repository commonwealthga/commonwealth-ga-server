#include "src/GameServer/TgGame/TgPlayerActions/ChangeTeam/ChangeTeam.hpp"

#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/GameServer/Stats/MatchStats.hpp"
#include "src/IpcClient/IpcClient.hpp"
#include "src/GameServer/TgGame/TgPawn/SetTaskForceNumber/TgPawn__SetTaskForceNumber.hpp"
#include "src/GameServer/TgGame/TgPawn/KillDeployables/TgPawn__KillDeployables.hpp"
#include "src/GameServer/TgGame/TgTeamBeaconManager/BeaconSdkSafe/BeaconSdkSafe.hpp"
#include "src/GameServer/Combat/MissionAlerts/SendAlert.hpp"
#include "src/GameServer/Utils/ActorCache/ActorCache.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace TgPlayerActions::ChangeTeamCmd {

namespace {

// Walk GClientConnectionsData looking for a matching SessionGuid.
// Returns the connected pawn or nullptr.
ATgPawn_Character* FindPawnBySessionGuid(const std::string& guid) {
    for (auto& kv : GClientConnectionsData) {
        if (kv.second.SessionGuid == guid) {
            return kv.second.Pawn;
        }
    }
    return nullptr;
}

const char* TargetName(Target t) {
    switch (t) {
        case Target::Toggle:    return "toggle";
        case Target::Attackers: return "attackers";
        case Target::Defenders: return "defenders";
    }
    return "?";
}

void Audit(const std::string& guid,
           const std::string& outcome, const std::string& details) {
    IpcClient::SendChatCommandAudit(guid, "-changeteam", outcome, details);
}

// Determine the current team number (1 = Attackers, 2 = Defenders).
// Reads the pawn's PlayerReplicationInfo->r_TaskForce and compares against
// the global team rep-info pointers in GTeamsData.
//
// Returns 0 if it cannot determine the team (PRI null, r_TaskForce null,
// GTeamsData unpopulated).
int ResolveCurrentTeamNumber(ATgPawn_Character* Pawn) {
    if (!Pawn || !Pawn->PlayerReplicationInfo) return 0;
    // Same cast pattern as TgPawn__SetTaskForceNumber.cpp:6 — the engine
    // PlayerReplicationInfo* on a TgPawn is always an ATgRepInfo_Player.
    ATgRepInfo_Player* repinfo = reinterpret_cast<ATgRepInfo_Player*>(Pawn->PlayerReplicationInfo);
    auto* tf = repinfo->r_TaskForce;
    if (!tf) return 0;
    if (tf == GTeamsData.Attackers) return 1;
    if (tf == GTeamsData.Defenders) return 2;
    return 0;
}

// Move the (living) pawn to its team's player start. FindPlayerStart reads the
// controller PRI's r_TaskForce, so this MUST run AFTER the team flip to land on
// the new side. Mirrors TgTeleporter.UsePlayerStart / ReturnHomeArea: a
// game-thread relocate, not a respawn.
void TeleportToTeamStart(const std::string& session_guid, ATgPawn_Character* pawn) {
    if (!pawn->Controller || !pawn->WorldInfo || !pawn->WorldInfo->Game) {
        Logger::Log("chat-command",
            "[ChatCmd][DLL] /changeteam guid=%s: missing controller/world/game; skipping teleport\n",
            session_guid.c_str());
        return;
    }

    ATgGame* game = reinterpret_cast<ATgGame*>(pawn->WorldInfo->Game);
    FString incomingName;
    ANavigationPoint* start = game->FindPlayerStart(pawn->Controller, 0, incomingName);
    if (!start) {
        Logger::Log("chat-command",
            "[ChatCmd][DLL] /changeteam guid=%s: FindPlayerStart returned null\n",
            session_guid.c_str());
        return;
    }

    FRotator newRot = pawn->Rotation;
    newRot.Pitch = 0;
    newRot.Yaw = start->Rotation.Yaw;
    newRot.Roll = 0;

    if (!pawn->SetLocation(start->Location)) {
        Logger::Log("chat-command",
            "[ChatCmd][DLL] /changeteam guid=%s: SetLocation failed loc=(%.0f,%.0f,%.0f)\n",
            session_guid.c_str(), start->Location.X, start->Location.Y, start->Location.Z);
        return;
    }

    pawn->Velocity = FVector(0, 0, 0);
    pawn->SetRotation(newRot);
    pawn->SetViewRotation(newRot);
    pawn->ClientSetRotation(newRot);
    pawn->Controller->ClientSetRotation(newRot, 1);
    pawn->Controller->MoveTimer = -1.0f;
    pawn->SetAnchor(start);
    pawn->SetMoveTarget(start);
    pawn->PlayTeleportEffect(false, true);

    // SetLocation doesn't fire volume Touched events — re-resolve volume-driven
    // pawn properties at the destination (mirrors ReturnHomeArea).
    ActorCache::CacheMapActors();
    pawn->eventModifyPawnPropertiesVolumeChanged();
    pawn->eventOmegaVolumePropertiesChanged();
    pawn->bNetDirty = 1;
    pawn->bForceNetUpdate = 1;

    Logger::Log("chat-command",
        "[ChatCmd][DLL] /changeteam guid=%s: teleported to %s loc=(%.0f,%.0f,%.0f) yaw=%d\n",
        session_guid.c_str(), ((UObject*)start)->GetName(),
        start->Location.X, start->Location.Y, start->Location.Z, newRot.Yaw);
}

// Free-text center-screen toast telling the player they were auto-rebalanced.
// PlayerControllers only — bots / AI have no client connection.
void SendAutobalanceAlert(ATgPawn_Character* pawn) {
    if (!pawn->Controller) return;
    const char* raw = pawn->Controller->Class ? pawn->Controller->Class->GetFullName() : nullptr;
    const std::string ctrlClass(raw ? raw : "");
    if (ctrlClass.find("PlayerController") == std::string::npos) return;

    APlayerController* PC = (APlayerController*)pawn->Controller;
    if (!PC->Player) return;
    UNetConnection* Connection = (UNetConnection*)PC->Player;

    // priority 2 = High, type 3 = Important (TgObject.AlertPriority / AlertType).
    SendAlert::SendText(Connection,
        L"You have been moved to the other team to balance the match.",
        2, 3, 4.0f);
}

} // namespace

void Execute(const std::string& session_guid, Target target, bool is_autobalance) {
    ATgPawn_Character* Pawn = FindPawnBySessionGuid(session_guid);
    if (!Pawn) {
        Logger::Log("chat-command",
            "[ChatCmd][DLL] /changeteam: no pawn for guid=%s; dropping\n",
            session_guid.c_str());
        Audit(session_guid, "ignored", "no player pawn");
        return;
    }

    int current_team = ResolveCurrentTeamNumber(Pawn);
    if (current_team == 0) {
        Logger::Log("chat-command",
            "[ChatCmd][DLL] /changeteam guid=%s: cannot resolve current team; dropping\n",
            session_guid.c_str());
        Audit(session_guid, "ignored", "cannot resolve current team");
        return;
    }

    int new_team = 0;
    switch (target) {
        case Target::Toggle:    new_team = (current_team == 1) ? 2 : 1; break;
        case Target::Attackers: new_team = 1; break;
        case Target::Defenders: new_team = 2; break;
    }

    if (new_team == current_team) {
        Logger::Log("chat-command",
            "[ChatCmd][DLL] /changeteam guid=%s target=%s: already on team %d; no-op\n",
            session_guid.c_str(), TargetName(target), current_team);
        Audit(session_guid, "no-op",
            std::string("already on team ") + std::to_string(current_team)
            + " target=" + TargetName(target));
        return;
    }

    Logger::Log("chat-command",
        "[ChatCmd][DLL] /changeteam guid=%s target=%s autobalance=%d: flipping %d -> %d\n",
        session_guid.c_str(), TargetName(target), (int)is_autobalance, current_team, new_team);

    // Bank the old-team stint + emit TEAM_CHANGE before the flip.
    MatchStats::OnTeamChanged((ATgPawn*)Pawn, new_team);

    // Beacon handoff BEFORE the flip: if the player carries the team beacon,
    // strip it and respawn for the OLD team; if they deployed it, sever their
    // personal ownership. Reads the still-current (old) team, so order matters
    // — same reason the player teardown ran before the team change on death.
    BeaconSdk::ReleaseBeaconForTeamChange((ATgPawn*)Pawn);

    // Destroy all personal deployables before the team flip so they don't
    // linger as orphaned objects belonging to the old team.
    TgPawn__KillDeployables::KillAllOwned((ATgPawn*)Pawn);

    // Flip the team. SetTaskForceNumber writes both r_TaskForce and Team on the
    // PRI, calls repinfo->SetTeam(taskforce), and fires Pawn->NotifyTeamChanged.
    // 1 = Attackers, anything else = Defenders (per
    // TgPawn__SetTaskForceNumber.cpp:5-18).
    TgPawn__SetTaskForceNumber::Call(Pawn, nullptr, new_team);

    // No death, no respawn timer: teleport the living pawn to the new team's
    // player start (FindPlayerStart reads the now-flipped PRI team).
    TeleportToTeamStart(session_guid, Pawn);

    // Auto-rebalance only: tell the player they were moved.
    if (is_autobalance) {
        SendAutobalanceAlert(Pawn);
    }

    Logger::Log("chat-command",
        "[ChatCmd][DLL] /changeteam guid=%s: SetTaskForceNumber + teleport dispatched\n",
        session_guid.c_str());
    Audit(session_guid, "activated",
        "changed team " + std::to_string(current_team) + "->" + std::to_string(new_team));
}

} // namespace TgPlayerActions::ChangeTeamCmd
