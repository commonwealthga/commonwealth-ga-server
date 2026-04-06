#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgGame__CalcDefenderReviveTime : public HookBase<
	float(__fastcall*)(ATgGame*, void*),
	0x10ad9cd0,
	TgGame__CalcDefenderReviveTime> {
public:
	static float __fastcall Call(ATgGame* Game, void* edx);
	static inline float __fastcall CallOriginal(ATgGame* Game, void* edx) {
		return m_original(Game, edx);
	};
};
