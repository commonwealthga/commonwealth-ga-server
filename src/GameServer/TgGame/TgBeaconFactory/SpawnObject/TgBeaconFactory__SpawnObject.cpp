#include "src/GameServer/TgGame/TgBeaconFactory/SpawnObject/TgBeaconFactory__SpawnObject.hpp"
#include "src/GameServer/TgGame/TgProj_Deployable/SpawnDeployable/TgProj_Deployable__SpawnDeployable.hpp"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include <vector>

bool TgBeaconFactory__SpawnObject::s_firstPlayerConnected = false;

// Stashes BeaconFactory pointers whose SpawnObject fired before any client
// connected.  Replayed from HandlePlayerConnected on first connect so the
// beacon/DRI replication runs with a populated client NetGUID table.
static std::vector<ATgBeaconFactory*> s_pendingFactories;

void TgBeaconFactory__SpawnObject::FlushPendingSpawns() {
	s_firstPlayerConnected = true;
	Logger::Log("debug", "[BeaconFactory] flushing %zu deferred spawns\n", s_pendingFactories.size());
	for (ATgBeaconFactory* bf : s_pendingFactories) {
		RunSpawn(bf);
	}
	s_pendingFactories.clear();
}

void __fastcall* TgBeaconFactory__SpawnObject::Call(ATgBeaconFactory* BeaconFactory, void* edx) {
	if (!s_firstPlayerConnected) {
		Logger::Log("debug", "[BeaconFactory] deferring spawn for 0x%p until first player connects\n", BeaconFactory);
		s_pendingFactories.push_back(BeaconFactory);
		return nullptr;
	}
	RunSpawn(BeaconFactory);
	return nullptr;
}

