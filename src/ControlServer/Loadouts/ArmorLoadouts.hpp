#pragma once

#include <cstdint>

// =============================================================================
// ArmorLoadouts — universal armor seed for every character.
//
// Every player gets the same 35 armor pieces in inventory: 7 slots × 5 mod
// variants. Variants differ only in their `mod_effect_group_ids` CSV — the
// base item per slot is identical across variants. The UI suffix on each
// piece is what the player reads to tell them apart (e.g. "[rrrrrr]" vs
// "[nnnnnn]"); each mod_effect_group_id in the CSV renders one letter via
// its prop's ui_code.
//
// One variant per slot is equipped by default at character creation —
// kDefaultVariantIndex picks which (currently `[rrrrrr]`, the Ranged
// Protection mod-pure variant).
//
// All four mod-kit egid constants live in ModConstants.hpp's ArmorBallistics
// / ArmorPadding / ArmorFlak / ArmorPlate namespaces; the values inlined
// here are the EPIC-tier picks, single source of truth cross-checked at
// review time.
// =============================================================================

namespace ArmorLoadouts {

struct ArmorVariant {
    const char* mods_csv;  // mod_effect_group_ids column value
    const char* name;      // "[rrrrrr]" — logging only
};

struct ArmorSlotEntry {
    int slot_value_id;     // group-126 SVID (also the `equipped_slot` DB value)
    int base_item_id;      // asm_data_set_items.item_id (NOT row PK .id)
    int base_effect_egid;  // The "+10% Health Mod" baseline mod attached to the
                           // item's first blueprint's Common-tier (1165) slot.
                           // Resolved via:
                           //   SELECT bmg.effect_group_id
                           //   FROM asm_data_set_blueprints b
                           //   JOIN asm_data_set_blueprint_item_mods bim
                           //     ON bim.blueprint_id = b.blueprint_id
                           //   JOIN asm_data_set_blueprint_mod_effect_groups bmg
                           //     ON bmg.blueprint_mod_id = bim.blueprint_mod_id
                           //   JOIN asm_data_set_effects e
                           //     ON e.effect_group_id = bmg.effect_group_id
                           //   WHERE b.created_item_id = <slot's base_item_id>
                           //     AND bim.quality_value_id = 1165   -- Common
                           //     AND e.prop_id = 390              -- Health Mod
                           //   LIMIT 1;
                           // Each row carries prop 390 ("Health Mod") base 10.0
                           // calc 68 class 157 — the canonical per-piece +10%
                           // MaxHP baseline. Prepended to every variant's mod
                           // CSV so the client's item card renders it AND
                           // Armor.cpp's per-egid ApplyBuff fanout picks it up.
                           // No `ui_code` on prop 390 → no letter in the suffix.
    const char* name;      // "Head" / "Shoulder" / … — logging only
};

constexpr int kVariantCount = 5;
constexpr int kSlotCount    = 7;

// stock_n in the inventory row equals the variant's index here.
// kDefaultVariantIndex picks which variant is auto-equipped per slot.
constexpr int kDefaultVariantIndex = 0;  // [rrrrrr]

// EPIC-tier kit egids:
//   ArmorBallistics::EPIC = 24165  (prop 218 Protection-Ranged   'r')
//   ArmorPadding::EPIC    = 24074  (prop 412 Health Max Mod      'n')
//   ArmorFlak::EPIC       = 24083  (prop 219 Protection-AOE      'b')
//   ArmorPlate::EPIC      = 24170  (prop 217 Protection-Melee    'm')
inline constexpr ArmorVariant kVariants[kVariantCount] = {
    { "24165,24165,24165,24165,24165,24165", "[rrrrrr]" },  // index 0 — default
    { "24074,24074,24074,24074,24074,24074", "[nnnnnn]" },
    { "24083,24083,24083,24083,24083,24083", "[bbbbbb]" },
    { "24170,24170,24170,24170,24170,24170", "[mmmmmm]" },
    { "24165,24165,24165,24074,24074,24074", "[rrrnnn]" },  // half-r / half-n
};

// Per-slot canonical base item. Picked for plain client-side localization
// labels ("Head Armor" / "Chest Armor" / etc.) rather than themed names
// ("Nanotube Head Armor", "Explosive Head Guard", etc.) — confirmed via
// `JOIN asm_data_set_msg_translations m ON m.msg_id = i.name_msg_id`. These
// are game `item_id`s, NOT row PKs (same trap as cosmetic seeding).
// slot_value_id = group-129 "Enhancement Slot Unlock" SVID (decoded from
// FTGEQUIP_SLOTS_STRUCT.MiscItems[] empirically via 2026-05-30 equip-save
// log). MiscItems index = slot_value_id - 1128, so e.g. Head's SVID 1130
// → MiscItems[2]. The 5 body slots in group 129 (Head/Chest/Arms/Legs/Feet)
// map directly to their Armor SVIDs (1130/1133/1136/1139/1142). The OG UI
// repurposed two of the Core/Implant slots for Hands and Shoulders that
// group 129 doesn't natively list:
//   Hands     → MiscItems[4]  → SVID 1132 (would be "Head Implant")
//   Shoulders → MiscItems[15] → SVID 1143 (would be "Feet Core")
// item_subtype_value_id from group 126 (1107/1109/1110/1113/1116/1119/1120)
// is still the item-side discriminator (used for inventory-pool browsing
// and "what slot does this piece fit" UI logic); slot_value_id is what
// goes on the wire and into `ga_character_devices.equipped_slot`.
inline constexpr ArmorSlotEntry kSlots[kSlotCount] = {
    { 1130, 5914, 23173, "Head"     },  // "Head Armor"     (.id=2367, blueprint 5912)
    { 1143, 5915, 23211, "Shoulder" },  // "Shoulder Armor" (.id=2368, blueprint 5921)
    { 1133, 5916, 23224, "Chest"    },  // "Chest Armor"    (.id=2369, blueprint 5922)
    { 1136, 5917, 23237, "Arms"     },  // "Arm Armor"      (.id=2370, blueprint 5923)
    { 1132, 5918, 23289, "Hands"    },  // "Hand Armor"     (.id=2371, blueprint 5927)
    { 1139, 5919, 23302, "Legs"     },  // "Leg Armor"      (.id=2372, blueprint 5928)
    { 1142, 5920, 23315, "Feet"     },  // "Foot Armor"     (.id=2373, blueprint 5929)
};

// Universal armor blueprint id used as the wire `BLUEPRINT_ID` for every
// armor inventory record so the client's `FUN_10a13820` mod-application
// path doesn't skip — that path bails on `BLUEPRINT_ID == 0`. The blueprint
// just needs to resolve via `AssemblyManager::GetBlueprint(id)`; the
// parser doesn't validate blueprint↔item-id match, and the rendered
// `[…]` letters come from each effect's `prop.ui_code` (not from the
// blueprint's mod pool). 5973 is the "Padded Chest Guard" blueprint —
// any existing armor blueprint id works equally well; picked this one
// because it carries the full mod-slot schema (4 quality tiers).
//
// 4 of 7 slots have a matching plain-named blueprint in the DB (Shoulder
// 5913 → item 5915); the rest don't. Rather than mix-and-match per slot,
// using a single id keeps the wire code simple and avoids a per-slot
// blueprint lookup that would have to fall back anyway.
constexpr int kUniversalArmorBlueprintId = 5973;

}  // namespace ArmorLoadouts
