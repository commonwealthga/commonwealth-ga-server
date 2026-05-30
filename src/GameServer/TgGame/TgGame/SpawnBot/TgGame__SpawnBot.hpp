#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// Native signature (UC TgGame.uc:188):
//   native function Pawn SpawnBot(name sName, Vector vLocation, Rotator rRotation,
//                                 TgBotFactory pFactory, ...);
// MSVC __fastcall ABI: returns APawn* in EAX (pointer, NOT a struct return — no
// hidden out-pointer arg). The hook must declare APawn* return type or the
// caller reads EAX as garbage → null-deref crash.
class TgGame__SpawnBot : public HookBase<
	APawn*(__fastcall*)(ATgGame*, void*, FName, FVector, FRotator, ATgBotFactory*, bool, ATgPawn*, bool, UTgDeviceFire*, float),
	0x10ad9ab0,
	TgGame__SpawnBot> {
public:
	static APawn* __fastcall Call(ATgGame* Game, void* edx, FName sName, FVector vLocation, FRotator rRotation, ATgBotFactory* pFactory, bool bIgnoreCollision, ATgPawn* pOwnerPawn, bool bIsDecoy, UTgDeviceFire* deviceFire, float fDeploySecs);
	static inline APawn* __fastcall CallOriginal(ATgGame* Game, void* edx, FName sName, FVector vLocation, FRotator rRotation, ATgBotFactory* pFactory, bool bIgnoreCollision, ATgPawn* pOwnerPawn, bool bIsDecoy, UTgDeviceFire* deviceFire, float fDeploySecs) {
		return m_original(Game, edx, sName, vLocation, rRotation, pFactory, bIgnoreCollision, pOwnerPawn, bIsDecoy, deviceFire, fDeploySecs);
	};
};

