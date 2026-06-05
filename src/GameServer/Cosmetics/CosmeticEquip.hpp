#pragma once

#include <cstdint>
#include <set>

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
bool ApplyToPawn(ATgPawn* Pawn, int64_t character_id, int item_profile_id,
                 int slot, int invId, int itemId);

// On pawn spawn/profile switch, replay rows from ga_character_devices whose
// joined inventory row is a cosmetic (item_id > 0) for the active profile, applying
// each via the same dispatch as ApplyToPawn but WITHOUT re-persisting
// (those rows already exist). Pawn comes up wearing the player's
// last-saved appearance for that profile.
void LoadFromDB(ATgPawn* Pawn, int64_t character_id, int item_profile_id);

// Reset r_CustomCharacterAssembly fields whose engine equip-points are
// NOT present in `equippedEngineSlots` to profile defaults. Used by the
// loadout-profile switch hook: when the new profile has nothing in a
// cosmetic slot (helmet, suit flair, dye N, trail), the previous
// profile's visual must drop. `equippedEngineSlots` should contain the
// engine equip-points the new profile DID populate; everything else gets
// reset (HeadFlairId=-1, SuitFlairId=-1, HelmetMeshId=-1,
// JetpackTrailId=0, DyeList[i]=0). Mirrors
// writes to PRI and bumps bNetDirty so the client repaints.
void ClearUnsetSlots(ATgPawn* Pawn, const std::set<int>& equippedEngineSlots);

}  // namespace CosmeticEquip
