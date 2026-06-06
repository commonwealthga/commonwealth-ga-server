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
    { 5800,  1,  SVID_MELEE,      Q_EPIC,   Mods::Letters("ddd", "ddd", false) },  // Life Stealer
    { 3967,  1,  SVID_MELEE,      Q_EPIC,   Mods::Letters("ddd", "ddd", false) },  // Poison injector

    { 2991,  2,  SVID_RANGED,     Q_EPIC,   Mods::Letters("rrr", "ppp", false) },  // Agonizer
    { 4154,  2,  SVID_RANGED,     Q_EPIC,   Mods::Letters("ddd", "ddd", true) },  // Euthanizer
    { 5797,  2,  SVID_RANGED,     Q_EPIC,   Mods::Letters("ddd", "ddd", true) },  // Rockwind
    { 4676,  2,  SVID_RANGED,     Q_EPIC,   Mods::Letters("ppp", "ppp", false) },  // Paingun
    { 6884,  2,  SVID_RANGED,     Q_RARE,   Mods::Letters("dd", "ddd", false) },  // Legion side arm


    { 2906,  3,  SVID_SPECIALTY,  Q_EPIC,   Mods::Letters("hhh", "hhh", true) },  // BFB
    { 7457,  3,  SVID_SPECIALTY,  Q_EPIC,   Mods::Letters("hhh", "hhh", false) },  // adren
    { 3946,  3,  SVID_SPECIALTY,  Q_EPIC,   Mods::Letters("hhh", "hhh", true) },  // boost beam
    { 6004,  3,  SVID_SPECIALTY,  Q_EPIC,   Mods::Letters("hhh", "hhh", true) },  // multiboost
    { 5064,  3,  SVID_SPECIALTY,  Q_EPIC,   Mods::Letters("hhh", "hhh", true) },  // nanite enhance
    { 2061,  3,  SVID_SPECIALTY,  Q_EPIC,   Mods::Letters("hhh", "hhh", true) },  // nanite restore

    { 7032,  5,  SVID_JETPACK,    Q_EPIC,   Mods::Letters("ppp", "ppp", false) },  // Medic Crescent Jetpack

    { 2531,  7,  SVID_OFFHAND1, Q_EPIC,   Mods::Letters("hhh", "hhh", true) },  // Healing Grenade
    { 2168,  8,  SVID_OFFHAND2, Q_EPIC,   Mods::Letters("xxx", "ccc", true) },  // Poison Grenade
    { 3645,  8,  SVID_OFFHAND2, Q_EPIC,   Mods::Letters("xxx", "ccc", false) },  // frenzy
    { 3642,  8,  SVID_OFFHAND2, Q_EPIC,   Mods::Letters("xxx", "ccc", false) },  // neut
    { 2379,  8,  SVID_OFFHAND2, Q_EPIC,   Mods::Letters("xxx", "ccc", false) },  // psn aura
    { 4682,  8,  SVID_OFFHAND2, Q_EPIC,   Mods::Letters("xxx", "ccc", false) },  // power wave
    { 4690,  8,  SVID_OFFHAND2, Q_EPIC,   Mods::Letters("xxx", "ccc", false) },  // powervirus
    { 3639,  8,  SVID_OFFHAND2, Q_EPIC,   Mods::Letters("xxx", "ccc", false) },  // prot wave
    { 2246,  8,  SVID_OFFHAND2, Q_EPIC,   Mods::Letters("hhh", "hhh", false) },  // regen
    { 6006,  8,  SVID_OFFHAND2, Q_EPIC,   Mods::Letters("xxx", "ccc", false) },  // soul stealer
    { 5808,  8,  SVID_OFFHAND2, Q_EPIC,   Mods::Letters("xxx", "ccc", false) },  // triage
    { 2376,  9,  SVID_OFFHAND3, Q_EPIC,   Mods::Letters("xxx", "ccc" , false)}, // healwave

    { 2773, 10,  SVID_MORALE,       Q_EPIC, Mods::Letters("mmm", "hhh", false) },  // Healing Boost
    { 5776, 10,  SVID_MORALE,       Q_EPIC, Mods::Letters("mmm", "ddd", false) },  // Oathbreaker
    {  864, 14,  SVID_CLASS_DEVICE,      Q_COMMON, {} },  // HUMAN BASE ATTRIBUTES
};