void TgBeaconFactory__SpawnObject::RunSpawn(ATgBeaconFactory* BeaconFactory) {
	Logger::Log("debug", "MINE TgBeaconFactory::SpawnObject START\n");
	// LogToFile("C:\\mylog.txt", "MINE TgBeaconFactory::SpawnObject START");
	// PreloadClasses();

	UClass* TgDeployBeaconClass = ClassPreloader::GetTgDeployBeaconClass();
	UClass* TgDeployBeaconEntranceClass = ClassPreloader::GetTgDeployBeaconEntranceClass();

	Logger::Log("debug", "Beacon class: %s\n", TgDeployBeaconClass->GetFullName());
	Logger::Log("debug", "Beacon entrance class: %s\n", TgDeployBeaconEntranceClass->GetFullName());


	if (BeaconFactory->m_bBeaconExit) {
		Logger::Log("debug", "trying to spawn beacon exit\n");

		// BeaconFactory->Location is at ground level; actor Location is the
		// cylinder center, so lift by the beacon's halfHeight plus the 5uu
		// floor-buffer used by the game's placement trace (_DAT_1197edd8).
		// Mirrors the lift inside SpawnDeployableActor.  Beacon = deployable_id 36.
		FVector SpawnLocation = BeaconFactory->Location;
		{
			float r = 0.f, halfH = 0.f;
			TgProj_Deployable__SpawnDeployable::GetDeployableCollisionCylinder(36, &r, &halfH);
			SpawnLocation.Z += halfH + 5.0f;
		}

		ATgDeploy_Beacon* Beacon = (ATgDeploy_Beacon*)BeaconFactory->Spawn(
			ClassPreloader::GetTgDeployBeaconClass(),
			BeaconFactory,
			FName(),
			SpawnLocation,
			BeaconFactory->Rotation,
			nullptr,
			1
		);
		if (Beacon == nullptr) {
			Logger::Log("debug", "spawning beacon exit failed\n");
		} else {
			Logger::Log("debug", "beacon exit spawned\n");
		}


		if (GTeamsData.BeaconAttackers == nullptr) {
			GTeamsData.BeaconAttackers = Beacon;
			Logger::Log("debug", "attackers beacon global set\n");
			// LogToFile("C:\\mylog.txt", "Attackers beacon global set");
		} else if (GTeamsData.BeaconDefenders == nullptr) {
			GTeamsData.BeaconDefenders = Beacon;
			Logger::Log("debug", "defenders beacon global set\n");
			// LogToFile("C:\\mylog.txt", "Defenders beacon global set");
		}

		Beacon->r_nDeployableId = 36;
		Beacon->m_bInDestroyedState = 0;
		Beacon->m_nPickupDeviceId = 1918;
		Beacon->s_DeployFactory = BeaconFactory;
		Beacon->s_bWasPickedUp = 0;
		Beacon->m_bPickupOnlyOnce = 1;
		Beacon->m_bPickupConsumeOnUse = 1;
		Beacon->m_bPickupConsumeOnDeath = 1;
		Beacon->r_bTakeDamage = 1;
		Beacon->s_bIsActivated = 1;
		Beacon->m_bIsDeployed = 1;
		Beacon->bOnlyDirtyReplication = 1;
		Beacon->bAlwaysRelevant = 1;

		Logger::Log("debug", "trying to init beacon replication info\n");
		Beacon->eventInitReplicationInfo();
		Logger::Log("debug", "beacon replication info inited, setting task force\n");

		Beacon->SetTaskForceNumber(BeaconFactory->s_nTaskForce);
		Logger::Log("debug", "beacon task force set\n");
		Beacon->Role = 3;

		// eventInitReplicationInfo calls GRI.GetTaskForce() which may return null during early
		// init (before GTeamsData.Attackers/Defenders are populated).  Explicitly set the correct
		// team so r_DRI.r_TaskforceInfo is non-null when the initial replication packet fires.
		{
			ATgRepInfo_TaskForce* tf = (GTeamsData.BeaconAttackers == Beacon) ? GTeamsData.Attackers : GTeamsData.Defenders;
			if (Beacon->r_DRI && tf) {
				// Write r_TaskforceInfo directly — TgRepInfo_Deployable::SetTaskForce
				// (native 0x109ee560) would zero r_InstigatorInfo, killing the
				// client's GetTaskForceFor fallback path. See
				// TgProj_Deployable__SpawnDeployable.cpp for the full rationale.
				Beacon->r_DRI->r_bOwnedByTaskforce = 1;
				Beacon->r_DRI->r_TaskforceInfo   = tf;
				Beacon->r_DRI->bNetDirty       = 1;
				Beacon->r_DRI->bForceNetUpdate = 1;
				// DRI rep-order fix: lower priority below the beacon's so the
				// beacon's NetGUID is cached on the client before the DRI's
				// r_DeployableOwner is serialized.  See the big comment in
				// SpawnDeployableActor for the full chain of cause-and-effect.
				Beacon->r_DRI->NetPriority = 1.0f;
				Logger::Log("debug", "beacon r_DRI taskforce set directly to 0x%p (r_bOwnedByTaskforce=1)\n", tf);
			}
		}

		// r_bInitialIsEnemy is a repnotify trigger: client fires NotifyGroupChanged() →
		// IsFriendlyWithLocalPawn() → RecalculateMaterial().  Must be set before bNetInitial
		// fires so the callback runs with correct r_DRI data.
		Beacon->r_bInitialIsEnemy = 0;

		Beacon->bNetInitial = 1;
	} else {
		Logger::Log("debug", "trying to spawn beacon entrance\n");

		// Same ground-lift as the beacon branch above.  BeaconEntrance =
		// deployable_id 48 (DVC_MDL_Teleport_Beacon_Static_Deployed).
		FVector EntranceLocation = BeaconFactory->Location;
		{
			float r = 0.f, halfH = 0.f;
			TgProj_Deployable__SpawnDeployable::GetDeployableCollisionCylinder(48, &r, &halfH);
			EntranceLocation.Z += halfH + 5.0f;
		}

		ATgDeploy_BeaconEntrance* BeaconEntrance = (ATgDeploy_BeaconEntrance*)BeaconFactory->Spawn(
			ClassPreloader::GetTgDeployBeaconEntranceClass(),
			BeaconFactory,
			FName(),
			EntranceLocation,
			BeaconFactory->Rotation,
			nullptr,
			1
		);
		if (BeaconEntrance == nullptr) {
			Logger::Log("debug", "spawning beacon entrance failed\n");
		} else {
			Logger::Log("debug", "beacon entrance spawned\n");
		}

		if (GTeamsData.BeaconEntranceAttackers == nullptr) {
			GTeamsData.BeaconEntranceAttackers = BeaconEntrance;
			Logger::Log("debug", "attackers beacon entrance global set\n");
			// LogToFile("C:\\mylog.txt", "Attackers beacon entrance global set");
		} else if (GTeamsData.BeaconEntranceDefenders == nullptr) {
			GTeamsData.BeaconEntranceDefenders = BeaconEntrance;
			Logger::Log("debug", "defenders beacon entrance global set\n");
			// LogToFile("C:\\mylog.txt", "Defenders beacon entrance global set");
		}

		BeaconEntrance->r_nDeployableId = 48;
		BeaconEntrance->m_bInDestroyedState = 0;
		BeaconEntrance->s_DeployFactory = BeaconFactory;
		Logger::Log("debug", "trying to init beacon replication info\n");
		BeaconEntrance->eventInitReplicationInfo();
		Logger::Log("debug", "beacon replication info inited, setting task force\n");
		BeaconEntrance->SetTaskForceNumber(BeaconFactory->s_nTaskForce);
		Logger::Log("debug", "beacon task force set\n");
		BeaconEntrance->Role = 3;
		BeaconEntrance->bOnlyDirtyReplication = 1;
		// Needs to be always-relevant so the DRI reaches every client.
		// Without this, enemy-team viewers never receive the entrance's DRI
		// and fall back to the global `!r_bInitialIsEnemy` branch, showing
		// the entrance in their own colors regardless of actual ownership.
		BeaconEntrance->bAlwaysRelevant = 1;

		// Same explicit team fix as the beacon above.
		{
			ATgRepInfo_TaskForce* tf = (GTeamsData.BeaconEntranceAttackers == BeaconEntrance) ? GTeamsData.Attackers : GTeamsData.Defenders;
			if (BeaconEntrance->r_DRI && tf) {
				// Same direct-write pattern as the beacon block above —
				// avoid SetTaskForce so we don't zero r_InstigatorInfo.
				BeaconEntrance->r_DRI->r_bOwnedByTaskforce = 1;
				BeaconEntrance->r_DRI->r_TaskforceInfo   = tf;
				BeaconEntrance->r_DRI->bNetDirty       = 1;
				BeaconEntrance->r_DRI->bForceNetUpdate = 1;
				// Same rep-order fix as the beacon block.
				BeaconEntrance->r_DRI->NetPriority = 1.0f;
				Logger::Log("debug", "entrance r_DRI taskforce set directly to 0x%p (r_bOwnedByTaskforce=1)\n", tf);
			}
		}

		BeaconEntrance->r_bInitialIsEnemy = 0;

		BeaconEntrance->bNetInitial = 1;
	}
	// LogToFile("C:\\mylog.txt", "MINE TgTeamBeaconManager::SpawnNewBeaconForTeam END");

	Logger::Log("debug", "MINE TgBeaconFactory::SpawnObject END\n");
}
