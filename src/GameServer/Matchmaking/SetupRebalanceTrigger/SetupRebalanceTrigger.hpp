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
};
