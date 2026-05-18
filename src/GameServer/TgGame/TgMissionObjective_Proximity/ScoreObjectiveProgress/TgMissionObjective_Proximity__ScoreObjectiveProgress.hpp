#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgMissionObjective_Proximity__ScoreObjectiveProgress : public HookBase<
	void(__fastcall*)(ATgMissionObjective_Proximity*, void*, float),
	0x10a79790,
	TgMissionObjective_Proximity__ScoreObjectiveProgress> {
public:
	static void __fastcall Call(ATgMissionObjective_Proximity* Obj, void* edx, float fDeltaTime);
	static inline void __fastcall CallOriginal(ATgMissionObjective_Proximity* Obj, void* edx, float fDeltaTime) {
		m_original(Obj, edx, fDeltaTime);
	};
};
