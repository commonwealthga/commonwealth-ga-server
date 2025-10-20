#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgGame__MissionTimeRemaining : public HookBase<
	float(__fastcall*)(ATgGame*, void*),
	0x10ad9c40,
	TgGame__MissionTimeRemaining> {
public:
	static float __fastcall Call(ATgGame* Game, void* edx);
	static inline float __fastcall CallOriginal(ATgGame* Game, void* edx) {
		return m_original(Game, edx);
	};
};


