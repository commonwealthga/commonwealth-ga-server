#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgGame_Control__TickCountdownCalculation : public HookBase<
	void(__fastcall*)(ATgGame_Control*, void*, float),
	0x10ad9db0,
	TgGame_Control__TickCountdownCalculation> {
public:
	static void __fastcall Call(ATgGame_Control* Game, void* edx, float DeltaTime);
	static inline void __fastcall CallOriginal(ATgGame_Control* Game, void* edx, float DeltaTime) {
		m_original(Game, edx, DeltaTime);
	};
};
