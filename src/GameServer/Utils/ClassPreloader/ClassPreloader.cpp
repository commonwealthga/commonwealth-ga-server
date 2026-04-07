#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/Utils/ObjectCache/ObjectCache.hpp"

UClass* ClassPreloader::GetClass(const char* ClassName) {
	return (UClass*)ObjectCache::Find(ClassName);
}

UObject* ClassPreloader::GetObject(const char* FullName) {
	return ObjectCache::Find(FullName);
}

UClass* ClassPreloader::GetTgPawnCharacterClass() {
	return GetClass("Class TgGame.TgPawn_Character");
}

UClass* ClassPreloader::GetTgPropertyClass() {
	return GetClass("Class TgGame.TgProperty");
}

UClass* ClassPreloader::GetTgRepInfoTaskForceClass() {
	return GetClass("Class TgGame.TgRepInfo_TaskForce");
}

UClass* ClassPreloader::GetTgDeployableClass() {
	return GetClass("Class TgGame.TgDeployable");
}

UClass* ClassPreloader::GetTgDeployBeaconClass() {
	return GetClass("Class TgGame.TgDeploy_Beacon");
}

UClass* ClassPreloader::GetTgDeployBeaconEntranceClass() {
	return GetClass("Class TgGame.TgDeploy_BeaconEntrance");
}

UClass* ClassPreloader::GetTgRandomSMManagerClass() {
	return GetClass("Class TgGame.TgRandomSMManager");
}

UClass* ClassPreloader::GetTgPawnEliteAlchemistClass() {
	return GetClass("Class TgGame.TgPawn_Elite_Alchemist");
}

UClass* ClassPreloader::GetTgAIControllerClass() {
	return GetClass("Class TgGame.TgAIController");
}

UClass* ClassPreloader::GetTgPawnThinkTankClass() {
	return GetClass("Class TgGame.TgPawn_ThinkTank");
}

UClass* ClassPreloader::GetTgPawnGuardianClass() {
	return GetClass("Class TgGame.TgPawn_Guardian");
}

UClass* ClassPreloader::GetTgDeviceClass() {
	return GetClass("Class TgGame.TgDevice");
}

UClass* ClassPreloader::GetTgInventoryObjectDeviceClass() {
	return GetClass("Class TgGame.TgInventoryObject_Device");
}

UClass* ClassPreloader::GetTgHudTeamGameClass() {
	return GetClass("Class TgClient.TgHUD_TeamGame");
}

UClass* ClassPreloader::GetTgSeqEventMissionTimerClass() {
	return GetClass("Class TgGame.TgSeqEvent_MissionTimer");
}

UClass* ClassPreloader::GetTgSeqEventLevelFadedInClass() {
	return GetClass("Class TgGame.TgSeqEvent_LevelFadedIn");
}

UClass* ClassPreloader::GetSeqEventLevelLoadedClass() {
	return GetClass("Class Engine.SeqEvent_LevelLoaded");
}
