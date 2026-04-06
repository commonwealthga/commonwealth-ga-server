#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgGame_Control__SendCountdownRemainingMessages : public HookBase<
	void(__fastcall*)(ATgGame_Control*, void*, float, float),
	0x10ad9de0,
	TgGame_Control__SendCountdownRemainingMessages> {
public:
	static void __fastcall Call(ATgGame_Control* Game, void* edx, float oldTimeRemaining, float newTimeRemaining);
	static inline void __fastcall CallOriginal(ATgGame_Control* Game, void* edx, float oldTimeRemaining, float newTimeRemaining) {
		m_original(Game, edx, oldTimeRemaining, newTimeRemaining);
	};
};
