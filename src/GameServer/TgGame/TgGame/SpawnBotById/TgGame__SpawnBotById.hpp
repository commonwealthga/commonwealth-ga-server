#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgGame__SpawnBotById : public HookBase<
	ATgPawn*(__fastcall*)(ATgGame*, void*, int, FVector, FRotator, bool, ATgBotFactory*, bool, ATgPawn*, bool, UTgDeviceFire*, float),
	0x10ad9b70,
	TgGame__SpawnBotById> {
public:
	static std::map<int, int> m_spawnedBotIds;
	static ATgPawn* __fastcall Call(
		ATgGame* Game,
		void* edx,
		int nBotId,
		FVector vLocation,
		FRotator rRotation,
		bool bKillController,
		ATgBotFactory* pFactory,
		bool bIgnoreCollision,
		ATgPawn* pOwnerPawn,
		bool bIsDecoy,
		UTgDeviceFire* deviceFire,
		float fDeployAnimLength
	);
	static inline ATgPawn* __fastcall CallOriginal(
		ATgGame* Game,
		void* edx,
		int nBotId,
		FVector vLocation,
		FRotator rRotation,
		bool bKillController,
		ATgBotFactory* pFactory,
		bool bIgnoreCollision,
		ATgPawn* pOwnerPawn,
		bool bIsDecoy,
		UTgDeviceFire* deviceFire,
		float fDeployAnimLength
	) {
		return m_original(Game, edx, nBotId, vLocation, rRotation, bKillController, pFactory, bIgnoreCollision, pOwnerPawn, bIsDecoy, deviceFire, fDeployAnimLength);
	};
};

