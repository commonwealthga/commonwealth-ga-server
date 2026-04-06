#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgGame__SpawnAllHenchman : public HookBase<
	void(__fastcall*)(ATgGame*, void*),
	0x10ad9ae0,
	TgGame__SpawnAllHenchman> {
public:
	static void __fastcall Call(ATgGame* Game, void* edx);
	static inline void __fastcall CallOriginal(ATgGame* Game, void* edx) {
		m_original(Game, edx);
	};
};
