#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgGame_Defense__FinalizeRoundScore : public HookBase<
	void(__fastcall*)(ATgGame_Defense*, void*, ATgRepInfo_TaskForce*),
	0x10ad9ee0,
	TgGame_Defense__FinalizeRoundScore> {
public:
	static void __fastcall Call(ATgGame_Defense* Game, void* edx, ATgRepInfo_TaskForce* Winner);
	static inline void __fastcall CallOriginal(ATgGame_Defense* Game, void* edx, ATgRepInfo_TaskForce* Winner) {
		m_original(Game, edx, Winner);
	};
};
