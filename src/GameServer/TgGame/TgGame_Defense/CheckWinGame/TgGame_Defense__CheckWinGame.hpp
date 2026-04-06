#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgGame_Defense__CheckWinGame : public HookBase<
	bool(__fastcall*)(ATgGame_Defense*, void*, unsigned long, ATgRepInfo_TaskForce**),
	0x10ad9e90,
	TgGame_Defense__CheckWinGame> {
public:
	static bool __fastcall Call(ATgGame_Defense* Game, void* edx, unsigned long bForceWin, ATgRepInfo_TaskForce** Winner);
	static inline bool __fastcall CallOriginal(ATgGame_Defense* Game, void* edx, unsigned long bForceWin, ATgRepInfo_TaskForce** Winner) {
		return m_original(Game, edx, bForceWin, Winner);
	};
};
