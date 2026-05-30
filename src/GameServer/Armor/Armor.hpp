// Logger channel: "armor"
#pragma once

class ATgPawn;

namespace Armor {

// Apply / revert the armor stat buffs for a player pawn, driven by the
// character's equipped armor rows in `ga_character_devices` (joined to
// `ga_players_inventory` for the per-piece mod CSV).
//
// At a high level: for each of the 7 group-126 enhancement slots the pawn
// has an equipped piece in, the pawn gains:
//   - +10% Health Max baseline                (1× ApplyBuff prop 412 cm 68)
//   - one buff per egid in the piece's mods   (e.g. 6× +0.5 raw Protection-
//                                              Ranged for a `[rrrrrr]` piece)
//
// Every ApplyBuff entry is tagged with the armor's `ga_players_inventory.id`
// as the buff's `nReqDeviceInstId`, just like rolled weapon mods — keeps the
// buff registry clean and lets us identify which entries belong to which
// piece if/when we ship a swap-variant flow.
//
// Two-phase API. ReapplyCharacterSkillTree calls them in this order:
//   1. Armor::RevertDefaultArmor — undoes last apply's deltas FIRST, before
//      skills revert. Mirrors apply order (LIFO) so snapshot checks compose.
//   2. Skill revert + skill apply (existing RCST code).
//   3. Armor::ApplyDefaultArmor — apply-only.
//
// Splitting revert from apply is what makes per-layer composition work
// across RCST re-runs (skill UI saves, respec, respawn). If revert+apply
// both lived inside ApplyDefaultArmor with skill apply between them, the
// skill apply would shift the registry between armor's revert and re-apply
// — both layers would silently re-stack and HP would creep unbounded with
// every save.
//
// No-op for bots / non-player pawns. No-op if the pawn has no equipped
// armor rows in the DB (so newly-introduced characters that predate the
// armor seed don't error out — they just fall back to bare stats until
// next login when the seed runs).
void RevertDefaultArmor(ATgPawn* Pawn);
void ApplyDefaultArmor(ATgPawn* Pawn);

}  // namespace Armor
