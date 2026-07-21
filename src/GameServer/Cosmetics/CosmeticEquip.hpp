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

// Apply ONE cosmetic item to a pawn with no character/DB backing — for BOTS.
// Same dispatch as ApplyToPawn (helmet/flair writes HeadFlairId + HelmetMeshId,
// suit writes SuitFlairId + SuitMeshId) but persists nothing.
//
// Returns false — a safe no-op — unless the pawn is a TgPawn_Character. Only
// that class builds its mesh from r_CustomCharacterAssembly. Bot-mesh classes
// take their look from asm_data_set_bots.body_asm_id and have no assembly field
// at all (e.g. the Elite Helot is TgPawn_AndroidMinion), so cosmetics cannot
// show on them — and writing the field anyway would scribble whatever lives at
// that offset. The guard is deliberately TgPawn_Character ONLY, not the whole
// lineage: TgPawn_CTR has its own r_CustomCharacterAssembly at a DIFFERENT
// offset (0x1650 vs 0x1784), so the ATgPawn_Character cast is wrong for it.
bool ApplyItemToBot(ATgPawn* Pawn, int itemId);

// Clear one cosmetic slot from live assembly and profile storage. Accepts
// engine slot 6 (Suit), 12 (Helmet), or 16-20 (dyes). Suit/helmet reset their
// Flair/Mesh fields to -1 (bare — no overlay) and delete the remapped DB row
// (22/23); dyes reset to kNoCosmeticItemId. Returns false for any other slot.
bool ClearSlot(ATgPawn* Pawn, int64_t character_id, int item_profile_id, int slot);

// Class-default body suit asm_id for (profile, gender) — the mesh a pawn with
// no cosmetic suit equipped renders. Exposed for BrokenSuitSwap's
// "show as wearing no suit" sentinel.
int ClassDefaultSuitAsmId(uint32_t profileId, bool isFemale);

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
