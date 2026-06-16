// ============================================================================
//  THE SUPER AGENT MISSION
// ============================================================================
// This file IS the mission. Everything you tune lives here. The machinery in
// SuperAgent.cpp reads these configs to spawn, gate, and tick the objectives —
// you should not need to touch it.
//
//   - Each capture point is configured independently below.
//   - A.at(pct, []{ ... }) adds a hook that fires ONCE, the first time the hold
//     bar crosses `pct` (0.0 .. 1.0). This is where you spawn things for the
//     players to fight, taunt them, change the rules — whatever.
//   - Helpers usable inside a hook body:
//       Adds::Unleash(tableId)  re-table every boss-room factory and spawn now
//                               (41 = bosses). Guaranteed spawn at all of them.
//       Adds::Alarm()           fire the engine siren / AlarmBots kismet.
//       Log("...", ...)         write to the "superagent" log channel.
//
// Objective ids + HUD names + base capture times are in <repo>/objectives.md.
// ============================================================================

#include "src/GameServer/GameModes/SuperAgent/SuperAgent.hpp"

using namespace SuperAgent;

void SuperAgent::AuthorMission() {

	// ------------------------------------------------------------------ //
	//  POINT A — the 5-minute hold. The heart of the mission.            //
	// ------------------------------------------------------------------ //
	A.objectiveDefId    = 194;      // "Hack and Hold 5m"  (HUD name)
	A.priority          = 2;        // unlocks after the boss (priority 1)
	A.ownerTaskForce    = 2;        // defenders own it -> players (TF1) capture it
	A.timeToCapture     = 300.0f;   // 5 minutes
	A.requireAllPlayers = false;     // every living + dead player must stand on it
	A.cylRadius         = 400.0f;
	A.cylHalfHeight     = 400.0f;

	// --- A's hooks: spawn shit for the players to fight while they hold. ---
	A.at(0.05f, [] {
		Adds::Unleash(99); // colony shit
		Adds::Alarm();
	});
	A.at(0.10f, [] {
		Adds::Unleash(101); // colony & scorpion
		Adds::Alarm();
	});
	A.at(0.15f, [] {
		Adds::Unleash(41); // boss
		Adds::Alarm();
	});
	A.at(0.20f, [] {
		Adds::Unleash(44); // boss
		Adds::Alarm();
	});
	A.at(0.50f, [] {
		Adds::Unleash(102); // colony shit
		Adds::Alarm();
	});
	A.at(0.55f, [] {
		Adds::Unleash(103); // colony shit
		Adds::Alarm();
	});
	A.at(0.60f, [] {
		Adds::Unleash(149); // juggernaut
		Adds::Alarm();
	});


	// Escape::Remap(59, 41);          // boss-room adds -> bosses
	Escape::PeriodicAlarm(120.0f, 300.0f);

	// ------------------------------------------------------------------ //
	//  POINT B — back to the dropship. Default attacker-capture = win.   //
	// ------------------------------------------------------------------ //
	B.objectiveDefId    = 287;      // "Get to the Dropship"
	B.priority          = 3;        // final objective
	B.ownerTaskForce    = 2;        // defenders own it -> players capture to win
	B.timeToCapture     = 3.0f;
	B.requireAllPlayers = true;
	B.cylRadius         = 300.0f;
	B.cylHalfHeight     = 400.0f;
}

// ============================================================================
//  COMPOSITE SPAWN TABLES
// ============================================================================
// For the Super Agent difficulty, REBUILD a spawn table by concatenating the
// groups of other tables — no DB editing, just list them here. Groups are
// appended in order and renumbered (table 100's 4 groups become 0-3, table
// 101's 3 groups become 4-6 → table 34 ends up with 7 groups). List a table as
// its own source to keep its original groups too (e.g. { 34, { 34, 100 } }).
//
//   target table -> { source tables, in append order }
const std::map<int, std::vector<int>>& SuperAgent::SpawnTableComposites() {
	static const std::map<int, std::vector<int>> composites = {
		{ 34, { 34, 100, 101, 104 } },   // support enemies
		{ 28, { 28, 167} }, // normal spawns
		{ 29, { 29, 212, 72, 210} }, // first spawn
		{ 33, { 33, 102, 102 } }, // responders
		{ 40, { 40, 100, 101, 104 } }, // support enemies
		{ 58, { 58, 167} }, // normal spawns + guardian
	};
	return composites;
}
