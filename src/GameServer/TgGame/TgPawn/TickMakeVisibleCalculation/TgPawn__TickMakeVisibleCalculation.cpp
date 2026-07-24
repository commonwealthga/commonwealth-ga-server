#include "src/GameServer/TgGame/TgPawn/TickMakeVisibleCalculation/TgPawn__TickMakeVisibleCalculation.hpp"
#include <unordered_map>
#include <unordered_set>

// Server-side native stripped to `RET 0x4`; reimplemented as the reveal driver.
// m_fMakeVisibleCurrent is the client-local "MakeVisible" MIC scalar (0..100);
// the client folds r_fMakeVisibleIncreased into it on each rep and self-decays
// it at r_fMakeVisibleFadeRate/s. While a reveal is active (damage window or
// sensor alert bit1) we stream +10/tick — the engine's tick clamp pins client m
// at 100; when the reveal ends the client decays 100->0 (fadeRate 50/s = 2s;
// Stealth Restealth +50% ⇒ ~1.3s). Full model: reference_stealth_system.md.
// Keyed by r_nPawnId, not pointer, to survive UE3 address reuse.

static std::unordered_map<int, float> g_revealRemaining;  // seconds of reveal left, by r_nPawnId
static std::unordered_set<int> g_stealthDisabledBumped;   // pawnIds we've +1'd r_nStealthDisabled on

void TgPawn__TickMakeVisibleCalculation::QueueRevealPulse(int pawnId, float durationSeconds) {
	float& r = g_revealRemaining[pawnId];
	if (durationSeconds > r) r = durationSeconds;   // refresh window; sustained fire re-extends it
}

void __fastcall TgPawn__TickMakeVisibleCalculation::Call(ATgPawn* Pawn, void* /*edx*/, float DeltaTime) {
	if (!Pawn) return;

	// Deploy-sensor reveal: alert bit1 = "display stealthed in game"
	// (IsStealthedPlayerDisplayInGameBySensor); UC clears it on radius exit.
	const bool sensorRevealed =
		Pawn->r_bIsStealthed && (Pawn->r_nSensorAlertLevel & 2) != 0;

	auto it = g_revealRemaining.find(Pawn->r_nPawnId);
	const bool windowActive =
		it != g_revealRemaining.end() && it->second > 0.0f && Pawn->r_bIsStealthed;
	if (windowActive || sensorRevealed) {
		if (windowActive) it->second -= DeltaTime;
		Pawn->m_fMakeVisibleCurrent = 100.0f;  // server copy: drops the IsStealthed gate (targetable while revealed)

		// Clamp-pinned stream: the client fold (m += incr) is sampled at the
		// channel's delivery rate (~19/s), not per server tick, so level-holding
		// schemes drift. A fat constant crushes decay and the engine's own tick
		// clamp at 100 pins m there — delivery-rate-insensitive.
		Pawn->r_fMakeVisibleIncreased = 10.0f;  // DO_REP_FORCED ships while != 0

		// Lock gate: the m_fMakeVisibleCurrent path above only reveals via the
		// rate-fragile r_fMakeVisibleIncreased fold, so on an observer's client
		// IsStealthed() can still flip true for a frame — enough for the medic's
		// auto-lock (CheckTargetActor, TgPawn.uc:6672 `r_Target.IsStealthed()`)
		// to drop r_Target and break the PainGun beam. r_nStealthDisabled is a
		// replicated int and one of IsStealthed()'s three clauses; bumping it
		// closes the gate directly on every client. Faithful: taking damage
		// disables stealth for the reveal window (see IsStealthed decomp note).
		if (g_stealthDisabledBumped.insert(Pawn->r_nPawnId).second) {
			Pawn->r_nStealthDisabled++;
		}
		Pawn->bNetDirty = 1;
		Pawn->bForceNetUpdate = 1;
		if (windowActive && it->second <= 0.0f) g_revealRemaining.erase(it);
		return;
	}

	// Reveal ended: undo our one-time stealth-disable bump (symmetric decrement).
	auto b = g_stealthDisabledBumped.find(Pawn->r_nPawnId);
	if (b != g_stealthDisabledBumped.end()) {
		if (Pawn->r_nStealthDisabled > 0) Pawn->r_nStealthDisabled--;
		Pawn->bNetDirty = 1;
		g_stealthDisabledBumped.erase(b);
	}

	// No active reveal: restore the cloaked sentinel server-side and stop streaming
	// (the client self-decays its own copy from 100 to exactly 0 at fadeRate/s).
	if (Pawn->m_fMakeVisibleCurrent != 0.0f || Pawn->r_fMakeVisibleIncreased != 0.0f) {
		Pawn->m_fMakeVisibleCurrent   = 0.0f;
		Pawn->r_fMakeVisibleIncreased = 0.0f;
		Pawn->bNetDirty = 1;
	}
	if (it != g_revealRemaining.end()) g_revealRemaining.erase(it);
}
