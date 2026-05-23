#include "src/GameServer/TgGame/TgBeaconFactory/SpawnObject/TgBeaconFactory__SpawnObject.hpp"
#include "src/GameServer/TgGame/TgProj_Deployable/SpawnDeployable/TgProj_Deployable__SpawnDeployable.hpp"
#include "src/GameServer/TgGame/TgTeamBeaconManager/BeaconSdkSafe/BeaconSdkSafe.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Stripped native (TgBeaconFactory__SpawnObject_notimplemented @ 0x10a8c260).
// Callers in UC: TgActorFactory.PostBeginPlay (auto on dedicated server),
// TgBeaconFactory.OnToggle (kismet), TgTeamBeaconManager.SpawnNewBeaconForTeam
// (our reimpl). What it must do:
//
//   * Entrance factory (m_bBeaconExit=false): pre-spawn a TgDeploy_BeaconEntrance
//     at the factory location. Static — entrance pad in the team's spawn room.
//     No manager required; entrance.HasExit() queries the manager at runtime.
//
//   * Exit factory (m_bBeaconExit=true): spawn a TgDeploy_Beacon and register
//     it with the team's BeaconManager. Only when the manager exists — if
//     PostBeginPlay fires before TgRepInfo_TaskForce.PostInit creates the
//     manager, defer (skip). The manager's own InitFor -> CheckBeacon ->
//     SpawnNewBeaconForTeam path will call us back.

static ATgRepInfo_TaskForce* ResolveTaskForce(int taskForceNumber) {
	if (taskForceNumber <= 0) return nullptr;
	if (GTeamsData.Attackers && GTeamsData.Attackers->r_nTaskForce == taskForceNumber)
		return GTeamsData.Attackers;
	if (GTeamsData.Defenders && GTeamsData.Defenders->r_nTaskForce == taskForceNumber)
		return GTeamsData.Defenders;
	return nullptr;
}

static void WireDeployableOwnership(ATgDeployable* dep, ATgBeaconFactory* factory,
                                    ATgRepInfo_TaskForce* tf)
{
	dep->s_DeployFactory       = factory;
	dep->m_bInDestroyedState   = 0;
	dep->s_bIsActivated        = 1;
	dep->m_bIsDeployed         = 1;
	dep->bAlwaysRelevant       = 1;
	dep->bOnlyDirtyReplication = 1;
	dep->Role                  = 3;
	dep->RemoteRole            = 1;
	dep->bNetInitial           = 1;
	// dep->bNetDirty             = 1;
	// dep->bForceNetUpdate       = 1;

	dep->eventInitReplicationInfo();

	if (dep->r_DRI && tf) {
		dep->r_DRI->r_bOwnedByTaskforce = 1;
		dep->r_DRI->r_TaskforceInfo     = tf;
		dep->r_DRI->bNetDirty           = 1;
		// dep->r_DRI->bForceNetUpdate     = 1;
	}
	dep->r_bInitialIsEnemy = 0;
}

