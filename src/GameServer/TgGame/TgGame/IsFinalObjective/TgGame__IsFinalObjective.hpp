#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgGame__IsFinalObjective : public HookBase<
	bool(__fastcall*)(ATgGame*, void*, ATgMissionObjective*),
	0x10ad9c20,
	TgGame__IsFinalObjective> {
public:
	static bool __fastcall Call(ATgGame* Game, void* edx, ATgMissionObjective* Objective);
	static inline bool __fastcall CallOriginal(ATgGame* Game, void* edx, ATgMissionObjective* Objective) {
		return m_original(Game, edx, Objective);
	};
};


