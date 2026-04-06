#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgGame_Arena__AdjustBeaconForwardSpawn : public HookBase<
	void(__fastcall*)(ATgGame_Arena*, void*, int),
	0x10ad9d90,
	TgGame_Arena__AdjustBeaconForwardSpawn> {
public:
	static void __fastcall Call(ATgGame_Arena* Game, void* edx, int nPriority);
	static inline void __fastcall CallOriginal(ATgGame_Arena* Game, void* edx, int nPriority) {
		m_original(Game, edx, nPriority);
	};
};
