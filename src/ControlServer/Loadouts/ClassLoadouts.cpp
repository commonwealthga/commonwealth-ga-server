#include "src/ControlServer/Loadouts/ClassLoadouts.hpp"
#include "src/ControlServer/Loadouts/ModConstants.hpp"

namespace Loadouts {

// =============================================================================
// Per-class gear definitions.
//
// Format per row:
//   { device_id, equip_slot, slot_value_id, quality, { mod_effect_group_ids... } }
//
// Each entry in the mods vector renders as ONE letter in the item's [...]
// suffix (whatever ui_code the effect's prop has). Repeat the same constant
// to repeat a letter — six Mods::Healing entries → [hhhhhh].
//
// device_id values come from asm_data_set_devices; names below are from
// asm_data_set_items.name_msg_translated for the matching ref_device_id.
// =============================================================================

// 567 — Medic — Life Stealer + Medic Crescent Jetpack
static const std::vector<GearSlot> kMedic = {
    { 5800,  1,  SVID_MELEE,      Q_EPIC,   {} },  // Life Stealer
    { 2991,  2,  SVID_RANGED,     Q_EPIC,   {} },  // Agonizer
    { 6898,  3,  SVID_OFFHAND,    Q_EPIC,   {} },  // Adrenaline Gun
    { 7032,  5,  SVID_JETPACK,    Q_EPIC,   {} },  // Medic Crescent Jetpack
    { 2531,  7,  SVID_SPECIALTY1, Q_EPIC,   {} },  // Healing Grenade
    { 2168,  8,  SVID_SPECIALTY2, Q_EPIC,   {} },  // Poison Grenade
    { 2773, 10,  SVID_AMMO,       Q_COMMON, {} },  // Healing Boost
    {  864, 14,  SVID_SHIRT,      Q_COMMON, {} },  // HUMAN BASE ATTRIBUTES
};

// 679 — Robotics — Mace and Shield + Robotics Crescent Jetpack
//
// Example showing the [hhhhhh] requested in the spec: an Epic Focused Repair
// Arm in the specialty slot with three Healing kit tiers stacked twice. Edit
// the mods vector to whatever you want to test.
static const std::vector<GearSlot> kRobotics = {
    { 5802,  1,  SVID_MELEE,      Q_EPIC,   {} },  // Mace and Shield
    { 6885,  2,  SVID_RANGED,     Q_EPIC,   {} },  // Colony Energy Rifle
    { 2918,  3,  SVID_OFFHAND,    Q_EPIC,   {                       // Focused Repair Arm [hhhhhh]
                                              Mods::Healing::UNCOMMON,
                                              Mods::Healing::RARE,
                                              Mods::Healing::EPIC,
                                              Mods::Healing::UNCOMMON,
                                              Mods::Healing::RARE,
                                              Mods::Healing::EPIC,
                                            } },
    { 7034,  5,  SVID_JETPACK,    Q_EPIC,   {} },  // Robotics Crescent Jetpack
    { 2300,  7,  SVID_SPECIALTY1, Q_EPIC,   {} },  // Personal Turret
    { 2066,  8,  SVID_SPECIALTY2, Q_EPIC,   {} },  // Medical Station
    { 2886, 10,  SVID_AMMO,       Q_COMMON, {} },  // Dome Shield Boost
    {  864, 14,  SVID_SHIRT,      Q_COMMON, {} },
};

// 680 — Assault — Impact Hammer + Assault Crescent Jetpack
static const std::vector<GearSlot> kAssault = {
    { 5801,  1,  SVID_MELEE,      Q_EPIC,   {} },  // Impact Hammer
    { 5788,  2,  SVID_RANGED,     Q_EPIC,   {} },  // Rhino SMG
    { 2914,  3,  SVID_OFFHAND,    Q_EPIC,   {} },  // Inferno-X Cannon
    { 7031,  5,  SVID_JETPACK,    Q_EPIC,   {} },  // Assault Crescent Jetpack
    { 3699,  7,  SVID_SPECIALTY1, Q_EPIC,   {} },  // Power Stim
    { 2498,  8,  SVID_SPECIALTY2, Q_EPIC,   {} },  // Concussion Grenade
    { 5775, 10,  SVID_AMMO,       Q_COMMON, {} },  // Super Smash Boost
    {  864, 14,  SVID_SHIRT,      Q_COMMON, {} },
};

// 681 — Recon — Dual Daggers + Recon Crescent Jetpack
static const std::vector<GearSlot> kRecon = {
    { 5799,  1,  SVID_MELEE,      Q_EPIC,   {} },  // Dual Daggers
    { 2110,  2,  SVID_RANGED,     Q_EPIC,   {} },  // Ballista
    { 3023,  3,  SVID_OFFHAND,    Q_EPIC,   {} },  // Spring Stealth
    { 7033,  5,  SVID_JETPACK,    Q_EPIC,   {} },  // Recon Crescent Jetpack
    { 2219,  7,  SVID_SPECIALTY1, Q_EPIC,   {
		Mods::AOERadius::ANY,
		Mods::AOERadius::ANY,
		Mods::AOERadius::ANY,
		Mods::AOERadius::ANY,
		Mods::AOERadius::ANY,
		Mods::AOERadius::ANY,
	} },  // EMP Bomb
    { 2129,  8,  SVID_SPECIALTY2, Q_EPIC,   {} },  // Decoy
    { 2113, 10,  SVID_AMMO,       Q_COMMON, {} },  // Shatter Bomb Boost
    {  864, 14,  SVID_SHIRT,      Q_COMMON, {} },
};

static const std::vector<GearSlot> kEmpty = {};

const std::vector<GearSlot>& GetLoadout(uint32_t profile_id) {
    switch (profile_id) {
        case PROFILE_MEDIC:    return kMedic;
        case PROFILE_ROBOTICS: return kRobotics;
        case PROFILE_ASSAULT:  return kAssault;
        case PROFILE_RECON:    return kRecon;
        default:               return kEmpty;
    }
}

}  // namespace Loadouts
