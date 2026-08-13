#pragma once

#include "src/pch.hpp"

// BuffWindowTracking — attributes combat that happens WHILE a tracked buff is
// active to the player + device that applied the buff. This is the "benefit"
// metric for pure-buff devices, which otherwise measure as nothing (no heal,
// no cleanse, no damage of their own).
//
//   offensive window: damage dealt BY a buffed pawn  → buffed_damage_dealt
//   defensive window: damage taken BY a buffed pawn  → protected_damage_taken
//
// Called from TgEffect__TrackStats' enemy-damage branch, after the shooter's
// own damage credit. The scan walks the pawn's s_AppliedEffectGroups for the
// group ids in kTracked below; each applied instance carries its own
// m_Instigator + source device, so attribution is per-instance:
//
//   * Caster overwrite ("B frenzies over A's frenzy") is self-resolving. All
//     tracked groups have stack_count_max=0 with replace-on-reapply rules
//     (157 Strongest-Wins measures as newest-wins in practice — 2026-08-03,
//     doc §6 — and 156 IS newest-wins), so exactly one instance of a group
//     can exist on a pawn, and it belongs to whoever cast last. Damage before
//     the overwrite credited A, damage after credits B. No double counting.
//   * The shooter's own damage row is untouched — these are separate columns
//     on the BUFF device's row, so a Frenzy-buffed Raven SMG kill counts the
//     damage on the SMG row (damage) and on Frenzy Wave's row
//     (buffed_damage_dealt) without colliding.
//
// ============================ TUNING TABLE =================================
// kTracked in the .cpp is the parameter surface: one line per effect group,
// offensive or defensive. Group ids verified against gaa.db 2026-08-04.
// Currently: Frenzy Wave 10746 (offensive, 10s) · Protection Wave 10740
// (defensive, 7s) · Protection Boost 8964 (defensive, 10s) · Group Heal
// Savior 16587 (defensive, 5s, attributed to the delivering group heal).
// ===========================================================================
namespace BuffWindowTracking {

// Record one enemy damage event on a pawn victim. `shooter` is the actual
// firing pawn (NOT pet-resolved — the buff sits on the pet/pawn that fired),
// `victim` the pawn hit, `amount` the post-mitigation magnitude.
void OnEnemyPawnDamage(ATgPawn* shooter, ATgPawn* victim, int amount);

}  // namespace BuffWindowTracking
