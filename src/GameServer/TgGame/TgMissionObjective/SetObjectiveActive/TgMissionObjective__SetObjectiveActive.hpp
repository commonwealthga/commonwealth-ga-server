#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgMissionObjective__SetObjectiveActive : public HookBase<
	void(__fastcall*)(ATgMissionObjective*, void*, unsigned long),
	0x10a79660,
	TgMissionObjective__SetObjectiveActive> {
public:
	static void __fastcall Call(ATgMissionObjective* Obj, void* edx, unsigned long bActive);
	static inline void __fastcall CallOriginal(ATgMissionObjective* Obj, void* edx, unsigned long bActive) {
		m_original(Obj, edx, bActive);
	};
};


