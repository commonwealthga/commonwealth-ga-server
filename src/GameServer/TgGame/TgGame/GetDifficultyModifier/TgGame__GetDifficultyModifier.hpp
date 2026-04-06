#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgGame__GetDifficultyModifier : public HookBase<
	float(__fastcall*)(ATgGame*, void*, int),
	0x10ad9ad0,
	TgGame__GetDifficultyModifier> {
public:
	static float __fastcall Call(ATgGame* Game, void* edx, int nPriority);
	static inline float __fastcall CallOriginal(ATgGame* Game, void* edx, int nPriority) {
		return m_original(Game, edx, nPriority);
	};
};
