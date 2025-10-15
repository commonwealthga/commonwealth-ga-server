#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgGame__TgFindPlayerSpawnLocation : public HookBase<
	void(__fastcall*)(ATgGame*, void*, int),
	0x10ad9bf0,
	TgGame__TgFindPlayerSpawnLocation> {
public:
	static void __fastcall Call(ATgGame* Game, void* edx, int nPriority);
	static inline void __fastcall CallOriginal(ATgGame* Game, void* edx, int nPriority) {
		m_original(Game, edx, nPriority);
	};
};


