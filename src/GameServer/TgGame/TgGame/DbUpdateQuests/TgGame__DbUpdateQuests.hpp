#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgGame__DbUpdateQuests : public HookBase<
	void(__fastcall*)(ATgGame*, void*),
	0x10ad9b40,
	TgGame__DbUpdateQuests> {
public:
	static void __fastcall Call(ATgGame* Game, void* edx);
	static inline void __fastcall CallOriginal(ATgGame* Game, void* edx) {
		m_original(Game, edx);
	};
};
