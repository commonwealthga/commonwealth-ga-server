#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgGame__SpawnBot : public HookBase<
	void(__fastcall*)(ATgGame*, void*, FName, FVector, FRotator, ATgBotFactory*, bool, ATgPawn*, bool, UTgDeviceFire*, float),
	0x10ad9ab0,
	TgGame__SpawnBot> {
public:
	static void __fastcall Call(ATgGame* Game, void* edx, FName sName, FVector vLocation, FRotator rRotation, ATgBotFactory* pFactory, bool bIgnoreCollision, ATgPawn* pOwnerPawn, bool bIsDecoy, UTgDeviceFire* deviceFire, float fDeploySecs);
	static inline void __fastcall CallOriginal(ATgGame* Game, void* edx, FName sName, FVector vLocation, FRotator rRotation, ATgBotFactory* pFactory, bool bIgnoreCollision, ATgPawn* pOwnerPawn, bool bIsDecoy, UTgDeviceFire* deviceFire, float fDeploySecs) {
		m_original(Game, edx, sName, vLocation, rRotation, pFactory, bIgnoreCollision, pOwnerPawn, bIsDecoy, deviceFire, fDeploySecs);
	};
};

