#include "src/GameServer/TgGame/TgBeaconFactory/SpawnObject/TgBeaconFactory__SpawnObject.hpp"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall* TgBeaconFactory__SpawnObject::Call(ATgBeaconFactory* BeaconFactory, void* edx) {
	Logger::Log("debug", "MINE TgBeaconFactory::SpawnObject START\n");
	// LogToFile("C:\\mylog.txt", "MINE TgBeaconFactory::SpawnObject START");
	// PreloadClasses();

	UClass* TgDeployBeaconClass = ClassPreloader::GetTgDeployBeaconClass();
	UClass* TgDeployBeaconEntranceClass = ClassPreloader::GetTgDeployBeaconEntranceClass();

	Logger::Log("debug", "Beacon class: %s\n", TgDeployBeaconClass->GetFullName());
	Logger::Log("debug", "Beacon entrance class: %s\n", TgDeployBeaconEntranceClass->GetFullName());


	if (BeaconFactory->m_bBeaconExit) {
		Logger::Log("debug", "trying to spawn beacon exit\n");

		FVector SpawnLocation = BeaconFactory->Location;
		// SpawnLocation.X += 2500;
		// SpawnLocation.X += 1500;

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

		Logger::Log("debug", "trying to init beacon replication info\n");
		Beacon->eventInitReplicationInfo();
		Logger::Log("debug", "beacon replication info inited, setting task force\n");

		Beacon->SetTaskForceNumber(BeaconFactory->s_nTaskForce);
		Logger::Log("debug", "beacon task force set\n");
		Beacon->Role = 3;

		Beacon->bNetInitial = 1;
	} else {
		Logger::Log("debug", "trying to spawn beacon entrance\n");
		ATgDeploy_BeaconEntrance* BeaconEntrance = (ATgDeploy_BeaconEntrance*)BeaconFactory->Spawn(
			ClassPreloader::GetTgDeployBeaconEntranceClass(),
			BeaconFactory,
			FName(),
			BeaconFactory->Location,
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

		BeaconEntrance->bNetInitial = 1;
	}
	// LogToFile("C:\\mylog.txt", "MINE TgTeamBeaconManager::SpawnNewBeaconForTeam END");

	Logger::Log("debug", "MINE TgBeaconFactory::SpawnObject END\n");
}

