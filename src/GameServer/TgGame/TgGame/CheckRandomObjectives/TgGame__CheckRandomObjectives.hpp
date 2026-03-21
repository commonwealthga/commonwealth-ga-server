#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgGame__CheckRandomObjectives : public HookBase<
	void(__fastcall*)(ATgGame*, void*),
	0x10ad9bd0,
	TgGame__CheckRandomObjectives> {
public:
	static void __fastcall Call(ATgGame* Game, void* edx);
	static inline void __fastcall CallOriginal(ATgGame* Game, void* edx) {
		m_original(Game, edx);
	};
};


