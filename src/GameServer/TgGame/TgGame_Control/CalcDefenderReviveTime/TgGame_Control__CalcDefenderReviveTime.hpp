#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgGame_Control__CalcDefenderReviveTime : public HookBase<
	void(__fastcall*)(ATgGame_Control*, void*, int),
	0x10ad9bf0,
	TgGame_Control__CalcDefenderReviveTime> {
public:
	static void __fastcall Call(ATgGame_Control* Game, void* edx, int nPriority);
	static inline void __fastcall CallOriginal(ATgGame_Control* Game, void* edx, int nPriority) {
		m_original(Game, edx, nPriority);
	};
};


