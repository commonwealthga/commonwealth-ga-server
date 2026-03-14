#include "src/GameServer/TgGame/TgProj_Deployable/SpawnDeployable/TgProj_Deployable__SpawnDeployable.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include "src/Utils/Macros.hpp"

// Assembly data lookup function (from Ghidra / CreateEquipDevice disasm)
static constexpr uintptr_t FN_LOOKUP_DEPLOYABLE_CFG = 0x10942010; // FUN_10942010(mgr, id) → CAmDeployable*

// Shared spawn helper: spawn + initialise a deployable of the right class.
// Called from both SpawnDeployable (projectile path) and TgDeviceFire::Deploy (custom-fire path).
ATgDeployable* TgProj_Deployable__SpawnDeployable::SpawnDeployableActor(
    ATgPawn* pawn, int deployableId, FVector vLocation, FVector vNormal)
{
    if (!pawn) return nullptr;

    // Look up deployable config from assembly data to determine the UClass to spawn.
    using LookupFn = void*(__fastcall*)(void*, void*, int);
    void* cfg = ((LookupFn)FN_LOOKUP_DEPLOYABLE_CFG)(Globals::Get().GAssemblyDatManager, nullptr, deployableId);

    // cfg+0x10: pointer to wchar class name (0 → default TgDeployable, non-zero → specialised)
    bool bIsBeacon = (cfg && *(int*)((char*)cfg + 0x10) != 0);
    UClass* cls = nullptr;
    if (bIsBeacon) {
        // Specialised deployable class. We currently only support TgDeploy_Beacon here;
        // the assembly class name string (wide) at cfg+0x10 would be "TgGame.TgDeploy_Beacon".
        cls = ClassPreloader::GetTgDeployBeaconClass();
        // The aim trace returns the floor contact point; actors spawn with origin at that point.
        // CollisionHeight=10 (from TgDeployable defaults) → add that so the cylinder bottom
        // sits on the surface rather than half-underground.
        vLocation.Z += 5.0f;
        Logger::Log(GetLogChannel(), "SpawnDeployableActor: specialised class → TgDeploy_Beacon (deployableId=%d)\n", deployableId);
    } else {
        cls = ClassPreloader::GetTgDeployableClass();
        Logger::Log(GetLogChannel(), "SpawnDeployableActor: base TgDeployable class (deployableId=%d)\n", deployableId);
    }

    if (!cls) {
        Logger::Log(GetLogChannel(), "SpawnDeployableActor: NULL class, aborting\n");
        return nullptr;
    }

    ATgDeployable* Deployable = (ATgDeployable*)pawn->Spawn(
        cls,
        pawn,
        FName(),
        vLocation,
        pawn->Rotation,
        nullptr,
        1
    );

    if (!Deployable) {
        Logger::Log(GetLogChannel(), "SpawnDeployableActor: Spawn returned null\n");
        return nullptr;
    }

    ATgRepInfo_Player* pawnrep = (ATgRepInfo_Player*)pawn->PlayerReplicationInfo;

    Deployable->eventInitReplicationInfo();
    if (pawnrep && pawnrep->r_TaskForce) {
        Deployable->SetTaskForceNumber(pawnrep->r_TaskForce->r_nTaskForce);
    }

    TARRAY_INIT(pawn, s_SelfDeployableList, ATgDeployable*, 0x1514, 255);
    TARRAY_ADD(s_SelfDeployableList, Deployable);

    Deployable->r_nDeployableId    = deployableId;
    Deployable->r_nHealth          = 100;
    Deployable->m_bInDestroyedState = 0;
    Deployable->r_bTakeDamage      = 1;
    Deployable->s_bIsActivated     = 1;
    Deployable->m_bIsDeployed      = 0;
    Deployable->bOnlyDirtyReplication = 0;
    Deployable->Role               = 3;
    Deployable->RemoteRole         = 1;
    Deployable->bNetInitial        = 1;
    Deployable->bNetDirty          = 1;
    Deployable->bForceNetUpdate    = 1;
    Deployable->bAlwaysRelevant    = 1;

    Logger::Log(GetLogChannel(), "SpawnDeployableActor: spawned 0x%p deployableId=%d taskForce=%d\n",
        Deployable, deployableId,
        (pawnrep && pawnrep->r_TaskForce) ? pawnrep->r_TaskForce->r_nTaskForce : -1);

    // For specialised deployables (beacons), register with the TgTeamBeaconManager so that:
    //   - GetBeacon() returns non-null → BeaconEntrance::HasExit() returns true → teleport fires
    //   - r_BeaconStatus replicates to clients for UI (beacon deployed indicator)
    if (bIsBeacon) {
        ATgTeamBeaconManager* beaconMgr = pawnrep ? pawnrep->r_TaskForce->r_BeaconManager : nullptr;
        if (beaconMgr) {
            // Invalidate any previously deployed beacon so the old pointer doesn't linger.
            if (beaconMgr->r_Beacon && beaconMgr->r_Beacon != (ATgDeploy_Beacon*)Deployable) {
                beaconMgr->r_Beacon->m_bInDestroyedState = 1;
            }

            // Set s_DeployFactory before calling RegisterBeacon so CheckBeacon passes inside it.
            // s_BeaconFactoryList is populated at PostBeginPlay; no need to repopulate here.
            if (beaconMgr->s_BeaconFactoryList.Num() > 0) {
                Deployable->s_DeployFactory = (ATgActorFactory*)beaconMgr->s_BeaconFactoryList.Data[0];
            } else {
                Logger::Log(GetLogChannel(), "SpawnDeployableActor: WARNING no TgBeaconFactory in level\n");
            }

            Deployable->m_bInDestroyedState = 0;
            beaconMgr->r_Beacon       = (ATgDeploy_Beacon*)Deployable;
            beaconMgr->r_BeaconHolder = nullptr;
            beaconMgr->bNetDirty      = 1;
            beaconMgr->bForceNetUpdate = 1;

            // RegisterBeacon drives CheckBeacon which sets r_BeaconStatus to DEPLOYED(3),
            // FORWARD_SPAWN(4), or AT_SPAWN(5) — mirrors the initial beacon setup in HandlePlayerConnected.
            beaconMgr->RegisterBeacon((ATgDeploy_Beacon*)Deployable, 1);

            // eventInitReplicationInfo() was called before s_DeployFactory was set, so it took
            // the AddMinion path and left r_DRI.r_TaskforceInfo = null.  Fix it now so that
            // IsFriendlyWithLocalPawn() → NotifyGroupChanged() → RecalculateMaterial() can work.
            if (Deployable->r_DRI) {
                Deployable->r_DRI->SetTaskForce(pawnrep->r_TaskForce);
                Deployable->r_DRI->bNetDirty      = 1;
                Deployable->r_DRI->bForceNetUpdate = 1;
            }
            // Flip r_bInitialIsEnemy (was 0) so the client's ReplicatedEvent fires and
            // calls NotifyGroupChanged() after r_TaskforceInfo has replicated.
            Deployable->r_bInitialIsEnemy  = 1;
            Deployable->bNetDirty          = 1;
            Deployable->bForceNetUpdate    = 1;

            Logger::Log(GetLogChannel(), "SpawnDeployableActor: registered beacon 0x%p with BeaconManager 0x%p, factory=0x%p, status=%d\n",
                Deployable, beaconMgr, Deployable->s_DeployFactory, (int)beaconMgr->r_BeaconStatus);
        } else {
            Logger::Log(GetLogChannel(), "SpawnDeployableActor: WARNING no BeaconManager for taskForce\n");
        }
    }

    return Deployable;
}

