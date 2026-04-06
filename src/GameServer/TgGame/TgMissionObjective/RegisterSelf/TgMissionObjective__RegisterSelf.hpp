#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgMissionObjective__RegisterSelf : public HookBase<
	void(__fastcall*)(ATgMissionObjective*, void*),
	0x10a79640,
	TgMissionObjective__RegisterSelf> {
public:
	static void __fastcall Call(ATgMissionObjective* Objective, void* edx);
	static inline void __fastcall CallOriginal(ATgMissionObjective* Objective, void* edx) {
		m_original(Objective, edx);
	};
};
