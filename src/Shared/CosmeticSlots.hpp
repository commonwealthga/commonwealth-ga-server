#pragma once

// Server-side remap from the client-facing engine equip point to an internal
// DB storage slot for cosmetic Appearance items (helmet, helmet flair, suit).
//
// Problem: the gameplay suit (a device, slot_used_value_id=394) and the
// cosmetic suit overlay (item-only, subtype 1008) both target engine ES6.
// Same story for gameplay helmet (slot_used_value_id=741) vs cosmetic helmet
// /flair (subtypes 1006/1007) at ES12. `ga_character_devices` has
// UNIQUE(character_id, equipped_slot) — storing them both at slot 6/12 makes
// one silently overwrite the other.
//
// Solution: repurpose the engine's unused slots ES22/ES23 as internal DB
// storage slots for cosmetic-suit / cosmetic-head overlays. The client never
// equips anything at ES22/ES23 (no SVID maps to them in the equip screen),
// so the slots are free for our use. The wire protocol still presents these
// items at their logical ES6/ES12 — the remap is invisible to the client.
//
// Engine slots remain the source of truth for: replication, RPC payloads,
// pawn m_EquippedDevices, SVID mapping. The DB slot is bookkeeping only.

namespace CosmeticSlots {

// Internal DB slots — engine-unused ES22/ES23.
constexpr int kCosmeticSuitDbSlot   = 22;  // logical engine slot 6
constexpr int kCosmeticHelmetDbSlot = 23;  // logical engine slot 12

// Appearance item subtypes (asm_data_set_items.item_subtype_value_id).
constexpr int kSubtypeHelmet      = 1006;
constexpr int kSubtypeHelmetFlair = 1007;
constexpr int kSubtypeSuit        = 1008;

// Engine equip point → DB storage slot. Only cosmetic suit/helmet remap;
// everything else (gameplay devices, dyes, jetpack trail) passes through.
//
// `item_subtype` is the inventory item's item_subtype_value_id. Caller must
// have already established that this row is a cosmetic (device_id = 0,
// item_id > 0); otherwise pass 0 for subtype and the function passes through.
inline int DbSlotFor(int engine_slot, int item_subtype) {
    if (engine_slot == 6 && item_subtype == kSubtypeSuit) {
        return kCosmeticSuitDbSlot;
    }
    if (engine_slot == 12 &&
        (item_subtype == kSubtypeHelmet || item_subtype == kSubtypeHelmetFlair)) {
        return kCosmeticHelmetDbSlot;
    }
    return engine_slot;
}

// DB storage slot → engine equip point. Pure arithmetic inverse of DbSlotFor
// for the two remapped slots; everything else passes through.
//
// Used at SEND_INVENTORY emit time so the client sees the cosmetic suit /
// helmet at its logical ES6 / ES12 instead of the DB-internal ES22 / ES23.
inline int EngineSlotFor(int db_slot) {
    if (db_slot == kCosmeticSuitDbSlot)   return 6;
    if (db_slot == kCosmeticHelmetDbSlot) return 12;
    return db_slot;
}

}  // namespace CosmeticSlots
