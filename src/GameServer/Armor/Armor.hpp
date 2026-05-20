// Logger channel: "armor"
#pragma once

class ATgPawn;

namespace Armor {

// Apply a hardcoded "default" armor loadout to a player pawn. Phase 1: every
// human player wears the same 7-piece set —
//   4× [rrrrrr]  (Ranged-Protection epic) → 24 stacks of prop 218
//   3× [nnnnnn]  (Health-Max-Modifier epic) → 18 stacks of prop 412 (→304)
//   7× Core mod  (every armor piece has one) → 7 stacks of +10% Health Max
// resulting in roughly +160% Max Health and a large flat Ranged-Protection
// pool. Writes directly to s_Properties.m_fRaw (the only path CalcProtection
// reads from), then fans out HEALTH_MAX via SetProperty so the client sees
// the updated cap.
//
// Tracks deltas per pawn so subsequent calls (every respawn calls Reapply
// which calls this) reverse + reapply cleanly. Safe to call repeatedly.
//
// Two-phase API. ReapplyCharacterSkillTree calls them in this order:
//   1. Armor::RevertDefaultArmor — undoes last apply's deltas FIRST, while
//      raw still matches the snapshot (no skill writes have shifted it yet).
//   2. Skill revert + skill apply (existing RCST code).
//   3. Armor::ApplyDefaultArmor — apply-only; records fresh snapshots.
//
// Splitting revert from apply is what makes snapshot checks compose across
// layers. If revert+apply both lived in ApplyDefaultArmor (as before), the
// skill apply between them would shift raw, the armor revert would mismatch
// its snapshot, and both armor + skill deltas would silently re-stack on
// every call — producing the unbounded HP creep observed when toggling the
// skill UI or respeccing.
//
// No-op for bots / non-player pawns (PRI + r_bIsBot check).
// No-op if the pawn's s_Properties array is unpopulated.
void RevertDefaultArmor(ATgPawn* Pawn);
void ApplyDefaultArmor(ATgPawn* Pawn);

}  // namespace Armor
