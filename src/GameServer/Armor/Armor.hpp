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
// No-op for bots / non-player pawns (Controller class name strstr check).
// No-op if the pawn's s_Properties array is unpopulated.
void ApplyDefaultArmor(ATgPawn* Pawn);

}  // namespace Armor
