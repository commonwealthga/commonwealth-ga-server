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
// REVEAL-ON-DAMAGE: a single pulse decays on the client in ~40ms (imperceptible), so we
// HOLD the reveal for a window. Each tick in the window we pin r_fMakeVisibleIncreased to
// 1.0 (DO_REP_FORCED ships it every tick; the client folds + re-pins its own m to full).
// When the window ends we stop shipping and the client self-decays to 0 = fully cloaked.
// No delta/last-sent tracking -> no stuck-reveal (the old streaming bug). Keyed by
// r_nPawnId, not pointer, to survive UE3 address reuse.

static std::unordered_map<int, float> g_revealRemaining;  // seconds of reveal left, by r_nPawnId

void TgPawn__TickMakeVisibleCalculation::QueueRevealPulse(int pawnId, float durationSeconds) {
	float& r = g_revealRemaining[pawnId];
	if (durationSeconds > r) r = durationSeconds;   // refresh window; sustained fire re-extends it
}

void __fastcall TgPawn__TickMakeVisibleCalculation::Call(ATgPawn* Pawn, void* /*edx*/, float DeltaTime) {
	if (!Pawn) return;

	auto it = g_revealRemaining.find(Pawn->r_nPawnId);
	if (it != g_revealRemaining.end() && it->second > 0.0f && Pawn->r_bIsStealthed) {
		// Active reveal window (only while still cloaked): pin make-visible to full
		// and force-stream it every tick. If the pawn un-stealths mid-window we fall
		// through to the clear branch so no stale reveal leaks onto a re-stealth.
		it->second -= DeltaTime;
		Pawn->m_fMakeVisibleCurrent   = 1.0f;   // server copy: drops the IsStealthed gate (targetable while revealed)
		Pawn->r_fMakeVisibleIncreased = 1.0f;   // DO_REP_FORCED ships each tick; client folds + re-pins to full
		Pawn->bNetDirty = 1;
		Pawn->bForceNetUpdate = 1;
		// DIAGNOSTIC (channel "stealth"): throttled stream trace — shows reveal duration
		// and the client decay knob (r_fMakeVisibleFadeRate) we suspect causes pulsing.
		static std::unordered_map<int, int> s_n;
		int& n = s_n[Pawn->r_nPawnId];
		if ((n++ % 15) == 0)
			Logger::Log("stealth", "[stream] pawn=%d remain=%.3f incr=%.2f fadeRate=%.2f dt=%.4f\n",
				Pawn->r_nPawnId, it->second, Pawn->r_fMakeVisibleIncreased, Pawn->r_fMakeVisibleFadeRate, DeltaTime);
		if (it->second <= 0.0f) {
			g_revealRemaining.erase(it);
			Logger::Log("stealth", "[stream] pawn=%d window CLOSED\n", Pawn->r_nPawnId);
		}
		return;
	}

	// No active reveal: restore the cloaked sentinel server-side and stop streaming
	// (the client self-decays its own copy back to 0).
	if (Pawn->m_fMakeVisibleCurrent != 0.0f || Pawn->r_fMakeVisibleIncreased != 0.0f) {
		Pawn->m_fMakeVisibleCurrent   = 0.0f;
		Pawn->r_fMakeVisibleIncreased = 0.0f;
		Pawn->bNetDirty = 1;
	}
	if (it != g_revealRemaining.end()) g_revealRemaining.erase(it);
}
