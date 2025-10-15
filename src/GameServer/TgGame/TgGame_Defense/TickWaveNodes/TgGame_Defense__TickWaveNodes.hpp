#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgGame_Defense__TickWaveNodes : public HookBase<
	void(__fastcall*)(ATgGame_Defense*, void*, int),
	0x10ad9bf0,
	TgGame_Defense__TickWaveNodes> {
public:
	static void __fastcall Call(ATgGame_Defense* Game, void* edx, int nPriority);
	static inline void __fastcall CallOriginal(ATgGame_Defense* Game, void* edx, int nPriority) {
		m_original(Game, edx, nPriority);
	};
};


