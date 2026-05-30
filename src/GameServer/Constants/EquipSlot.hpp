#pragma once

namespace GA {

// ---------------------------------------------------------------------------
// EquipSlot — player equip point constants
//
// These are the slot indices used in ATgPawn::m_EquippedDevices[N] and the
// equip point parameter to Inventory::Equip().  Values match the mapping in
// FUN_109a1320 (slot value ID -> equip point) verified from Ghidra.
// ---------------------------------------------------------------------------
namespace EquipSlot {

    // --- Player equip points (1-12 are the main 12 player-facing slots) ---
    constexpr int Melee      = 1;
    constexpr int Ranged     = 2;
    constexpr int Specialty  = 3;
    constexpr int Specialty2 = 4;
    constexpr int Jetpack    = 5;
    constexpr int Offhand1   = 6;
    constexpr int Offhand2   = 7;
    constexpr int Offhand3   = 8;
    constexpr int Morale     = 9;
    constexpr int Rest       = 10;
    constexpr int Beacon     = 11;
    constexpr int Consumable = 12;

    // --- Cosmetic identifier constants ---
    // These are NOT equip points and do NOT index m_EquippedDevices.
    // They identify fields in CustomCharacterAssembly for cosmetic data.
    // Use negative values to avoid collisions with real equip points.
    //
    // Character dyes — stored in CustomCharacterAssembly::DyeList[0..2]
    constexpr int DyePrimary        = -1;
    constexpr int DyeSecondary      = -2;
    constexpr int DyeEmissive       = -3;
    // Weapon dyes — stored in CustomCharacterAssembly::DyeList[3..4]
    constexpr int DyeWeaponColor    = -4;
    constexpr int DyeWeaponEmissive = -5;
    // Cosmetic mesh/trail fields in CustomCharacterAssembly
    constexpr int Suit              = -6;   // SuitMeshId
    constexpr int Helmet            = -7;   // HelmetMeshId
    constexpr int JetpackTrail      = -8;   // JetpackTrailId

} // namespace EquipSlot

// ---------------------------------------------------------------------------
// ArmorSlot — armor enhancement slots, identified by group-129 SVID. These
// are stored as `ga_character_devices.equipped_slot` and sent on the wire
// as EQUIPPED_SLOT_VALUE_ID. The client's Armor tab decodes equipped armor
// via FTGEQUIP_SLOTS_STRUCT.MiscItems[i] where i = SVID - 1128 (verified
// empirically against an equip-save packet capture 2026-05-30).
//
// Group 129's nominal label set has 5 body parts × {Armor, Core, Implant}
// = 15 slots + Title; the shipped UI ships only 7 visible slots and
// repurposes two of the Core/Implant slots:
//   SVID 1132 (would be "Head Implant") → repurposed as Hands (MiscItems[4])
//   SVID 1143 (would be "Feet Core")    → repurposed as Shoulders (MiscItems[15])
// The other 5 use the natural "<Body Part> Armor" SVIDs from group 129.
//
// Distinct from group-126 item_subtype_value_id (1107/1109/...) which is
// the ITEM-side discriminator on `asm_data_set_items.item_subtype_value_id`.
// Subtype ↔ slot mapping is canonical-but-implicit:
//   1107 Head Armor    → 1130
//   1109 Hand Armor    → 1132 (repurposed)
//   1110 Chest Armor   → 1133
//   1113 Arm Armor     → 1136
//   1116 Leg Armor     → 1139
//   1119 Feet Armor    → 1142
//   1120 Shoulder Armor→ 1143 (repurposed)
// ---------------------------------------------------------------------------
namespace ArmorSlot {

constexpr int Head     = 1130;  // group 129 sort 1 "Head Armor"          MiscItems[2]
constexpr int Hands    = 1132;  // group 129 sort 3 (repurposed)          MiscItems[4]
constexpr int Chest    = 1133;  // group 129 sort 4 "Chest Armor"         MiscItems[5]
constexpr int Arms     = 1136;  // group 129 sort 7 "Arms Armor"          MiscItems[8]
constexpr int Legs     = 1139;  // group 129 sort 10 "Legs Armor"         MiscItems[11]
constexpr int Feet     = 1142;  // group 129 sort 13 "Feet Armor"         MiscItems[14]
constexpr int Shoulder = 1143;  // group 129 sort 14 (repurposed)         MiscItems[15]

constexpr int All[7]   = { Head, Shoulder, Chest, Arms, Hands, Legs, Feet };

// MiscItems[] index ↔ slot_value_id helpers.
// SVID = MiscItems index + 1128.
constexpr int MiscItemsIndexForSlot(int slot_value_id) { return slot_value_id - 1128; }
constexpr int SlotForMiscItemsIndex(int idx)           { return idx + 1128; }

} // namespace ArmorSlot

// ---------------------------------------------------------------------------
// SlotValueId(equipPoint) — returns the inventory slot value ID for a given
// equip point.  Mirrors the mapping in FUN_109a1320 (native binary).
// Returns 0 for unknown equip points.
// ---------------------------------------------------------------------------
constexpr int SlotValueId(int equipPoint) {
    switch (equipPoint) {
        case  1: return 221;
        case  2: return 198;
        case  3: return 200;
        case  4: return 199;
        case  5: return 201;
        case  6: return 202;
        case  7: return 203;
        case  8: return 204;
        case  9: return 385;
        case 10: return 386;
        case 11: return 499;
        case 12: return 500;
        case 13: return 501;
        case 14: return 502;
        case 15: return 823;
        case 16: return 996;
        case 17: return 997;
        case 18: return 998;
        case 19: return 999;
        case 20: return 1000;
        case 21: return 1001;
        case 22: return 1002;
        case 23: return 1003;
        case 24: return 1004;
        default: return 0;
    }
}

// ---------------------------------------------------------------------------
// EquipPointFromSlotValueId(slotValueId) — reverse of SlotValueId().
// Given a slot value ID from an inventory marshal, returns the equip point.
// Returns 0 for unknown slot value IDs.
// ---------------------------------------------------------------------------
constexpr int EquipPointFromSlotValueId(int slotValueId) {
    switch (slotValueId) {
        case 221:  return  1;
        case 198:  return  2;
        case 200:  return  3;
        case 199:  return  4;
        case 201:  return  5;
        case 202:  return  6;
        case 203:  return  7;
        case 204:  return  8;
        case 385:  return  9;
        case 386:  return 10;
        case 499:  return 11;
        case 500:  return 12;
        case 501:  return 13;
        case 502:  return 14;
        case 823:  return 15;
        case 996:  return 16;
        case 997:  return 17;
        case 998:  return 18;
        case 999:  return 19;
        case 1000: return 20;
        case 1001: return 21;
        case 1002: return 22;
        case 1003: return 23;
        case 1004: return 24;
        default:   return  0;
    }
}

} // namespace GA
