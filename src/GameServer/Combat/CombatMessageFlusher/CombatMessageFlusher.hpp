#pragma once

// Periodic flusher for the engine's per-pawn combat-message accumulator.
//
// TgPawn_Character::SendCombatMessage at 0x109dd9c0 only flushes when the
// accumulator hits records>=21 OR indices>=101 (FUN_109f9bc0). The original
// engine almost certainly had a time-based force-flush — s_fLastCombatMessageProcess
// at +0x1648 is the dead giveaway — but it was stripped along with the rest
// of the server-side natives, leaving sparse damage events to pile up
// indefinitely in the accumulator and never reach the client.
//
// We compensate by walking GClientConnectionsData at ~10Hz off WorldInfo
// time and serializing any non-empty accumulator into a marshal that's
// dispatched via the same pawn->vtable[0x266] path the engine emitter
// uses internally — landing in our TgPawn_Character__SendMarshal hook.
class CombatMessageFlusher {
public:
	// Called once per GameEngine::Tick. Self-throttles internally.
	static void Tick();
};
