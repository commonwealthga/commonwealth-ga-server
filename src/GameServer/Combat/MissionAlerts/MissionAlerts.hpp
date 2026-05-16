#pragma once

#include "src/pch.hpp"

// Mission-wide announcer alerts. Drives:
//
//   ALL game modes:
//     - "60 seconds remaining"
//     - "30 seconds remaining"
//     - "10 seconds remaining"
//
//   TgGame_Ticket:
//     - "Your team has taken control!" / "The enemy has taken control!"
//       — fires when a team's owned-captured count crosses ≥2 (the points-
//         awarding threshold)
//     - "Your/enemy team has 90 percent control."
//       — fires when a team's r_nCurrentPointCount crosses 90% of pointsToWin
//
//   TgGame_PointRotation:
//     - "Your team has taken the lead." / "The enemy team has taken the lead."
//     - "There is a tie for control."
//       — each followed by "Point Changing"
//     - "15 / 10 / 5 Seconds to Point Activation."  (between objectives)
//     - "Objective Location Activated"             (when next objective unlocks)
//
// State is tracked per-Game (via Game*). Tick() is throttled at the call site
// (currently GameEngine__Tick) to ~1Hz so cheap polling is fine.
class MissionAlerts {
public:
	// Called at ~1Hz from GameEngine__Tick. Resolves current Game/GRI/now and
	// runs all per-mode pollers.
	static void Tick();

	// PointRotation rotation hooks — recorded as timestamps and consumed by
	// the per-tick poller to produce countdown / activation alerts.
	static void NotifyNextObjectiveScheduled(void* Game, float scheduledAtWorldTime);
	static void NotifyObjectiveActivated(void* Game);

	// Reset all per-Game state. Call on map change / level travel if needed.
	static void Reset();
};
