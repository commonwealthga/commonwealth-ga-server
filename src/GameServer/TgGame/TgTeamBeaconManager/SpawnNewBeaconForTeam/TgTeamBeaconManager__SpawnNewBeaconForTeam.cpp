#include "src/GameServer/TgGame/TgTeamBeaconManager/SpawnNewBeaconForTeam/TgTeamBeaconManager__SpawnNewBeaconForTeam.hpp"

void TgTeamBeaconManager__SpawnNewBeaconForTeam::Call(ATgTeamBeaconManager* BeaconManager, void* edx) {
	// LogToFile("C:\\mylog.txt", "MINE TgTeamBeaconManager::SpawnNewBeaconForTeam START");
	// ATgTeamBeaconManager* BeaconManager = reinterpret_cast<ATgTeamBeaconManager*>(thisxx);

	for (int i=0; i<BeaconManager->s_BeaconFactoryList.Num(); i++) {
		ATgBeaconFactory* beaconFactory = BeaconManager->s_BeaconFactoryList.Data[i];
		beaconFactory->s_nTaskForce = BeaconManager->r_TaskForce->r_nTaskForce;
		beaconFactory->SpawnObject();
	}

	// LogToFile("C:\\mylog.txt", "MINE TgTeamBeaconManager::SpawnNewBeaconForTeam END");
}

