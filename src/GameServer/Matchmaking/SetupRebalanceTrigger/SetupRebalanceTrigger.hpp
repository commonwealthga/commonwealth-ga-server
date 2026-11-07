#pragma once

#include "src/pch.hpp"

// Detects the "5 seconds before setup ends" moment and asks the control server
// to rebalance the already-spawned roster (one-shot per setup phase). State is
// tracked per-Game; Tick() is called once per frame from GameEngine__Tick and
// self-gates on the mission timer state, so cheap polling is fine.
class SetupRebalanceTrigger {
public:
	static void Tick();

	// Clear all per-Game latches. Call on map change / level travel if needed.
	static void Reset();

	// Current TgGame.GameTimerState (0 before the first login, 1 setup, 2 mission
	// running, ...) and the seconds left in setup (-1 outside setup).
	static int TimerState();
	static float SetupSecondsLeft();
};