// 679 — Robotics — Mace and Shield + Robotics Crescent Jetpack
//
// Example showing the [hhhhhh] requested in the spec: an Epic Focused Repair
// Arm in the specialty slot with three Healing kit tiers stacked twice. Edit
// the mods vector to whatever you want to test.
static const std::vector<GearSlot> kRobotics = {
    { 5802,  1,  SVID_MELEE,      Q_EPIC,   Mods::Letters("ddd", "ddd" , false)}, // mace and shield
    { 3951,  1,  SVID_MELEE,      Q_EPIC,   Mods::Letters("ddd", "ddd" , false)}, // energyburn mace

    { 6885,  2,  SVID_RANGED,     Q_RARE,   Mods::Letters("dd", "ddd", false )}, // colony rifle
    { 4158,  2,  SVID_RANGED,     Q_EPIC,   Mods::Letters("ddd", "ddd", true )}, // hel-tac
    { 5798,  2,  SVID_RANGED,     Q_EPIC,   Mods::Letters("ddd", "ddd", true )}, // harken
    { 3523,  2,  SVID_RANGED,     Q_EPIC,   Mods::Letters("ddd", "ddd", true )}, // rumbleblaster
    { 6897,  2,  SVID_RANGED,     Q_EPIC,   Mods::Letters("ddd", "ddd", false )}, // techno blaster
    { 4192,  2,  SVID_RANGED,     Q_EPIC,   Mods::Letters("ddd", "ddd", true )}, // tempest


    { 2918,  3,  SVID_SPECIALTY,  Q_EPIC,   Mods::Letters("hhh", "hhh", true )}, // focused repair arm
    { 2046,  3,  SVID_SPECIALTY,  Q_EPIC,   Mods::Letters("hhh", "hhh", true )}, // ARC
    { 5810,  3,  SVID_SPECIALTY,  Q_EPIC,   Mods::Letters("hhh", "hhh", true )}, // Nanite repair
    { 5811,  3,  SVID_SPECIALTY,  Q_EPIC,   Mods::Letters("ddp", "ppp", false )}, // Force target

    { 7034,  5,  SVID_JETPACK,    Q_EPIC,   Mods::Letters("ppp", "ppp", false)},

    { 2095,  7,  SVID_OFFHAND1, Q_EPIC,   Mods::Letters("ddd", "ddd", false )}, // rocket turret
    { 3755,  7,  SVID_OFFHAND1, Q_EPIC,   Mods::Letters("ddd", "ddd", false )}, // auto cannon
    { 2675,  7,  SVID_OFFHAND1, Q_EPIC,   Mods::Letters("ddd", "ddd", false )}, // eye drone
    { 5792,  7,  SVID_OFFHAND1, Q_EPIC,   Mods::Letters("ddr", "ddd", false )}, // flame turret
    { 2051,  7,  SVID_OFFHAND1, Q_EPIC,   Mods::Letters("ccc", "ccc", false )}, // force wall
    { 2107,  7,  SVID_OFFHAND1, Q_EPIC,   Mods::Letters("ddd", "ddd", true )}, // grizzly drone
    { 4782,  7,  SVID_OFFHAND1, Q_EPIC,   Mods::Letters("ddd", "ddd", false )}, // harrier drone
    { 2279,  7,  SVID_OFFHAND1, Q_EPIC,   Mods::Letters("ddd", "ddd", false )}, // hornet drone
    { 4698,  7,  SVID_OFFHAND1, Q_EPIC,   Mods::Letters("ddd", "ddd", false )}, // lockdown drone
    { 2300,  7,  SVID_OFFHAND1, Q_EPIC,   Mods::Letters("ddd", "ddd", false )}, // personal turret
    { 4076,  7,  SVID_OFFHAND1, Q_EPIC,   Mods::Letters("xxx", "ccc", false )}, // power station
    { 2326,  7,  SVID_OFFHAND1, Q_EPIC,   Mods::Letters("xxx", "ccc", false )}, // sensor
    { 2066,  8,  SVID_OFFHAND2, Q_EPIC,   Mods::Letters("hhh", "hhh" , false)}, // medical station
    // { 6143,  9,  SVID_OFFHAND3, Q_EPIC,   Mods::Letters("xxx", "ccc" , false)}, // buff station

    { 2886, 10,  SVID_MORALE,       Q_EPIC, Mods::Letters("mmm", "vnvnvn", false)},
    {  864, 14,  SVID_CLASS_DEVICE,      Q_COMMON, {}},
};

