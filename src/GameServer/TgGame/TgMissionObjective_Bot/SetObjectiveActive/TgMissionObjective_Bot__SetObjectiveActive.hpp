#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgMissionObjective_Bot__SetObjectiveActive : public HookBase<
	void(__fastcall*)(ATgMissionObjective_Bot*, void*, unsigned long),
	0x10a797c0,
	TgMissionObjective_Bot__SetObjectiveActive> {
public:
	static void __fastcall Call(ATgMissionObjective_Bot* Obj, void* edx, unsigned long bActive);
	static inline void __fastcall CallOriginal(ATgMissionObjective_Bot* Obj, void* edx, unsigned long bActive) {
		m_original(Obj, edx, bActive);
	};
};
