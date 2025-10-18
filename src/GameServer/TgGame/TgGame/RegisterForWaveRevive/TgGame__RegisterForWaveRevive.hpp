#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgGame__RegisterForWaveRevive : public HookBase<
	void(__fastcall*)(ATgGame*, void*, AController*),
	0x10ad9c80,
	TgGame__RegisterForWaveRevive> {
public:
	static void __fastcall Call(ATgGame* Game, void* edx, AController* Controller);
	static inline void __fastcall CallOriginal(ATgGame* Game, void* edx, AController* Controller) {
		m_original(Game, edx, Controller);
	};
};


