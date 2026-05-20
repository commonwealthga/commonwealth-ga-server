#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// UTgDroppedItem::GetEffectGroup — original is a stripped no-op stub @ 0x109af110
// that always returns null and sets *nIndex = -1, terminating UC's GiveTo loop
// immediately. With this hook in place the loop iterates s_EffectGroupList by
// type and ProcessEffect is invoked for each matching aura.
//
// UC signature: native function TgEffectGroup GetEffectGroup(int nType, out int nIndex);
// Called only from TgDroppedItem.uc's GiveTo with nType=264 (aura).
class TgDroppedItem__GetEffectGroup : public HookBase<
	UTgEffectGroup*(__fastcall*)(ATgDroppedItem*, void*, int, int*),
	0x109af110,
	TgDroppedItem__GetEffectGroup> {
public:
	static UTgEffectGroup* __fastcall Call(ATgDroppedItem* Item, void* edx, int nType, int* nIndex);
	static inline UTgEffectGroup* __fastcall CallOriginal(ATgDroppedItem* Item, void* edx, int nType, int* nIndex) {
		return m_original(Item, edx, nType, nIndex);
	}
};
