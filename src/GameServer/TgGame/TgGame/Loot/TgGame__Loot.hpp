#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgGame__Loot : public HookBase<
	void(__fastcall*)(ATgGame*, void*, int, FVector, int),
	0x10ad9c70,
	TgGame__Loot> {
public:
	static void __fastcall Call(ATgGame* Game, void* edx, int nLootTableId, FVector vBaseLocation, int nTaskForce);
	static inline void __fastcall CallOriginal(ATgGame* Game, void* edx, int nLootTableId, FVector vBaseLocation, int nTaskForce) {
		m_original(Game, edx, nLootTableId, vBaseLocation, nTaskForce);
	};
};