void __fastcall* TgBeaconFactory__SpawnObject::Call(ATgBeaconFactory* factory, void* /*edx*/) {
	if (!factory) return nullptr;

	// End-mission respawn gate. UC `AllPlayersEndGame` calls
	// `foreach DynamicActors(TgDeployable) Deploy.Destroy()` on every deployable
	// at game-over. Each destroyed beacon's `TgDeploy_Beacon.Destroyed` calls
	// the intact native `UnRegisterBeacon`, which internally fires
	// `PopulateBeaconFactoryList → Spawn(TgBeaconFactory) → factory.PostBeginPlay
	// → factory.SpawnObject` to materialize a replacement. UE3's
	// `foreach DynamicActors` iterates by index and picks up actors spawned
	// during iteration, so the new beacon is found and destroyed too — infinite
	// loop. The respawn pipeline is in a native we don't own, so we break the
	// cycle at the only node we DO own: this hook. Returning nullptr from
	// SpawnObject leaves the (empty, harmless) replacement factory but skips
	// the actual beacon creation, draining AllPlayersEndGame's foreach.
	ATgGame* game = (ATgGame*)Globals::Get().GGameInfo;
	if (game && game->bGameEnded) {
		Logger::Log("beacon",
			"SpawnObject suppressed — bGameEnded=1, end-mission cleanup in progress (factory=0x%p)\n",
			factory);
		return nullptr;
	}

	ATgRepInfo_TaskForce* tf = ResolveTaskForce((int)factory->s_nTaskForce);

	// Priority gate. TgGame.s_nCurrentPriority is the active tier; factories
	// tagged with a positive m_nPriority only spawn while their tier is
	// active. m_nPriority <= 0 means "untiered, always eligible" (matches
	// the TgFindPlayerStart convention). Skipping here applies to both the
	// PostBeginPlay auto-spawn path and the SpawnNewBeaconForTeam-driven
	// respawn path; AdjustBeaconForwardSpawn drives object lifecycle when
	// the active tier advances.
	const int currentPriority = game ? game->s_nCurrentPriority : 0;
	const bool priorityOk =
		factory->m_nPriority <= 0 || factory->m_nPriority == currentPriority;

	Logger::Log("beacon",
		"TgBeaconFactory::SpawnObject factory=0x%p mapObjId=%d tf=%d m_bBeaconExit=%d "
		"m_nPriority=%d currentPriority=%d resolvedTf=0x%p\n",
		factory, factory->m_nMapObjectId, (int)factory->s_nTaskForce,
		(int)factory->m_bBeaconExit, factory->m_nPriority, currentPriority, tf);

	if (tf && tf->r_BeaconManager) {
		BeaconSdk::PopulateBeaconFactoryList(tf->r_BeaconManager);
		Logger::Log("beacon",
			"  refreshed factory list on manager 0x%p (new count=%d)\n",
			tf->r_BeaconManager, tf->r_BeaconManager->s_BeaconFactoryList.Num());
	}

	if (!priorityOk) {
		Logger::Log("beacon",
			"  factory priority=%d != current=%d — skipping spawn\n",
			factory->m_nPriority, currentPriority);
		return nullptr;
	}

	// ENTRANCE
	if (!factory->m_bBeaconExit) {
		UClass* cls = ClassPreloader::GetTgDeployBeaconEntranceClass();
		if (!cls) {
			Logger::Log("beacon", "  entrance: class not preloaded, skipping\n");
			return nullptr;
		}

		FVector spawnLoc = factory->Location;
		float r = 0.f, halfH = 0.f;
		TgProj_Deployable__SpawnDeployable::GetDeployableCollisionCylinder(48, &r, &halfH);
		spawnLoc.Z += halfH + 5.0f;

		ATgDeploy_BeaconEntrance* entrance = (ATgDeploy_BeaconEntrance*)factory->Spawn(
			cls, factory, FName(), spawnLoc, factory->Rotation, nullptr, 1);
		if (!entrance) {
			Logger::Log("beacon", "  entrance: Spawn returned null\n");
			return nullptr;
		}

		entrance->r_nDeployableId = 48;
		WireDeployableOwnership(entrance, factory, tf);

		Logger::Log("beacon",
			"  entrance spawned 0x%p tf=%d at (%.0f,%.0f,%.0f)\n",
			entrance, (int)factory->s_nTaskForce,
			entrance->Location.X, entrance->Location.Y, entrance->Location.Z);
		return nullptr;
	}

	// EXIT
	ATgTeamBeaconManager* mgr = tf ? tf->r_BeaconManager : nullptr;
	if (!mgr) {
		Logger::Log("beacon",
			"  exit: no manager yet for tf %d — deferring\n", (int)factory->s_nTaskForce);
		return nullptr;
	}
	if (mgr->r_Beacon) {
		Logger::Log("beacon",
			"  exit: manager 0x%p already has beacon 0x%p — skipping factory 0x%p\n",
			mgr, mgr->r_Beacon, factory);
		return nullptr;
	}

	UClass* cls = ClassPreloader::GetTgDeployBeaconClass();
	if (!cls) {
		Logger::Log("beacon", "  exit: class not preloaded, skipping\n");
		return nullptr;
	}

	FVector spawnLoc = factory->Location;
	float r = 0.f, halfH = 0.f;
	TgProj_Deployable__SpawnDeployable::GetDeployableCollisionCylinder(36, &r, &halfH);
	spawnLoc.Z += halfH + 5.0f;

	ATgDeploy_Beacon* beacon = (ATgDeploy_Beacon*)factory->Spawn(
		cls, factory, FName(), spawnLoc, factory->Rotation, nullptr, 1);
	if (!beacon) {
		Logger::Log("beacon", "  exit: Spawn returned null\n");
		return nullptr;
	}

	beacon->r_nDeployableId   = 36;
	beacon->m_nPickupDeviceId = 1918;
	WireDeployableOwnership(beacon, factory, tf);

	// Pre-deployed (no Deploy animation). Marks m_bIsDeployed=1 (via
	// WireDeployableOwnership above) AND status=DEPLOYED so HasExit returns
	// true immediately at map load. RegisterBeacon native fills the DRI:
	// r_vLoc, r_bDeployed, r_TaskforceInfo. Use the bitfield-safe wrapper
	// — SDK auto-generated Parms hits the multi-bitfield packing bug
	// (see reference_sdk_bitfield_params_bug.md).
	BeaconSdk::RegisterBeacon(mgr, beacon, true);

	// Re-mark DRI dirty so the rep tick picks up RegisterBeacon's writes
	// (the native doesn't auto-set bNetDirty on the DRI). r_nName stays
	// empty for factory-spawned beacons — no player deployer at map load.
	if (mgr->r_BeaconInfo) {
		mgr->r_BeaconInfo->bNetDirty       = 1;
		mgr->r_BeaconInfo->bForceNetUpdate = 1;
	}

	Logger::Log("beacon",
		"  exit spawned 0x%p tf=%d at (%.0f,%.0f,%.0f) registered with manager 0x%p r_BeaconInfo=0x%p\n",
		beacon, (int)factory->s_nTaskForce,
		beacon->Location.X, beacon->Location.Y, beacon->Location.Z, mgr,
		mgr->r_BeaconInfo);
	return nullptr;
}
