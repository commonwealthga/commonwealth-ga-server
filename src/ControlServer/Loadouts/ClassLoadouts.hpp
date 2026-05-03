#pragma once

#include <cstdint>
#include <vector>

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

// Equip-slot value IDs (FUN_109a1320 maps these → engine equip points 1..24).
constexpr int SVID_MELEE      = 221;  // → equip point 1
constexpr int SVID_RANGED     = 198;  // → equip point 2
constexpr int SVID_OFFHAND    = 200;  // → equip point 3
constexpr int SVID_SIDE       = 199;  // → equip point 4
constexpr int SVID_JETPACK    = 201;  // → equip point 5
constexpr int SVID_MODULE_1   = 202;  // → equip point 6
constexpr int SVID_SPECIALTY1 = 203;  // → equip point 7
constexpr int SVID_SPECIALTY2 = 204;  // → equip point 8
constexpr int SVID_HEAD_FLAIR = 385;  // → equip point 9
constexpr int SVID_AMMO       = 386;  // → equip point 10
constexpr int SVID_BAG        = 499;  // → equip point 11
constexpr int SVID_TROUSERS   = 500;  // → equip point 12
constexpr int SVID_BOOTS      = 501;  // → equip point 13
constexpr int SVID_SHIRT      = 502;  // → equip point 14
constexpr int SVID_BELT       = 823;  // → equip point 15

// Profile IDs (asm_data_set_bots.id, used as profile selector).
constexpr uint32_t PROFILE_MEDIC    = 567;
constexpr uint32_t PROFILE_ROBOTICS = 679;
constexpr uint32_t PROFILE_ASSAULT  = 680;
constexpr uint32_t PROFILE_RECON    = 681;

struct GearSlot {
    int              device_id;       // asm_data_set_devices.device_id (= asm_data_set_items.ref_device_id)
    int              equip_slot;      // engine equip point 1..24
    int              slot_value_id;   // SVID_*
    int              quality;         // Q_* — reaches client via CRAFTED_QUALITY_VALUE_ID
    std::vector<int> mods;            // rolled-mod effect_group_ids; one letter per entry
};

// Returns the gear list for a class. Empty list for unknown profile_id.
const std::vector<GearSlot>& GetLoadout(uint32_t profile_id);

}  // namespace Loadouts
