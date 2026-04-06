#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgGame_Ticket__AwardTickets : public HookBase<
	void(__fastcall*)(ATgGame_Ticket*, void*),
	0x10ad9d40,
	TgGame_Ticket__AwardTickets> {
public:
	static void __fastcall Call(ATgGame_Ticket* Game, void* edx);
	static inline void __fastcall CallOriginal(ATgGame_Ticket* Game, void* edx) {
		m_original(Game, edx);
	};
};
