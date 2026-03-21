#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgGame__GetFinalObjectivesList : public HookBase<
	TArray<ATgMissionObjective*>*(__fastcall*)(ATgGame*, void*),
	0x10ad9f20,
	TgGame__GetFinalObjectivesList> {
public:
	static TArray<ATgMissionObjective*>* __fastcall Call(ATgGame* Game, void* edx);
	static inline TArray<ATgMissionObjective*>* __fastcall CallOriginal(ATgGame* Game, void* edx) {
		return m_original(Game, edx);
	};
};


