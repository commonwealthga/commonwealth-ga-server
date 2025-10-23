#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgMissionObjective_Bot__SpawnObjectiveBot : public HookBase<
	void(__fastcall*)(ATgMissionObjective_Bot*, void*),
	0x10a797d0,
	TgMissionObjective_Bot__SpawnObjectiveBot> {
public:
	static void __fastcall Call(ATgMissionObjective_Bot* ObjectiveBot, void* edx);
	static inline void __fastcall CallOriginal(ATgMissionObjective_Bot* ObjectiveBot, void* edx) {
		m_original(ObjectiveBot, edx);
	};
};


