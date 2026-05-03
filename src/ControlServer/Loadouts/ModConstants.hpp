#pragma once

// =============================================================================
// Mod effect_group_id constants for hardcoded loadouts.
//
// Each constant is an effect_group_id from asm_data_set_effect_groups whose
// effect targets a property with a non-empty `ui_code` letter — that letter
// is what shows inside the [...] suffix on the item name.
//
// When TcpSession sends DATA_SET_INVENTORY_STATE, every effect_group_id in a
// device's mods array becomes one row → one letter on the client. So adding
// the same constant six times produces six identical letters (e.g. [hhhhhh]).
//
// The `Kits` block contains the canonical 100%-make-chance effect groups from
// shipped Weapon/Armor mod kits (item types 1434/1441). The `Unused` block
// holds props that have a `ui_code` but no shipped kit — pick whichever
// effect_group_id makes sense for experimentation; the picks below are just
// the highest-base-value representative for each prop.
// =============================================================================

namespace Mods {

// ── Shipped weapon mod kits ──────────────────────────────────────────────────

namespace Cooldown {  // 'c' Recharge Time Modifier (prop 203) — Weapon Cooldown Mod
    constexpr int UNCOMMON = 24188;
    constexpr int RARE     = 24200;
    constexpr int EPIC     = 24201;
}

namespace Damage {  // 'd' Effect Damage Modifier (prop 65) — Weapon Damage Mod
    constexpr int UNCOMMON = 24191;
    constexpr int RARE     = 24195;
    constexpr int EPIC     = 24199;
}

namespace PetDamage {  // 'd' Pet Damage Modifier (prop 350) — Weapon Pet Damage Mod
    constexpr int UNCOMMON = 25321;
    constexpr int RARE     = 25322;
    constexpr int EPIC     = 25323;
}

namespace Healing {  // 'h' Effect Healing Modifier (prop 330) — Weapon Healing Mod
    constexpr int UNCOMMON = 24208;
    constexpr int RARE     = 24211;
    constexpr int EPIC     = 24212;
}

namespace Power {  // 'p' Power Pool Cost (prop 242) — Weapon Power Mod (cheaper firing)
    constexpr int UNCOMMON = 24233;
    constexpr int RARE     = 24234;
    constexpr int EPIC     = 24230;
}

namespace Survivor {  // 'n'+'v' Health Max Deployables / Pet Max Health — Weapon Survivor Mod
    // Note: kit blueprints reuse the same effect_group_id for both props.
    constexpr int UNCOMMON = 24222;
    constexpr int RARE     = 24223;
    constexpr int EPIC     = 24219;
}

namespace Threat {  // 'T' Threat Modifier (prop 421) — Weapon Threat Mod
    constexpr int UNCOMMON = 26088;
    constexpr int RARE     = 26089;
    constexpr int EPIC     = 26090;
}

// ── Shipped armor mod kits ───────────────────────────────────────────────────

namespace ArmorBallistics {  // 'r' Protection - Ranged (prop 218) — Armor Ballistics Mod
    constexpr int UNCOMMON = 24144;
    constexpr int RARE     = 24163;
    constexpr int EPIC     = 24165;
}

namespace ArmorFlak {  // 'b' Protection - AOE (prop 219) — Armor Flak Mod
    constexpr int UNCOMMON = 24081;
    constexpr int RARE     = 23948;
    constexpr int EPIC     = 24083;
}

namespace ArmorPadding {  // 'n' Health Max Modifier (prop 412) — Armor Padding Mod
    constexpr int UNCOMMON = 24072;
    constexpr int RARE     = 24073;
    constexpr int EPIC     = 24074;
}

namespace ArmorPlate {  // 'm' Protection - Melee (prop 217) — Armor Plate Mod
    constexpr int UNCOMMON = 24141;
    constexpr int RARE     = 24168;
    constexpr int EPIC     = 24170;
}

// ── Mods that exist in DB but were never shipped as kits ─────────────────────
//
// No tier separation exists in any blueprint for these — the constants here
// are single representatives. Edit / replace at will; any effect_group_id
// whose effect's prop has the matching ui_code will render the right letter.

namespace Accuracy        { constexpr int ANY = 22312; }   // 'a' +50 accuracy (Accuracy Modifier prop 113)
namespace EffectiveRange  { constexpr int ANY = 13381; }   // 'q' Device Effective Range Modifier (prop 207)
namespace AOERadius       { constexpr int ANY = 16255; }   // 'x' AOE Radius Modifier (prop 352)
namespace HealSelf        { constexpr int ANY = 27742; }   // 'y' Effect Heal Modifier (Self) (prop 210)
namespace EffectLifetime  { constexpr int ANY = 24420; }   // 't' Effect Lifetime Modifier (prop 208) — Berserk Epic
namespace PetLifespan     { constexpr int ANY = 24601; }   // 'l' Pet LifeSpan Modifier (prop 355) — Eye Drone Epic
namespace Shield          { constexpr int ANY = 24503; }   // 's' Effect Shield Modifier (prop 386) — AOE Shield
namespace EffectRadius    { constexpr int ANY = 16255; }   // 'z' Effect Radius (prop ?) — share AOE pool
namespace FallingDamage   { constexpr int ANY = 7312;  }   // 'f' Falling Damage Modifier (prop 137)
namespace EffectPotency   { constexpr int ANY = 16795; }   // '*' Effect Potency Modifier (prop 376) — +200 (massive)
namespace ProtPhysical    { constexpr int ANY = 16601; }   // 'g' Protection - Physical (prop 155) — +20000

}  // namespace Mods
