#include "src/GameServer/TgGame/TgPlayerActions/Deploy/Deploy.hpp"

#include <cmath>

#include "src/GameServer/Globals.hpp"
#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/GameServer/TgGame/TgProj_Deployable/SpawnDeployable/TgProj_Deployable__SpawnDeployable.hpp"
#include "src/GameServer/TgGame/_deployable_classify/DeployableClassify.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace TgPlayerActions::DeployCmd {

namespace {

ATgPawn_Character* FindPawnBySessionGuid(const std::string& guid) {
    for (auto& kv : GClientConnectionsData) {
        if (kv.second.SessionGuid == guid) {
            return kv.second.Pawn;
        }
    }
    return nullptr;
}

const char* TeamName(Team t) {
    switch (t) {
        case Team::Friend: return "friend";
        case Team::Enemy:  return "enemy";
    }
    return "?";
}

constexpr float kPi = 3.14159265358979323846f;

// Same forward-of-player distance SpawnBot uses — far enough that the
// deployable's cylinder never overlaps the requesting player.
constexpr float kSpawnDistanceUU = 500.0f;

// Player CDO half-height (TgPawn UC default). Used to step from the
// pawn's cylinder-center Location.Z down to ground level so
// SpawnDeployableActor's vNormal-relative lift lands the deployable on the
// floor instead of mid-air.
constexpr float kPlayerCDOHalfHeight = 46.0f;

} // namespace

void Execute(const std::string& session_guid, int deployable_id, Team team) {
    ATgPawn_Character* Pawn = FindPawnBySessionGuid(session_guid);
    if (!Pawn) {
        Logger::Log("chat-command",
            "[ChatCmd][DLL] /deploy%s: no pawn for guid=%s; dropping\n",
            TeamName(team), session_guid.c_str());
        return;
    }

    // Resolve Friend/Enemy against the requesting player's task force, same
    // shape as SpawnBotCmd.
    ATgRepInfo_Player* PlayerRep = (ATgRepInfo_Player*)Pawn->PlayerReplicationInfo;
    ATgRepInfo_TaskForce* playerTf =
        PlayerRep ? (ATgRepInfo_TaskForce*)PlayerRep->Team : nullptr;
    if (!playerTf) {
        Logger::Log("chat-command",
            "[ChatCmd][DLL] /deploy%s guid=%s: player has no task force; dropping\n",
            TeamName(team), session_guid.c_str());
        return;
    }
    ATgRepInfo_TaskForce* targetTf;
    if (team == Team::Friend) {
        targetTf = playerTf;
    } else {
        targetTf = (playerTf == GTeamsData.Attackers)
                       ? GTeamsData.Defenders
                       : GTeamsData.Attackers;
    }

    // Aim direction = controller yaw when present (where the player is
    // actually looking), else pawn body yaw.
    const int yaw = Pawn->Controller ? Pawn->Controller->Rotation.Yaw
                                     : Pawn->Rotation.Yaw;
    const float yawRad = static_cast<float>(yaw) * (kPi / 32768.0f);

    // Placement contract for SpawnDeployableActor (see its header):
    //   - non-selfSpawn deployables: vLocation is the ground-contact point;
    //     the helper lifts by halfHeight along vNormal to land the cylinder
    //     center above ground.
    //   - selfSpawn (dome shield, deployable 22 today): vLocation is the
    //     pawn's cylinder-center Location; the helper drops by the pawn's
    //     collisionHeight so the dome envelops the player.
    const bool selfSpawn = DeployableClassify::DeploysOnSelf(deployable_id);

    FVector loc;
    if (selfSpawn) {
        loc = Pawn->Location;  // dome centers on the pawn
    } else {
        loc.X = Pawn->Location.X + std::cos(yawRad) * kSpawnDistanceUU;
        loc.Y = Pawn->Location.Y + std::sin(yawRad) * kSpawnDistanceUU;
        loc.Z = Pawn->Location.Z - kPlayerCDOHalfHeight;  // step to ground
    }

    FVector normal;
    normal.X = 0.f; normal.Y = 0.f; normal.Z = 1.f;  // flat ground

    Logger::Log("chat-command",
        "[ChatCmd][DLL] /deploy%s guid=%s deployable_id=%d selfSpawn=%d loc=(%.1f,%.1f,%.1f) yaw=%d\n",
        TeamName(team), session_guid.c_str(), deployable_id, (int)selfSpawn,
        loc.X, loc.Y, loc.Z, yaw);

    // sourceDevice/sourceFireMode left null — no equipped device drove this
    // spawn, so per-type limits and source-trace credit don't apply. The
    // helper handles the null case (skips r_Owner / s_SpawnerDeviceMode
    // writes).
    ATgDeployable* Deployable =
        TgProj_Deployable__SpawnDeployable::SpawnDeployableActor(
            Pawn, deployable_id, loc, normal,
            /*sourceDevice*/   nullptr,
            /*sourceFireMode*/ nullptr);

    if (!Deployable) {
        Logger::Log("chat-command",
            "[ChatCmd][DLL] /deploy%s guid=%s deployable_id=%d: SpawnDeployableActor returned null\n",
            TeamName(team), session_guid.c_str(), deployable_id);
        return;
    }

    // SpawnDeployableActor wired the team to the player (Friend). For Enemy,
    // rewrite the team fields the client's GetTaskForceFor reads:
    //   - r_TaskforceInfo + r_bOwnedByTaskforce → primary path
    //   - r_InstigatorInfo → fallback path (null = no PRI attribution; the
    //     client falls through to "no team" rather than back to the player)
    // SetTaskForceNumber pushes the basic team number through whatever
    // engine fields cache it.
    if (team == Team::Enemy) {
        Deployable->SetTaskForceNumber(targetTf->r_nTaskForce);
        if (Deployable->r_DRI) {
            ATgRepInfo_Deployable* dri = Deployable->r_DRI;
            dri->r_TaskforceInfo   = targetTf;
            dri->r_bOwnedByTaskforce = 1;
            dri->r_InstigatorInfo  = nullptr;
        }
        // Null Instigator so damage / heal / morale attribution doesn't
        // credit back to the requesting player — an "enemy" deployable's
        // effects should NOT be the player's responsibility.
        Deployable->Instigator = nullptr;
    }

    Logger::Log("chat-command",
        "[ChatCmd][DLL] /deploy%s guid=%s: deployable=%p (depId=%d) tf=%d hp=%d spawned\n",
        TeamName(team), session_guid.c_str(),
        (void*)Deployable, deployable_id,
        (int)targetTf->r_nTaskForce, Deployable->r_nHealth);
}

} // namespace TgPlayerActions::DeployCmd
