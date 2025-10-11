#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"

std::map <char*, UClass*> ClassPreloader::Classes;
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
				Classes[name] = (UClass*)obj;
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

