#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgGame_Ticket__UpdateGameWinState : public HookBase<
	void(__fastcall*)(ATgGame_Ticket*, void*, int),
	0x10ad9bf0,
	TgGame_Ticket__UpdateGameWinState> {
public:
	static void __fastcall Call(ATgGame_Ticket* Game, void* edx, int nPriority);
	static inline void __fastcall CallOriginal(ATgGame_Ticket* Game, void* edx, int nPriority) {
		m_original(Game, edx, nPriority);
	};
};


