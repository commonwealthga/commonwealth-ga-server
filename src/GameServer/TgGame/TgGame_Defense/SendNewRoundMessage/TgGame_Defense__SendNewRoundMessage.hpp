#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgGame_Defense__SendNewRoundMessage : public HookBase<
	void(__fastcall*)(ATgGame_Defense*, void*),
	0x10ad9f10,
	TgGame_Defense__SendNewRoundMessage> {
public:
	static void __fastcall Call(ATgGame_Defense* Game, void* edx);
	static inline void __fastcall CallOriginal(ATgGame_Defense* Game, void* edx) {
		m_original(Game, edx);
	};
};
