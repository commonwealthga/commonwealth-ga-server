#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// UTgDroppedItem::ApplyItemSetup — original @ 0x109b2b10 sets m_fLifeSpan from
// asm_data_set_items.time_to_live_secs and attaches c_Mesh from the item's
// mesh resource, but DOES NOT populate s_EffectGroupList — leaving GiveTo with
// nothing to give. This hook calls the original first (mesh + lifespan), then
// queries asm_data_set_item_effect_groups for the item and builds each
// effect group via the shared BuildEffectGroup utility.
//
// Called from:
//   - UC TgDroppedItem.ReplicatedEvent on the CLIENT when r_nItemId arrives
//     (the canonical path).
//   - Our TgPawn__SpawnLoot reimpl SERVER-side, right after Spawn() sets
//     r_nItemId, so the server-side GiveTo loop also has a populated list.
class TgDroppedItem__ApplyItemSetup : public HookBase<
	bool(__fastcall*)(ATgDroppedItem*, void*),
	0x109b2b10,
	TgDroppedItem__ApplyItemSetup> {
public:
	static bool __fastcall Call(ATgDroppedItem* Item, void* edx);
	static inline bool __fastcall CallOriginal(ATgDroppedItem* Item, void* edx) {
		return m_original(Item, edx);
	}
};
