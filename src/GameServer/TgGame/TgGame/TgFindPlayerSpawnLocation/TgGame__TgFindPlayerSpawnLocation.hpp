#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgGame__TgFindPlayerSpawnLocation : public HookBase<
	bool(__fastcall*)(ATgGame*, void*, AController*, FVector*),
	0x10ad9d00,
	TgGame__TgFindPlayerSpawnLocation> {
public:
	static bool __fastcall Call(ATgGame* Game, void* edx, AController* pPlayer, FVector* vLoc);
	static inline bool __fastcall CallOriginal(ATgGame* Game, void* edx, AController* pPlayer, FVector* vLoc) {
		return m_original(Game, edx, pPlayer, vLoc);
	};
};
