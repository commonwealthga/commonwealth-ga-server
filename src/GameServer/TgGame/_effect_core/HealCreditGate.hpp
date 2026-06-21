#pragma once

#include "src/pch.hpp"
#include <unordered_set>

// One-shot suppression for heal score/morale credit when a max-HP buff
// (TgPawn__ApplyBuff, prop 412/390, device-fire source) tops a pawn off from
// a freshly-raised ceiling rather than the player's own healing.
//
// Concrete case: Adrenaline Gun fires HoT + instant heal + +400 max-HP buff,
// reordered (see TgDeviceFire__GetEffectGroup.cpp) so the max-HP buff lands
// BEFORE the instant heal — otherwise the heal clamps against the OLD max
// and is lost (db5d99e). But if the target was already at the OLD max
// (truly full) when the buff raises the ceiling, the instant heal that
// follows is just filling headroom the device itself created — not real
// healing performed by the player — so it shouldn't earn STYPE_HEALING or
// morale credit. TgEffect__TrackStats consumes the flag and zeroes
// fMissingHealth, which the existing TrackHealing / MoraleCredit::Award
// overheal clamps already turn into zero credit.
//
// Keyed by pawn pointer only (no per-device correlation): the buff and its
// follow-up heal fire back-to-back in the same device-fire callstack, so an
// unrelated heal landing in that same instant is not a realistic case.
namespace HealCreditGate {

inline std::unordered_set<ATgPawn*> g_fullBeforeMaxBuff;

inline void MarkFullBeforeMaxBuff(ATgPawn* Pawn) {
	if (Pawn) g_fullBeforeMaxBuff.insert(Pawn);
}

// Consume (clear) the flag; returns true if it was set.
inline bool ConsumeFullBeforeMaxBuff(ATgPawn* Pawn) {
	if (!Pawn) return false;
	return g_fullBeforeMaxBuff.erase(Pawn) > 0;
}

}  // namespace HealCreditGate
