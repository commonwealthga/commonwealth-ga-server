#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgGame__FinishEndMission : public HookBase<
	bool(__fastcall*)(ATgGame*, void*),
	0x10ad9b10,
	TgGame__FinishEndMission> {
public:
	static bool __fastcall Call(ATgGame* Game, void* edx);
	static inline bool __fastcall CallOriginal(ATgGame* Game, void* edx) {
		return m_original(Game, edx);
	};
};
