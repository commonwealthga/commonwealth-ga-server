#pragma once

#include "src/pch.hpp"

// Mobile Grinder (bot 1428, TgPawn_NPC, body_asm_id 2110) loses its walk cycle
// after a stun — it slides with a frozen skeleton. Root cause (reproduced on a
// healthy grinder, log 597): the robot anim tree breaks its posture-0 walk node
// whenever it passes through posture TG_POSTURE_CRITICALFAILURE (11) and back to
// DEFAULT (0). The stun effect drives this chassis into CRITICALFAILURE. Fix:
// remap posture 11 -> TG_POSTURE_STUNNED (5) for the grinder — the stun posture
// other NPCs use, which its tree handles without poisoning the walk. r_ePosture
// is delta-replicated, so this reaches the vanilla client. (Pinning to DEFAULT 0
// also works and is the proven fallback if STUNNED misbehaves.)
namespace GrinderWalkFix {
	// Identity: r_nBodyMeshAsmId == 2110 (works on the headless server, unique to
	// the grinder). Keeps the fix strictly scoped.
	bool IsMobileGrinder(ATgPawn* pawn);

	// Per-frame; called from GameEngine::Tick. Suppresses CRITICALFAILURE posture
	// on grinders and (when the "grinder" channel is on) samples their state.
	void Tick();
}