// Called by TgProj_Deployable::SpawnTheDeployable when a deployable projectile lands.
ATgDeployable* __fastcall TgProj_Deployable__SpawnDeployable::Call(
    ATgProj_Deployable* pThis, void* edx,
    FVector vLocation, AActor* TargetActor, FVector vNormal)
{
    if (!pThis) return nullptr;

    ATgPawn* pawn = (ATgPawn*)pThis->Instigator;
    if (!pawn) {
        Logger::Log(GetLogChannel(), "TgProj_Deployable::SpawnDeployable: null Instigator\n");
        return nullptr;
    }

    // Get deployable ID from the fire mode that spawned this projectile.
    UTgDeviceFire* fireMode = pThis->s_LastDefaultMode;
    if (!fireMode || !fireMode->m_pFireModeSetup.Dummy) {
        Logger::Log(GetLogChannel(), "TgProj_Deployable::SpawnDeployable: no fire mode setup\n");
        return nullptr;
    }
    int deployableId = *(int*)((char*)fireMode->m_pFireModeSetup.Dummy + 0x28);

    Logger::Log(GetLogChannel(), "TgProj_Deployable::SpawnDeployable: pawn=%s deployableId=%d loc=(%.0f,%.0f,%.0f)\n",
        pawn->GetFullName(), deployableId, vLocation.X, vLocation.Y, vLocation.Z);

    return SpawnDeployableActor(pawn, deployableId, vLocation, vNormal);
}
