#include "src/GameServer/TgGame/TgPawn_Character/ReapplyLoadoutEffects/TgPawn_Character__ReapplyLoadoutEffects.hpp"
#include "src/GameServer/TgGame/TgEffectManager/RemoveAllEffects/TgEffectManager__RemoveAllEffects.hpp"
#include "src/Utils/Logger/Logger.hpp"

// UC declares `native function ReapplyLoadoutEffects();` on TgPawn and
// TgPawn_Character; binary symbol at 0x109c0e10 is the stripped stub
// `TgPawn_Character__ReapplyLoadoutEffects_notimplemented` (returns nothing).
// Two UC call sites consume this:
//
//   1. `TgPawn.uc:10269` — `Dying.EndState`. Fires when a player leaves the
//      Dying state without being destroyed (medic revive, self-revive, etc.).
//      The same EndState block first does `ClearTimer('RemoveAllEffects')` —
//      the deferred death-time cleanup scheduled at TgPawn.uc:4456 — so by
//      the time ReapplyLoadoutEffects runs, the broad RemoveAllEffects sweep
//      has been cancelled and the pawn still carries every lifetime=0 buff
//      that was active at the moment of death.
//
//   2. `TgGame_Arena.uc:398` — round-restart loop. Pawn is teleported to a
//      fresh spawn point, HP/Power topped up, made invulnerable for 3s. Stale
//      transient buffs from the previous round should be cleared.
//
// The bug this fixes: with the stub being a no-op, lifetime=0 held-buff
// effects that were active at death stay applied to the revived pawn's
// `s_Properties[*].m_fRaw` AND its engine fields (Pawn.GroundSpeed,
// Pawn.AirSpeed, Pawn.r_FlightAcceleration) forever. Concrete symptoms the
// user hit:
//
//   - iMINIGUN right-click root (egId 9350): UC TgEffect.ApplyEffect special-
//     cases prop 338 → `ApplyToProperty(Target, 49, 10000)` with calc=70 →
//     `Pawn.GroundSpeed` clamped to 0. If the player is downed and revived
//     while holding RMB, the root stays applied through revive. Releasing
//     RMB later tries to remove the clone, but the clone was already swept
//     out of `s_AppliedEffectGroups` by our pre-fix RemoveAllEffects (which
//     removed-without-reversing for lifetime=0). Engine.GroundSpeed stays
//     at 0. Player can't move.
//
//   - Jetpack-fire (egIds 10450/52/54/56): prop 70 +45 AirSpeed, prop 59
//     +0.02 FlightAccel. Each death+revive cycle while jetpacking leaks
//     these deltas onto m_fRaw. After several cycles AirSpeed compounds
//     into the multiples-of-baseline range and physics gets weird.
//
// Fix: call our `TgEffectManager__RemoveAllEffects` which now correctly
// reverses all lifetime=0 non-aoi (held-buff) clones before swap-removing
// them from `s_AppliedEffectGroups`. Skill effects and equip effects don't
// live in s_AppliedEffectGroups (they're direct s_Properties.m_fRaw writes
// via `ReapplyCharacterSkillTree` and `Inventory::ApplyDeviceEquipEffects`),
// so they stay intact across the cleanup — the revived pawn keeps its
// skill+equip baseline, just sheds the fire-cycle / scope / jetpack-style
// transient buffs that were active at the moment of death.
//
// CallOriginal is intentionally NOT called: the original at 0x109c0e10 is
// the stripped `_notimplemented` stub and does nothing.
void __fastcall TgPawn_Character__ReapplyLoadoutEffects::Call(ATgPawn_Character* Pawn, void* /*edx*/) {
	LogCallBegin();
	if (!Pawn) { LogCallEnd(); return; }

	ATgEffectManager* Mgr = Pawn->r_EffectManager;
	if (Mgr) {
		Logger::Log("effects",
			"[REAPPLY-LOADOUT] pawn=%p clearing transient effects (applied=%d)\n",
			(void*)Pawn, Mgr->s_AppliedEffectGroups.Count);
		TgEffectManager__RemoveAllEffects::Call(Mgr, nullptr, nullptr);
	}

	LogCallEnd();
}
