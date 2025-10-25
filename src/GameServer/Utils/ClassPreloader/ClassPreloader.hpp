#pragma once

#include "src/pch.hpp"

class ClassPreloader {
private:
	static bool bClassesPreloaded;
	static void PreloadClasses();

	static std::map <std::string, UClass*> Classes;
public:
	static UClass* GetClass(char* ClassName);
	static UClass* GetTgPawnCharacterClass();
	static UClass* GetTgPropertyClass();
	static UClass* GetTgRepInfoTaskForceClass();
	static UClass* GetTgDeployBeaconClass();
	static UClass* GetTgDeployBeaconEntranceClass();
	static UClass* GetTgRandomSMManagerClass();
	static UClass* GetTgPawnEliteAlchemistClass();
	static UClass* GetTgAIControllerClass();
	static UClass* GetTgPawnThinkTankClass();
	static UClass* GetTgPawnGuardianClass();
	static UClass* GetTgDeviceClass();
	static UClass* GetTgInventoryObjectDeviceClass();
	static UClass* GetTgHudTeamGameClass();
	static UClass* GetTgSeqEventMissionTimerClass();
	static UClass* GetTgSeqEventLevelFadedInClass();
	static UClass* GetSeqEventLevelLoadedClass();

};

