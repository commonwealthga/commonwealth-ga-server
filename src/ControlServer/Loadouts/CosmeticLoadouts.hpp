#pragma once

#include <cstdint>

namespace CosmeticLoadouts {

// Per-class default equipped cosmetic item_ids. Each field is an
// asm_data_set_items.id (NOT a mesh asm_id). The cosmetic-equip layer
// writes these into r_CustomCharacterAssembly on the pawn:
//   helmet_item_id  → HeadFlairId  (engine equip point 12, svid 500)
//   suit_item_id    → SuitFlairId  (engine equip point 6,  svid 202)
//   dye_*           → DyeList[0..4] via SetDyeItemId
//   jetpack_trail   → JetpackTrailId via SetJetpackTrailId
//
// Used by the per-character "fill empty cosmetic slots" seed (INSERT OR
// IGNORE into ga_character_devices). Existing player choices win; this is
// only consulted for slots the character hasn't yet customized.
struct CosmeticDefaults {
	int helmet_item_id;   // → engine equip point 12 (SVID_HELMET 500)
	int suit_item_id;     // → engine equip point 6  (SVID_SUIT 202)
	int dye_primary;      // → engine equip point 16 (SVID_DYE_PRIMARY 996)
	int dye_secondary;    // → engine equip point 17 (SVID_DYE_SECONDARY 997)
	int dye_emissive;     // → engine equip point 18 (SVID_DYE_EMISSIVE 998)
	int dye_weapon_pri;   // → engine equip point 19 (SVID_DYE_WEAPON_PRIMARY 999)
	int dye_weapon_emi;   // → engine equip point 20 (SVID_DYE_WEAPON_EMISSIVE 1000)
	int jetpack_trail;    // → engine equip point 21 (SVID_DYE_JETPACK_TRAIL 1001)
};

const CosmeticDefaults& GetDefaultsForProfile(uint32_t profile_id);

}  // namespace CosmeticLoadouts
