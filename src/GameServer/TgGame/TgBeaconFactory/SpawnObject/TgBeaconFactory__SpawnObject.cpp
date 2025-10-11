#include "src/GameServer/TgGame/TgBeaconFactory/SpawnObject/TgBeaconFactory__SpawnObject.hpp"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"

void TgBeaconFactory__SpawnObject::Call(ATgBeaconFactory* BeaconFactory, void* edx) {
	// LogToFile("C:\\mylog.txt", "MINE TgBeaconFactory::SpawnObject START");
	// PreloadClasses();

	if (BeaconFactory->m_bBeaconExit) {
		ATgDeploy_Beacon* Beacon = (ATgDeploy_Beacon*)BeaconFactory->Spawn(
			ClassPreloader::GetTgDeployBeaconClass(),
			BeaconFactory,
			FName(),
			BeaconFactory->Location,
			BeaconFactory->Rotation,
			nullptr,
			1
		);

		if (GTeamsData.BeaconAttackers == nullptr) {
			GTeamsData.BeaconAttackers = Beacon;
			// LogToFile("C:\\mylog.txt", "Attackers beacon global set");
		} else if (GTeamsData.BeaconDefenders == nullptr) {
			GTeamsData.BeaconDefenders = Beacon;
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
		Beacon->eventInitReplicationInfo();

		Beacon->SetTaskForceNumber(BeaconFactory->s_nTaskForce);
		Beacon->Role = 3;
	} else {
		ATgDeploy_BeaconEntrance* BeaconEntrance = (ATgDeploy_BeaconEntrance*)BeaconFactory->Spawn(
			ClassPreloader::GetTgDeployBeaconEntranceClass(),
			BeaconFactory,
			FName(),
			BeaconFactory->Location,
			BeaconFactory->Rotation,
			nullptr,
			1
		);

		if (GTeamsData.BeaconEntranceAttackers == nullptr) {
			GTeamsData.BeaconEntranceAttackers = BeaconEntrance;
			// LogToFile("C:\\mylog.txt", "Attackers beacon entrance global set");
		} else if (GTeamsData.BeaconEntranceDefenders == nullptr) {
			GTeamsData.BeaconEntranceDefenders = BeaconEntrance;
			// LogToFile("C:\\mylog.txt", "Defenders beacon entrance global set");
		}

		BeaconEntrance->r_nDeployableId = 48;
		BeaconEntrance->m_bInDestroyedState = 0;
		BeaconEntrance->s_DeployFactory = BeaconFactory;
		BeaconEntrance->eventInitReplicationInfo();
		BeaconEntrance->SetTaskForceNumber(BeaconFactory->s_nTaskForce);
		BeaconEntrance->Role = 3;
	}
	// LogToFile("C:\\mylog.txt", "MINE TgTeamBeaconManager::SpawnNewBeaconForTeam END");

}

