#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPawn_Character__CraftItem : public HookBase<
	void(__fastcall*)(ATgPawn_Character*, void*, int, unsigned long, unsigned long, int),
	0x109c1010,
	TgPawn_Character__CraftItem> {
public:
	static void __fastcall Call(ATgPawn_Character* Pawn, void* edx, int nBlueprintId, unsigned long bSystemCraft, unsigned long bUseComponents, int nSystemCraftLevel);
	static inline void __fastcall CallOriginal(ATgPawn_Character* Pawn, void* edx, int nBlueprintId, unsigned long bSystemCraft, unsigned long bUseComponents, int nSystemCraftLevel) {
		m_original(Pawn, edx, nBlueprintId, bSystemCraft, bUseComponents, nSystemCraftLevel);
	};
};
