#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgGame_Control__CalcDefenderReviveTime : public HookBase<
	float(__fastcall*)(ATgGame_Control*, void*),
	0x10ad9dd0,
	TgGame_Control__CalcDefenderReviveTime> {
public:
	static float __fastcall Call(ATgGame_Control* Game, void* edx);
	static inline float __fastcall CallOriginal(ATgGame_Control* Game, void* edx) {
		return m_original(Game, edx);
	};
};
