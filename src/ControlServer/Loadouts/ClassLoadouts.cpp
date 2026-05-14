#include "src/ControlServer/Loadouts/ClassLoadouts.hpp"
#include "src/ControlServer/Loadouts/ModConstants.hpp"

namespace Loadouts {

// =============================================================================
// Per-class gear definitions.
//
// Format per row:
//   { device_id, equip_slot, slot_value_id, quality, { mod_effect_group_ids... } }
//
// The mods vector is one effect_group_id per visible letter in the [...]
// suffix on the item name. Two ways to write it:
//
//   (A) Letter API — `Mods::Letters(innate, kit)`:
//       Pick the suffix you want as two strings. First arg = innate/rolled
//       letters (4–10% values, what the weapon ships with); second arg =
//       kit letters (2–3% values, applied by player crafting). Call returns
//       a std::vector<int> ready to drop in.
//
//         { 2066, 8, ..., Q_EPIC, Mods::Letters("hhx", "hhh") }     // [hhxhhh]
//         { 2066, 8, ..., Q_EPIC, Mods::Letters("xxx", "ccc") }     // [xxxccc]
//         { 2066, 8, ..., Q_EPIC, Mods::Letters("chx", "hhh") }     // [chxhhh]
//
//       Add `, /*oc=*/true` to use an Overclocked variant (+75% Output Mod
//       prepended vs the standard +70%):
//
//         { 2066, 8, ..., Q_EPIC, Mods::Letters("hhx", "hhh", true) }
//
//   (B) Raw constants — `Mods::Healing::EPIC` etc.:
//       For the rare cases where you need a specific effect_group_id (e.g.
//       a particular tier value, a different prop within a letter family).
//       See ModConstants.hpp namespaces.
//
// device_id values come from asm_data_set_devices; names below are from
// asm_data_set_items.name_msg_translated for the matching ref_device_id.
// =============================================================================

// 567 — Medic — Life Stealer + Medic Crescent Jetpack
static const std::vector<GearSlot> kMedic = {
    { 5800,  1,  SVID_MELEE,      Q_EPIC,   Mods::Letters("ddd", "hhh", false) },  // Life Stealer
    { 2991,  2,  SVID_RANGED,     Q_EPIC,   Mods::Letters("rrr", "ppp", false) },  // Agonizer
    { 2906,  3,  SVID_SPECIALTY,  Q_EPIC,   Mods::Letters("hhh", "hhh", true) },  // BFB
    { 7032,  5,  SVID_JETPACK,    Q_EPIC,   Mods::Letters("ppp", "ppp", false) },  // Medic Crescent Jetpack
    { 2531,  7,  SVID_OFFHAND1, Q_EPIC,   Mods::Letters("hhh", "hhh", true) },  // Healing Grenade
    { 2168,  8,  SVID_OFFHAND2, Q_EPIC,   Mods::Letters("xxx", "ccc", true) },  // Poison Grenade
    { 2376,  9,  SVID_OFFHAND3, Q_EPIC,   Mods::Letters("xxx", "ccc" , false)},
    { 2773, 10,  SVID_MORALE,       Q_COMMON, Mods::Letters("mmm", "hhh", false) },  // Healing Boost
    {  864, 14,  SVID_CLASS_DEVICE,      Q_COMMON, {} },  // HUMAN BASE ATTRIBUTES
};

// 679 — Robotics — Mace and Shield + Robotics Crescent Jetpack
//
// Example showing the [hhhhhh] requested in the spec: an Epic Focused Repair
// Arm in the specialty slot with three Healing kit tiers stacked twice. Edit
// the mods vector to whatever you want to test.
static const std::vector<GearSlot> kRobotics = {
    { 5802,  1,  SVID_MELEE,      Q_EPIC,   Mods::Letters("ddd", "ddd" , false)},
    { 6885,  2,  SVID_RANGED,     Q_RARE,   Mods::Letters("dd", "ddd", false )},
    { 2918,  3,  SVID_SPECIALTY,  Q_EPIC,   Mods::Letters("hhh", "hhh", true )},
    { 7034,  5,  SVID_JETPACK,    Q_EPIC,   Mods::Letters("ppp", "ppp", false)},
    { 2095,  7,  SVID_OFFHAND1, Q_EPIC,   Mods::Letters("ddd", "ddd", false )},
    { 2066,  8,  SVID_OFFHAND2, Q_EPIC,   Mods::Letters("hhx", "hhh" , false)},
    { 6143,  9,  SVID_OFFHAND3, Q_EPIC,   Mods::Letters("xxx", "ccc" , false)},
    { 2886, 10,  SVID_MORALE,       Q_COMMON, Mods::Letters("mmm", "vnvnvn", false)},
    {  864, 14,  SVID_CLASS_DEVICE,      Q_COMMON, {}},
};

// 680 — Assault — Impact Hammer + Assault Crescent Jetpack
static const std::vector<GearSlot> kAssault = {
    { 5801,  1,  SVID_MELEE,      Q_EPIC,   Mods::Letters("ddd", "ddd", true) },  // Impact Hammer
    { 5788,  2,  SVID_RANGED,     Q_EPIC,   Mods::Letters("ddd", "ddd", true) },  // Rhino SMG
    { 1987,  3,  SVID_SPECIALTY,  Q_EPIC,   Mods::Letters("ddd", "ddd", true)},  // Inferno-X Cannon
    { 7031,  5,  SVID_JETPACK,    Q_EPIC,   Mods::Letters("ppp", "ppp", false) },  // Assault Crescent Jetpack
    { 3699,  7,  SVID_OFFHAND1, Q_EPIC,   Mods::Letters("ppp", "ppp", false) },  // Power Stim
    { 2022,  8,  SVID_OFFHAND2, Q_EPIC,  Mods::Letters("xxx", "ccc", false)  },  // Concussion Grenade

    { 2013,  9,  SVID_OFFHAND3, Q_EPIC,   Mods::Letters("ttt", "ccc", false) },  
    { 5775, 10,  SVID_MORALE,       Q_COMMON, Mods::Letters("mmm", "ddd", false) },  // Super Smash Boost
    {  864, 14,  SVID_CLASS_DEVICE,      Q_COMMON, {} },
};

static const std::vector<GearSlot> kRecon = {
    { 5799,  1,  SVID_MELEE,      Q_EPIC,   Mods::Letters("ddd", "ddd", false) },  // Dual Daggers
    { 2110,  2,  SVID_RANGED,     Q_EPIC,   Mods::Letters("ddd", "ddd", true) },  // Ballista
    // { 5807,  3,  SVID_SPECIALTY,  Q_EPIC,   Mods::Letters("ppp", "ppp", false) },  // targetting system
    { 3023,  3,  SVID_SPECIALTY,  Q_EPIC,   Mods::Letters("ppp", "ppp", false) },  // spring stealth

    { 7033,  5,  SVID_JETPACK,    Q_EPIC,   Mods::Letters("ppp", "ppp", false) },  // Recon Crescent Jetpack
    { 4708,  7,  SVID_OFFHAND1, Q_EPIC,   Mods::Letters("xxx", "ddd", false) },  // Venom bomb
    { 5804,  8,  SVID_OFFHAND2, Q_EPIC,   Mods::Letters("ddx", "ddd", false) },  // Deconstructor
    { 6012,  9,  SVID_OFFHAND3, Q_EPIC,   Mods::Letters("ccc", "ccc", false) },  // vulture vision
    { 2113, 10,  SVID_MORALE,       Q_EPIC, Mods::Letters("mmm", "ddd", false) },  // Shatter Bomb Boost
    {  864, 14,  SVID_CLASS_DEVICE,      Q_COMMON, {} }, // rest device
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
