#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPawn_Character__ApplyItemEffects : public HookBase<
	bool(__fastcall*)(ATgPawn_Character*, void*, UTgInventoryObject*, unsigned long),
	0x109c0e20,
	TgPawn_Character__ApplyItemEffects> {
public:
	static bool __fastcall Call(ATgPawn_Character* Pawn, void* edx, UTgInventoryObject* pItem, unsigned long bRemove);
	static inline bool __fastcall CallOriginal(ATgPawn_Character* Pawn, void* edx, UTgInventoryObject* pItem, unsigned long bRemove) {
		return m_original(Pawn, edx, pItem, bRemove);
	};
};
