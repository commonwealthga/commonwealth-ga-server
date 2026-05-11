#pragma once

#include <cstdint>
#include <vector>

#include "src/ControlServer/Loadouts/ModConstants.hpp"

// =============================================================================
// Hardcoded class loadouts — single source of truth for what every character
// gets equipped on login. Every character logs in → ga_character_devices is
// wiped and re-inserted from GetLoadout(profile_id). Edit ClassLoadouts.cpp
// and restart; the next login reflects your changes.
//
// The DB is treated as a cache of what's defined here, not the canonical
// store. When (if) we add real inventory persistence later, only non-seed
// items would survive the re-sync.
// =============================================================================

namespace Loadouts {

// Quality value IDs (asm_data_set_valid_values group 133, by value_id).
constexpr int Q_COMMON   = 1165;
constexpr int Q_UNCOMMON = 1164;
constexpr int Q_RARE     = 1163;
constexpr int Q_EPIC     = 1162;

// Equip-slot value IDs (asm_data_set_valid_values group 48; FUN_109a1320 maps
// these → engine equip points 1..24). Names match the DB ESnn labels.
constexpr int SVID_MELEE              = 221;   // → equip point 1  (ES1  Melee)
constexpr int SVID_RANGED             = 198;   // → equip point 2  (ES2  Ranged)
constexpr int SVID_SPECIALTY          = 200;   // → equip point 3  (ES3  Specialty)
constexpr int SVID_UNUSED_4           = 199;   // → equip point 4  (ES4  ** Unused **)
constexpr int SVID_JETPACK            = 201;   // → equip point 5  (ES5  Travel)
constexpr int SVID_SUIT               = 202;   // → equip point 6  (ES6  Suit)
constexpr int SVID_OFFHAND1           = 203;   // → equip point 7  (ES7  Offhand 1)
constexpr int SVID_OFFHAND2           = 204;   // → equip point 8  (ES8  Offhand 2)
constexpr int SVID_OFFHAND3           = 385;   // → equip point 9  (ES9  Offhand 3)
constexpr int SVID_MORALE             = 386;   // → equip point 10 (ES10 Morale)
constexpr int SVID_BEACON             = 499;   // → equip point 11 (ES11 Beacon)
constexpr int SVID_HELMET             = 500;   // → equip point 12 (ES12 Helmet)
constexpr int SVID_CONSUMABLE         = 501;   // → equip point 13 (ES13 Consumable)
constexpr int SVID_CLASS_DEVICE       = 502;   // → equip point 14 (ES14 Class Device)
constexpr int SVID_UNUSED_15          = 823;   // → equip point 15 (ES15 ** Unused **)
constexpr int SVID_DYE_PRIMARY        = 996;   // → equip point 16 (ES16 Dye Primary)
constexpr int SVID_DYE_SECONDARY      = 997;   // → equip point 17 (ES17 Dye Secondary)
constexpr int SVID_DYE_EMISSIVE       = 998;   // → equip point 18 (ES18 Dye Emissive)
constexpr int SVID_DYE_WEAPON_PRIMARY = 999;   // → equip point 19 (ES19 Dye Weapon Pri)
constexpr int SVID_DYE_WEAPON_EMISSIVE= 1000;  // → equip point 20 (ES20 Dye Weapon Emi)
constexpr int SVID_DYE_JETPACK_TRAIL  = 1001;  // → equip point 21 (ES21 Jetpack Trail Dye)
constexpr int SVID_UNUSED_22          = 1002;  // → equip point 22 (ES22 ** Unused **)
constexpr int SVID_UNUSED_23          = 1003;  // → equip point 23 (ES23 ** Unused **)
constexpr int SVID_UNUSED_24          = 1004;  // → equip point 24 (ES24 ** Unused **)

// Profile IDs (asm_data_set_bots.id, used as profile selector).
constexpr uint32_t PROFILE_MEDIC    = 567;
constexpr uint32_t PROFILE_ROBOTICS = 679;
constexpr uint32_t PROFILE_ASSAULT  = 680;
constexpr uint32_t PROFILE_RECON    = 681;

struct GearSlot {
    int          device_id;       // asm_data_set_devices.device_id (= asm_data_set_items.ref_device_id)
    int          equip_slot;      // engine equip point 1..24
    int          slot_value_id;   // SVID_*
    int          quality;         // Q_* — reaches client via CRAFTED_QUALITY_VALUE_ID
    Mods::Result mods;            // rolled-mod effect_group_ids + OC flag (see ModConstants.hpp)
};

// Returns the gear list for a class. Empty list for unknown profile_id.
const std::vector<GearSlot>& GetLoadout(uint32_t profile_id);

}  // namespace Loadouts
