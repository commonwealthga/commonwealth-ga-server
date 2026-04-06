#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgGame__SpawnTemplatePlayer : public HookBase<
	void*(__fastcall*)(ATgGame*, void*, ATgPlayerController*, FName),
	0x10ad9b60,
	TgGame__SpawnTemplatePlayer> {
public:
	static void* __fastcall Call(ATgGame* Game, void* edx, ATgPlayerController* pTgPC, FName sName);
	static inline void* __fastcall CallOriginal(ATgGame* Game, void* edx, ATgPlayerController* pTgPC, FName sName) {
		return m_original(Game, edx, pTgPC, sName);
	};
};