// 680 — Assault — Impact Hammer + Assault Crescent Jetpack
static const std::vector<GearSlot> kAssault = {
    { 5801,  1,  SVID_MELEE,      Q_EPIC,   Mods::Letters("ddd", "ddd", false) },  // Impact Hammer
    { 6806,  1,  SVID_MELEE,      Q_RARE,   Mods::Letters("dd", "ppp", false) },  // beatstick
    { 3973,  1,  SVID_MELEE,      Q_EPIC,   Mods::Letters("ddd", "ddd", false) },  // axe

    { 5788,  2,  SVID_RANGED,     Q_EPIC,   Mods::Letters("ddd", "ddd", true) },  // Rhino SMG
    { 6882,  2,  SVID_RANGED,     Q_RARE,   Mods::Letters("dd", "ddd", false) },  // Legion SMG
    { 4166,  2,  SVID_RANGED,     Q_EPIC,   Mods::Letters("ddd", "ddd", true) },  // heatwrack
    { 4196,  2,  SVID_RANGED,     Q_EPIC,   Mods::Letters("ddd", "ddd", true) },  // stormer

    { 1987,  3,  SVID_SPECIALTY,  Q_EPIC,   Mods::Letters("ddd", "ddd", true)},  // imini
    { 2790,  3,  SVID_SPECIALTY,  Q_EPIC,   Mods::Letters("xxx", "ddd", false)},  // gammaburst
    { 1991,  3,  SVID_SPECIALTY,  Q_EPIC,   Mods::Letters("ddd", "ddd", true)},  // headhunter
    { 6896,  3,  SVID_SPECIALTY,  Q_EPIC,   Mods::Letters("ddd", "ddd", false)},  // helot mini
    { 2914,  3,  SVID_SPECIALTY,  Q_EPIC,   Mods::Letters("ddd", "ddd", true)},  // inferno
    { 5789,  3,  SVID_SPECIALTY,  Q_EPIC,   Mods::Letters("ddd", "ddd", true)},  // longbow
    { 3695,  3,  SVID_SPECIALTY,  Q_EPIC,   Mods::Letters("xxx", "ddd", false)},  // magmalance
    { 1994,  3,  SVID_SPECIALTY,  Q_EPIC,   Mods::Letters("xxx", "ddd", false)},  // tremor

    { 7031,  5,  SVID_JETPACK,    Q_EPIC,   Mods::Letters("ppp", "ppp", false) },  // Assault Crescent Jetpack

    { 3699,  7,  SVID_OFFHAND1, Q_EPIC,   Mods::Letters("ccc", "ccc", false) },  // Power Stim
    { 2022,  8,  SVID_OFFHAND2, Q_EPIC,  Mods::Letters("xxx", "ccc", false)  },  // emp
    { 2013,  9,  SVID_OFFHAND3, Q_EPIC,   Mods::Letters("ttt", "ccc", false) }, // range shield
    { 2004,  9,  SVID_OFFHAND3, Q_EPIC,   Mods::Letters("ttt", "ccc", false) }, // aoe shield
    { 5809,  9,  SVID_OFFHAND3, Q_EPIC,   Mods::Letters("ttc", "ccc", false) }, // berserk
    { 2498,  9,  SVID_OFFHAND3, Q_EPIC,   Mods::Letters("ddd", "ddd", true) }, // conc
    { 2019,  9,  SVID_OFFHAND3, Q_EPIC,   Mods::Letters("ddd", "ddd", false) }, // incen
    { 5806,  9,  SVID_OFFHAND3, Q_EPIC,   Mods::Letters("ddx", "ddd", false) }, // overcharge
    { 3708,  9,  SVID_OFFHAND3, Q_EPIC,   Mods::Letters("ttt", "ccc", false) }, // perfect target

    { 2838, 10,  SVID_MORALE,       Q_EPIC, Mods::Letters("mmm", "", false) },  // prot boost
    { 5775, 10,  SVID_MORALE,       Q_EPIC, Mods::Letters("mmm", "ddd", false) },  // Super Smash Boost

    {  864, 14,  SVID_CLASS_DEVICE,      Q_COMMON, {} },
};

