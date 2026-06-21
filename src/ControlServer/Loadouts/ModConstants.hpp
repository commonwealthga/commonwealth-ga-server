#pragma once

#include <string>
#include <vector>

// =============================================================================
// Mod effect_group_id helpers for hardcoded loadouts.
//
// Each effect_group_id in a device's mods vector becomes ONE entry in the
// inventory state blob and renders as ONE letter in the item's [...] suffix
// (using the effect's prop's ui_code; props without one are gameplay-meaningful
// but render no letter).
//
// PRIMARY API — `Mods::Letters(innate, kit)`:
//   Pass two strings; each character names a letter to appear in the suffix.
//   Letter → effect_group_id resolution happens at Resync time using the
//   device's own blueprint pool (asm_data_set_blueprint_item_mods), so the
//   same letter on different devices can resolve to different props (e.g.
//   'd' on a rocket turret picks Pet Damage; 'd' on a melee weapon picks
//   Melee Damage; 'd' on a regular weapon picks Effect Damage). The actual
//   resolution lives in ModResolver.cpp.
//
//   First arg is the "rolled / innate" half (4–10% per letter — picked from
//   the device's own innate pool); second arg is the "kit" half (2–3% per
//   letter — generic kit egid keyed by the letter's prop). Letters that have
//   no entry in the device's pool (innate) or no kit egid for the derived
//   prop (kit) are silently dropped.
//
//   Multi-letter kit pair: 'v' and 'n' written adjacently ("vn" or "nv") in
//   the kit string emit ONE Survivor kit egid which renders as both letters.
//   A lone 'v' (no adjacent 'n') is dropped because no single-letter 'v' kit
//   exists; a lone 'n' falls back to Padding (HP Max) when the device's pool
//   maps 'n' to prop 412.
//
//     // Medical Station with [hhxhhh] — 'hhx' innate, 'hhh' kit:
//     { 2066, 8, ..., Q_EPIC, Mods::Letters("hhx", "hhh") }
//
//     // Same slot, different mods → [xxxccc]:
//     { 2066, 8, ..., Q_EPIC, Mods::Letters("xxx", "ccc") }
//
//     // Innate only → [hhh]:    Mods::Letters("hhh", "")
//     // Kit only → [hhh]:       Mods::Letters("", "hhh")
//
//   OC overload (third bool arg, default false):
//     { 2918, 3, ..., Q_EPIC, Mods::Letters("hhh", "hhh", /*oc=*/true) }
//   Marks the slot as Overclocked. The OC flag is purely cosmetic / metadata:
//   we send a different blueprint_id (one with override_name_msg_id != 0,
//   e.g. 6424 for FRA → "Focused Repair Arm OC") and the client renders the
//   "OC" name suffix. Not every device has an OC variant — see
//   asm_data_set_blueprints rows where override_name_msg_id != 0 (77 rows
//   covering the canon Mercenary OC weapons + a handful of grenades).
//   Devices without an OC variant silently fall back to their standard
//   blueprint when oc=true. NO gameplay buff is implied — the item's actual
//   stats come from the rolled mods + device base data, exactly as without
//   the flag.
//
// LETTERS BY PROP (canonical mapping — actual letter ⇄ prop binding for any
// specific device comes from that device's blueprint pool, not this table):
//
//   Letter | Common prop_ids (which one applies depends on device)
//   -------+----------------------------------------------------------
//   d      | 65 Effect Damage / 212 Melee / 214 Ranged / 350 Pet Damage
//   h      | 330 Effect Healing
//   x      | 352 AOE Radius
//   r      | 114 Device Range / 218 Protection-Ranged / 381 Pet Range
//   c      | 203 Recharge Time
//   p      | 242 Power Pool Cost
//   s      | 386 Effect Shield
//   t      | 208 Effect Lifetime
//   v      | 366 Pet Max Health
//   n      | 339 HP Max Deployables / 412 Health Max (armor)
//   m      | 217 Protection-Melee
//   b      | 219 Protection-AOE
//   T      | 421 Threat
//
// Multi-letter Survivor kit (egid 24219 etc): one egid produces both 'v'
// and 'n' visible. Write "vn" or "nv" in the kit string to invoke it.
// =============================================================================

namespace Mods {

// Output of `Letters()` and the type stored in `GearSlot::mods`. Carries
// either letter strings (resolved at Resync time per-device) OR a raw egid
// list (back-compat path used by rows that pass concrete `Mods::*::EPIC`
// constants directly). The `oc` flag picks an Overclocked-named blueprint
// at TCP-send time. Constructors keep the existing brace-list patterns in
// ClassLoadouts.cpp working without per-row migration:
//
//     Q_EPIC, {}                                    // empty Result
//     Q_EPIC, { Mods::Damage::EPIC, ... }           // raw egids (bypass letter resolution)
//     Q_EPIC, Mods::Letters("hhh", "hhh")           // letters, device-resolved
//     Q_EPIC, Mods::Letters("hhh", "hhh", true)     // letters + OC name
//
// Raw `raw_egids` non-empty (via Mods::Egids(...) or a brace list) bypasses
// letter resolution — useful when you want specific egids the resolver
// wouldn't pick. The device Output Mod (prop 385) is still prepended.
struct Result {
    std::vector<int> raw_egids;  // explicit egids; bypasses letter resolution
    std::string      innate;     // letters → device-specific innate egids
    std::string      kit;        // letters → kit egids (prop derived from device pool)
    bool             oc = false;

