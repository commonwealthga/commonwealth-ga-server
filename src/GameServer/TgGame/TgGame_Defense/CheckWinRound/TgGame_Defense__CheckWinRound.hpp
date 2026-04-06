#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgGame_Defense__CheckWinRound : public HookBase<
	bool(__fastcall*)(ATgGame_Defense*, void*, ATgRepInfo_TaskForce**),
	0x10ad9ea0,
	TgGame_Defense__CheckWinRound> {
public:
	static bool __fastcall Call(ATgGame_Defense* Game, void* edx, ATgRepInfo_TaskForce** Winner);
	static inline bool __fastcall CallOriginal(ATgGame_Defense* Game, void* edx, ATgRepInfo_TaskForce** Winner) {
		return m_original(Game, edx, Winner);
	};
};
