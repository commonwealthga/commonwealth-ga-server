#pragma once

#include <cstdint>

class ATgPawn;

namespace CosmeticEquip {

// Apply a single cosmetic item to the pawn at engine equip point `slot`.
// `invId` is the row in ga_players_inventory (for persistence/logging);
// the caller resolved it to `itemId` (= asm_data_set_items.id) already.
//
// Dispatches by (item_type_value_id, item_subtype_value_id) lookup:
//   subtype 1006 / 1007 → r_CustomCharacterAssembly.HeadFlairId = itemId
//   subtype 1008        → r_CustomCharacterAssembly.SuitFlairId = itemId
//   type 1020 (dye)     → SetDyeItemId(itemId, slotToEDyeSlots(slot)) + ApplyDye
//   type 1612 (trail)   → SetJetpackTrailId(itemId) + ApplyJetpackTrail
//
// Persists the equip via INSERT OR REPLACE into ga_character_devices.
// Returns true on success.
bool ApplyToPawn(ATgPawn* Pawn, int64_t character_id, int slot, int invId, int itemId);

// On pawn spawn, replay all rows from ga_character_devices whose joined
// inventory row is a cosmetic (item_id > 0) for this character, applying
// each via the same dispatch as ApplyToPawn but WITHOUT re-persisting
// (those rows already exist). Pawn comes up wearing the player's
// last-saved appearance.
void LoadFromDB(ATgPawn* Pawn, int64_t character_id);

}  // namespace CosmeticEquip
