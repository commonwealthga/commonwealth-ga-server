#pragma once

#include "src/pch.hpp"

// Thin proxy over ObjectCache. Keeps existing API to avoid refactoring all callers.
class ClassPreloader {
public:
	static UClass* GetClass(const char* ClassName);
	static UObject* GetObject(const char* FullName);

	static UClass* GetTgPawnCharacterClass();
	static UClass* GetTgPropertyClass();
	static UClass* GetTgRepInfoTaskForceClass();
	static UClass* GetTgDeployableClass();
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
	static UClass* GetTgSeqEventMissionTimeRemainingClass();
	static UClass* GetTgSeqEventLevelFadedInClass();
	static UClass* GetSeqEventLevelLoadedClass();
};
