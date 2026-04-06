#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPlayerController__ServerRequestAssignment : public HookBase<
	bool(__fastcall*)(ATgPlayerController*, void*, ATgMissionObjective*),
	0x10963970,
	TgPlayerController__ServerRequestAssignment> {
public:
	static bool __fastcall Call(ATgPlayerController* Controller, void* edx, ATgMissionObjective* Objective);
	static inline bool __fastcall CallOriginal(ATgPlayerController* Controller, void* edx, ATgMissionObjective* Objective) {
		return m_original(Controller, edx, Objective);
	}
};
