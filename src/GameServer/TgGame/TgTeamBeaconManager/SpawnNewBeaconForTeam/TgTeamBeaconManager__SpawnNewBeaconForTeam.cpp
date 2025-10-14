#include "src/GameServer/TgGame/TgTeamBeaconManager/SpawnNewBeaconForTeam/TgTeamBeaconManager__SpawnNewBeaconForTeam.hpp"
#include "src/Utils/Logger/Logger.hpp"


void __fastcall* TgTeamBeaconManager__SpawnNewBeaconForTeam::Call(ATgTeamBeaconManager* BeaconManager, void* edx) {
	// LogToFile("C:\\mylog.txt", "MINE TgTeamBeaconManager::SpawnNewBeaconForTeam START");
	// ATgTeamBeaconManager* BeaconManager = reinterpret_cast<ATgTeamBeaconManager*>(thisxx);

	Logger::Log("debug", "MINE TgTeamBeaconManager::SpawnNewBeaconForTeam START\n");

	for (int i=0; i<BeaconManager->s_BeaconFactoryList.Num(); i++) {
		Logger::Log("debug", "BeaconManager->s_BeaconFactoryList = %p\n", BeaconManager->s_BeaconFactoryList);
		Logger::Log("debug", "BeaconManager->s_BeaconFactoryList.Num() = %d\n", BeaconManager->s_BeaconFactoryList.Num());
		Logger::Log("debug", "BeaconManager->s_BeaconFactoryList.Data[i] = %p\n", BeaconManager->s_BeaconFactoryList.Data[i]);
		ATgBeaconFactory* beaconFactory = BeaconManager->s_BeaconFactoryList.Data[i];
		beaconFactory->s_nTaskForce = BeaconManager->r_TaskForce->r_nTaskForce;
		beaconFactory->SpawnObject();
	}

	BeaconManager->bNetInitial = 1;

	Logger::Log("debug", "MINE TgTeamBeaconManager::SpawnNewBeaconForTeam END\n");
	// LogToFile("C:\\mylog.txt", "MINE TgTeamBeaconManager::SpawnNewBeaconForTeam END");
}