static const std::vector<GearSlot> kRecon = {
    { 5799,  1,  SVID_MELEE,      Q_EPIC,   Mods::Letters("ddd", "ddd", false) },  // Dual Daggers
    { 6895,  1,  SVID_MELEE,      Q_EPIC,   Mods::Letters("ddd", "ddd", false) },  // assassin blade
    { 3970,  1,  SVID_MELEE,      Q_EPIC,   Mods::Letters("ddd", "ddd", false) },  // ghost sword

    { 2110,  2,  SVID_RANGED,     Q_EPIC,   Mods::Letters("ddd", "ddd", true) },  // Ballista
    { 6883,  2,  SVID_RANGED,     Q_RARE,   Mods::Letters("dd", "ddd", false) },  // dweller
    { 6069,  2,  SVID_RANGED,     Q_EPIC,   Mods::Letters("ddd", "ddd", true) },  // raven
    { 4946,  2,  SVID_RANGED,     Q_EPIC,   Mods::Letters("ddd", "ddd", true) },  // rogue
    { 3249,  2,  SVID_RANGED,     Q_EPIC,   Mods::Letters("ddd", "ddd", true) },  // scorpia

    { 5807,  3,  SVID_SPECIALTY,  Q_EPIC,   Mods::Letters("ppp", "ppp", false) },  // targetting system
    { 3023,  3,  SVID_SPECIALTY,  Q_EPIC,   Mods::Letters("ppp", "ppp", false) },  // spring stealth
    { 2209,  3,  SVID_SPECIALTY,  Q_EPIC,   Mods::Letters("ppp", "ppp", false) },  // sprint stealth

    { 7033,  5,  SVID_JETPACK,    Q_EPIC,   Mods::Letters("ppp", "ppp", false) },  // Recon Crescent Jetpack

    { 4708,  7,  SVID_OFFHAND1, Q_EPIC,   Mods::Letters("xxx", "ccc", false) },  // Venom bomb
    { 2219,  7,  SVID_OFFHAND1, Q_EPIC,   Mods::Letters("xxx", "ccc", false) },  // emp bomb
    { 3056,  7,  SVID_OFFHAND1, Q_EPIC,   Mods::Letters("xxx", "ccc", false) },  // fire bomb
    { 4716,  7,  SVID_OFFHAND1, Q_EPIC,   Mods::Letters("xxx", "ccc", false) },  // graviton bomb
    { 5804,  8,  SVID_OFFHAND2, Q_EPIC,   Mods::Letters("ddx", "ddd", false) },  // Deconstructor
    { 6012,  9,  SVID_OFFHAND3, Q_EPIC,   Mods::Letters("ccc", "ccc", false) },  // vulture vision
    { 2368,  9,  SVID_OFFHAND3, Q_EPIC,   Mods::Letters("ccc", "ccc", false) },  // bionics
    { 2129,  9,  SVID_OFFHAND3, Q_EPIC,   Mods::Letters("ccc", "ccc", false) },  // decoy
    { 2953,  9,  SVID_OFFHAND3, Q_EPIC,   Mods::Letters("ccc", "ccc", false) },  // melee stim
    { 2218,  9,  SVID_OFFHAND3, Q_EPIC,   Mods::Letters("ccc", "ccc", false) },  // range stim
    { 3704,  9,  SVID_OFFHAND3, Q_EPIC,   Mods::Letters("ccc", "ccc", false) },  // sealed system
    { 2176,  9,  SVID_OFFHAND3, Q_EPIC,   Mods::Letters("ccc", "ccc", false) },  // visual scanner
    { 2225,  9,  SVID_OFFHAND3, Q_EPIC,   Mods::Letters("ddd", "ddd", true) },  // standard mine
    { 2897,  9,  SVID_OFFHAND3, Q_EPIC,   Mods::Letters("ddd", "ddd", false) },  // poison mine

    { 2113, 10,  SVID_MORALE,       Q_EPIC, Mods::Letters("mmm", "ddd", false) },  // Shatter Bomb Boost
    { 2846, 10,  SVID_MORALE,       Q_EPIC, Mods::Letters("mmm", "", false) },  // sensor boost
    { 7559, 10,  SVID_MORALE,       Q_EPIC, Mods::Letters("mmm", "", false) },  // fashion boost

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
