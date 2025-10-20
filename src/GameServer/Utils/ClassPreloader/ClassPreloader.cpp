#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"

std::map <std::string, UClass*> ClassPreloader::Classes;
bool ClassPreloader::bClassesPreloaded = false;

void ClassPreloader::PreloadClasses() {
	if (bClassesPreloaded) {
		return;
	}

	for (int i = 0; i < UObject::GObjObjects()->Count; i++) {
		if (UObject::GObjObjects()->Data[i]) {
			UObject* obj = UObject::GObjObjects()->Data[i];
			char* name = obj->GetFullName();
			if (strncmp(name, "Class ", 6) == 0) {
				Classes[std::string(name)] = (UClass*)obj;
			}
		}
	}

	bClassesPreloaded = true;
}

UClass* ClassPreloader::GetTgPawnCharacterClass() {
	PreloadClasses();
	return Classes["Class TgGame.TgPawn_Character"];
}

UClass* ClassPreloader::GetTgPropertyClass() {
	PreloadClasses();
	return Classes["Class TgGame.TgProperty"];
}

UClass* ClassPreloader::GetTgRepInfoTaskForceClass() {
	PreloadClasses();
	return Classes["Class TgGame.TgRepInfo_TaskForce"];
}

UClass* ClassPreloader::GetTgDeployBeaconClass() {
	PreloadClasses();
	return Classes["Class TgGame.TgDeploy_Beacon"];
}

UClass* ClassPreloader::GetTgDeployBeaconEntranceClass() {
	PreloadClasses();
	return Classes["Class TgGame.TgDeploy_BeaconEntrance"];
}

UClass* ClassPreloader::GetTgRandomSMManagerClass() {
	PreloadClasses();
	return Classes["Class TgGame.TgRandomSMManager"];
}

UClass* ClassPreloader::GetTgPawnEliteAlchemistClass() {
	PreloadClasses();
	return Classes["Class TgGame.TgPawn_Elite_Alchemist"];
}

UClass* ClassPreloader::GetTgAIControllerClass() {
	PreloadClasses();
	return Classes["Class TgGame.TgAIController"];
}

UClass* ClassPreloader::GetTgPawnThinkTankClass() {
	PreloadClasses();
	return Classes["Class TgGame.TgPawn_ThinkTank"];
}

UClass* ClassPreloader::GetTgPawnGuardianClass() {
	PreloadClasses();
	return Classes["Class TgGame.TgPawn_Guardian"];
}

UClass* ClassPreloader::GetTgDeviceClass() {
	PreloadClasses();
	return Classes["Class TgGame.TgDevice"];
}

UClass* ClassPreloader::GetTgHudTeamGameClass() {
	PreloadClasses();
	return Classes["Class TgClient.TgHUD_TeamGame"];
}

UClass* ClassPreloader::GetTgSeqEventMissionTimerClass() {
	PreloadClasses();
	return Classes["Class TgGame.TgSeqEvent_MissionTimer"];
}

UClass* ClassPreloader::GetTgSeqEventLevelFadedInClass() {
	PreloadClasses();
	return Classes["Class TgGame.TgSeqEvent_LevelFadedIn"];
}

UClass* ClassPreloader::GetSeqEventLevelLoadedClass() {
	PreloadClasses();
	return Classes["Class Engine.SeqEvent_LevelLoaded"];
}



