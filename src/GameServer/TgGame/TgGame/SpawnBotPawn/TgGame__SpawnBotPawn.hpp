#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgGame__SpawnBotPawn : public HookBase<
	ATgPawn*(__fastcall*)(ATgGame*, void*, ATgAIController*, FVector, FRotator, bool, ATgPawn*, bool, float),
	0x10ad9ac0,
	TgGame__SpawnBotPawn> {
public:
	static ATgPawn* __fastcall Call(ATgGame* Game, void* edx, ATgAIController* pTgAI, FVector vLocation, FRotator rRotation, bool bIgnoreCollision, ATgPawn* pOwner, bool bIsDecoy, float fDeploySecs);
	static inline ATgPawn* __fastcall CallOriginal(ATgGame* Game, void* edx, ATgAIController* pTgAI, FVector vLocation, FRotator rRotation, bool bIgnoreCollision, ATgPawn* pOwner, bool bIsDecoy, float fDeploySecs) {
		m_original(Game, edx, pTgAI, vLocation, rRotation, bIgnoreCollision, pOwner, bIsDecoy, fDeploySecs);
	};
};

