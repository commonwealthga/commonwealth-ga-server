#pragma once

#include "src/pch.hpp"

#include <map>
#include <string>

// Server-side AFK backstop. The client self-disconnects after the server-sent
// AFK_TIMEOUT_SEC (900s) via TgPlayerController.CheckAFKForDC — but that runs
// on the CLIENT, so a hung client that still ACKs TCP keepalives and sends UDP
// heartbeats never leaves: the engine's ~30s ConnectionTimeout never fires and
// the pawn squats in the map indefinitely (observed: days, until the instance
// degraded). This sweep reaps any player connection whose pawn shows no
// position/rotation change for Config::GetAfkKickSeconds() (default 1200s —
// deliberately above the client's 900s so healthy clients always self-DC
// first and this only catches clients that failed to), then notifies the
// control server (GAME_EVENT afk_kick) to drop the TCP session too.
//
// Driven from GameEngine__Tick; self-throttles to one sweep per ~15s.
// Log channel: "afk".
class AfkReaper {
public:
	static void Tick(float DeltaSeconds);

private:
	struct ActivityState {
		int      pawn_id = 0;
		// SDK FVector/FRotator default ctors leave members uninitialized —
		// zero explicitly. pawn_id 0 makes the first sweep count as activity
		// anyway, so the snapshot gets seeded from the live pawn.
		FVector  location = FVector(0.0f, 0.0f, 0.0f);
		FRotator rotation = FRotator(0, 0, 0);
		float    idle_seconds = 0.0f;
	};
	// Keyed by session_guid (stable across pawn respawns; pointer keys are
	// unsafe — engine recycles pawn allocations).
	static std::map<std::string, ActivityState> states_;
	static float sweep_accum_;
	static int   threshold_seconds_;  // -1 = not read yet, 0 = disabled
};
