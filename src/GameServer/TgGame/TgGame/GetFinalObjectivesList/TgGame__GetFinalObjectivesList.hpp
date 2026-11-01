#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// Returns TArray by value: MSVC passes a hidden result pointer on the stack
// (original does ret 0x4) and returns it in EAX.
class TgGame__GetFinalObjectivesList : public HookBase<
	TArray<ATgMissionObjective*>*(__fastcall*)(ATgGame*, void*, TArray<ATgMissionObjective*>*),
	0x10ad9f20,
	TgGame__GetFinalObjectivesList> {
public:
	static TArray<ATgMissionObjective*>* __fastcall Call(ATgGame* Game, void* edx, TArray<ATgMissionObjective*>* Result);
	static inline TArray<ATgMissionObjective*>* __fastcall CallOriginal(ATgGame* Game, void* edx, TArray<ATgMissionObjective*>* Result) {
		return m_original(Game, edx, Result);
	};
};
