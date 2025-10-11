#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgGame__SpawnPlayerCharacter : public HookBase<
	ATgPawn_Character*(__fastcall*)(ATgGame*, void*, ATgPlayerController*, FVector),
	0x10AD9AF0,
	TgGame__SpawnPlayerCharacter> {
public:
	static ATgPawn_Character* __fastcall Call(ATgGame* Game, void* edx, ATgPlayerController* PlayerController, FVector SpawnLocation);
	static inline ATgPawn_Character* __fastcall CallOriginal(ATgGame* Game, void* edx, ATgPlayerController* PlayerController, FVector SpawnLocation) {
		return m_original(Game, edx, PlayerController, SpawnLocation);
	};
};
