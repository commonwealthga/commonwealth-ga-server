#include "src/GameServer/TgGame/TgPawn/TickMakeVisibleCalculation/TgPawn__TickMakeVisibleCalculation.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include <unordered_map>

// Server-side native stripped to `RET 0x4` in the shipping binary; reimplemented here.
//
// CANONICAL MODEL (verified via ghidra 2026-06-09; see reference_stealth_system.md):
//   - The VISUAL cloak is driven by r_bIsStealthed (prop 124 / r_fStealthTransitionTime),
//     replicated. m_fMakeVisibleCurrent (+0xEFC) is a SEPARATE "MakeVisible" scalar on the
//     stealth MIC: 0 = cloaked, 1 = revealed. It is NOT replicated — the stock client owns
//     it: NativeReplicatedEvent (0x109d3450) folds r_fMakeVisibleIncreased into it, the
//     client Tick (0x109cb910) decays it at r_fMakeVisibleFadeRate (default 25/s, clamp
//     [0,1] -> ~40ms from full) and pushes it onto the MIC. ShouldUpdateStealthedFor
//     (0x109bf2c0) reveals a stealthed enemy to a viewer whenever make-visible != 0.
//   - In stock, make-visible is bumped only by client-local scanner/sensor natives;
//     r_fMakeVisibleIncreased is bNetInitial-only. There is no stock damage->make-visible
//     path, so a damage reveal must be driven by the server.
//
// REVEAL HOLD: while a reveal is active (damage window or sensor alert bit1) we stream
// +10/tick; the engine's own tick clamp pins client m at 100 ("fully visible" — the
// value the spectator-cheat branch uses). m's scale is 0..100, NOT 0..1. When the
// reveal ends we stop shipping and the client self-decays 100 -> exactly 0 at
// r_fMakeVisibleFadeRate (50/s default = 2s; Stealth Restealth +50% = ~1.3s).
// Keyed by r_nPawnId, not pointer, to survive UE3 address reuse.

static std::unordered_map<int, float> g_revealRemaining;  // seconds of reveal left, by r_nPawnId

void TgPawn__TickMakeVisibleCalculation::QueueRevealPulse(int pawnId, float durationSeconds) {
	float& r = g_revealRemaining[pawnId];
	if (durationSeconds > r) r = durationSeconds;   // refresh window; sustained fire re-extends it
}

void __fastcall TgPawn__TickMakeVisibleCalculation::Call(ATgPawn* Pawn, void* /*edx*/, float DeltaTime) {
	if (!Pawn) return;

	// Deploy-sensor partial reveal: while a sensor's short-range config row has
	// this stealthed pawn (alert bit1 = "display stealthed in game" — same bit
	// IsStealthedPlayerDisplayInGameBySensor @0x109bff70 checks), hold the
	// reveal stream. UC clears the bit when the pawn leaves the radius. The
	// retail lightup native path is dead data-side (no config carries the
	// lightup action bit), so the alert bit is the canonical detection signal.
	const bool sensorRevealed =
		Pawn->r_bIsStealthed && (Pawn->r_nSensorAlertLevel & 2) != 0;

	auto it = g_revealRemaining.find(Pawn->r_nPawnId);
	const bool windowActive =
		it != g_revealRemaining.end() && it->second > 0.0f && Pawn->r_bIsStealthed;
	if (windowActive || sensorRevealed) {
		// Active reveal (timed window or live sensor detection, only while still
		// cloaked): pin make-visible to full and force-stream it every tick. If
		// the pawn un-stealths mid-reveal we fall through to the clear branch so
		// no stale reveal leaks onto a re-stealth.
		if (windowActive) it->second -= DeltaTime;
		Pawn->m_fMakeVisibleCurrent = 100.0f;  // server copy: drops the IsStealthed gate (targetable while revealed)

		// Clamp-pinned stream. The client fold (m += incr) is additive and only
		// SAMPLED at the channel's delivery rate (~19/s observed vs 30 server
		// ticks — per-tick writes overwrite each other), so level-holding
		// schemes drift. Instead ship a fat constant: inflow (~19/s × 10)
		// crushes decay (≤75/s) and the ENGINE's own tick clamp at 100
		// (DAT_11690c58; the spectator-reveal value) pins m there — bounded,
		// delivery-rate-insensitive. Wear-off after the reveal = 100/fadeRate.
		Pawn->r_fMakeVisibleIncreased = 10.0f;  // DO_REP_FORCED ships while != 0
		Pawn->bNetDirty = 1;
		Pawn->bForceNetUpdate = 1;
		// DIAGNOSTIC (channel "stealth"): throttled stream trace.
		static std::unordered_map<int, int> s_n;
		int& n = s_n[Pawn->r_nPawnId];
		if ((n++ % 15) == 0)
			Logger::Log("stealth", "[stream] pawn=%d window=%.3f sensorBit=%d incr=%.1f fadeRate=%.2f dt=%.4f\n",
				Pawn->r_nPawnId, windowActive ? it->second : 0.0f, (int)sensorRevealed,
				Pawn->r_fMakeVisibleIncreased, Pawn->r_fMakeVisibleFadeRate, DeltaTime);
		if (windowActive && it->second <= 0.0f) {
			g_revealRemaining.erase(it);
			Logger::Log("stealth", "[stream] pawn=%d window CLOSED\n", Pawn->r_nPawnId);
		}
		return;
	}

	// No active reveal: restore the cloaked sentinel server-side and stop streaming
	// (the client self-decays its own copy from ~1.0 to exactly 0 in 1/fadeRate s).
	if (Pawn->m_fMakeVisibleCurrent != 0.0f || Pawn->r_fMakeVisibleIncreased != 0.0f) {
		Pawn->m_fMakeVisibleCurrent   = 0.0f;
		Pawn->r_fMakeVisibleIncreased = 0.0f;
		Pawn->bNetDirty = 1;
	}
	if (it != g_revealRemaining.end()) g_revealRemaining.erase(it);
}