    Result() = default;
    Result(std::initializer_list<int> il) : raw_egids(il) {}

    bool empty() const { return raw_egids.empty() && innate.empty() && kit.empty(); }
};

// ── Shipped weapon mod kits ──────────────────────────────────────────────────
// Three constants per family — UNCOMMON/RARE/EPIC — represent the three rolls
// from a shipped Mod Kit blueprint. Per-tier values are typically identical
// inside any one family (the "tier" controls how many letters the kit adds,
// not how much each one is worth).

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

namespace Power {  // 'p' Power Pool Cost (prop 242) — Weapon Power Mod
    constexpr int UNCOMMON = 24233;
    constexpr int RARE     = 24234;
    constexpr int EPIC     = 24230;
}

namespace Survivor {  // 'n'+'v' HP Max Deployables / Pet Max Health — Weapon Survivor Mod
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

// Build a Result from compact letter strings. Resolution is deferred to
// Resync time (ModResolver::Resolve) so the same letter can map to different
// props on different devices.
inline Result Letters(const char* innate, const char* kit, bool oc = false) {
    Result r;
    if (innate) r.innate = innate;
    if (kit)    r.kit    = kit;
    r.oc = oc;
    return r;
}

// Manual escape hatch — pin an EXACT egid list, bypassing letter resolution.
// Use for oddball devices whose own blueprint pool lacks the entries the
// resolver needs, so Mods::Letters() silently drops letters:
//   - innate letters need the letter present AT the requested quality
//     (most devices only ship innate rolls at RARE/UNCOMMON, not EPIC);
//   - kit letters need the letter present at ANY quality just to derive the
//     prop (even though the kit egid itself is generic & device-independent).
//
// Each egid → one inventory entry → one suffix letter (the effect's prop
// ui_code). The device Output Mod (prop 385) is still prepended, exactly as
// for every Letters() row, so base output matches. Same `oc` flag semantics.
//
// Reuse the named kit constants below for the kit half; pull innate egids
// straight from the device's pool, e.g.:
//   SELECT p.ui_code, bim.quality_value_id, bmg.effect_group_id
//   FROM asm_data_set_blueprints b
//   JOIN asm_data_set_blueprint_item_mods bim ON bim.blueprint_id=b.blueprint_id
//   JOIN asm_data_set_blueprint_mod_effect_groups bmg ON bmg.blueprint_mod_id=bim.blueprint_mod_id
//   JOIN asm_data_set_effects e ON e.effect_group_id=bmg.effect_group_id
//   JOIN asm_data_set_properties p ON p.prop_id=e.prop_id
//   WHERE p.ui_code!='' AND b.created_item_id=<device_id>;
//
//   // Adrenaline Gun [hhhhhh] — pool has no EPIC innate 'h', so
//   // Letters("hhh","hhh") dropped the 3 innate; pin them explicitly:
//   { 7457, 3, SVID_SPECIALTY, Q_EPIC, Mods::Egids({
//        27486, 27486, 27486,                                      // 3 innate 'h' (device pool, RARE roll)
//        Mods::Healing::EPIC, Mods::Healing::EPIC, Mods::Healing::EPIC }) },  // 3 kit 'h'
inline Result Egids(std::initializer_list<int> egids, bool oc = false) {
    Result r;
    r.raw_egids = egids;
    r.oc = oc;
    return r;
}

// ── Single-tier "Any" picks (props with a ui_code but no shipped kit) ────────
// Useful when you want raw egid access. The Letters() table above already
// picks representatives; reach for these only when you specifically need
// a non-default egid.

namespace Accuracy        { constexpr int ANY = 22312; }   // 'a' +50 accuracy (Accuracy Modifier prop 113)
namespace EffectiveRange  { constexpr int ANY = 13381; }   // 'q' Device Effective Range Modifier (prop 207)
namespace AOERadius       { constexpr int ANY = 16255; }   // 'x' AOE Radius Modifier (prop 352)
namespace HealSelf        { constexpr int ANY = 27742; }   // 'y' Effect Heal Modifier (Self) (prop 210)
namespace EffectLifetime  { constexpr int ANY = 24420; }   // 't' Effect Lifetime Modifier (prop 208) — Berserk Epic
namespace PetLifespan     { constexpr int ANY = 24601; }   // 'l' Pet LifeSpan Modifier (prop 355) — Eye Drone Epic
namespace Shield          { constexpr int ANY = 24503; }   // 's' Effect Shield Modifier (prop 386) — AOE Shield
namespace EffectRadius    { constexpr int ANY = 16255; }   // 'z' Effect Radius — share AOE pool
namespace FallingDamage   { constexpr int ANY = 7312;  }   // 'f' Falling Damage Modifier (prop 137)
namespace EffectPotency   { constexpr int ANY = 16795; }   // '*' Effect Potency Modifier (prop 376) — +200 (massive)
namespace ProtPhysical    { constexpr int ANY = 16601; }   // 'g' Protection - Physical (prop 155) — +20000

}  // namespace Mods
