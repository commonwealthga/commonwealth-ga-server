#pragma once

#include "src/pch.hpp"

#include <functional>
#include <map>
#include <vector>

// Custom "Super Agent" game mode. Active only under the difficulty
// GA_G::DIFFICULTY_VALUE_ID_CUSTOM_SUPER_AGENT.
//
// Players stay TF1 (attackers) for the WHOLE match — there is no team flip.
// The mission is reshaped purely by moving map geometry between task forces:
//
//   1. Players kill the boss -> proximity point A (the 5-minute hold) unlocks.
//      On boss death we flip BOTH spawn clusters + the DEFENDER-side door
//      markers/blockers, so a dying player now respawns by the boss room and
//      can walk out.
//   2. Players hold A together for 5 minutes. While holding, configurable
//      capture-% HOOKS fire (see SuperAgentMission.cpp) — e.g. at 25% the
//      boss-room "adds" factories switch to bosses and unleash.
//   3. A captured -> the RESIDUAL door markers/blockers flip, opening the path
//      back to the original spawn. Point B unlocks.
//   4. Players walk back and capture B (default attacker-capture) -> win.
//
// The MISSION ITSELF is authored in SuperAgentMission.cpp. This header is the
// surface that file uses. The machinery (spawning, ticking, geometry flips)
// lives in SuperAgent.cpp and you should rarely need to touch it.
//
// See .planning/2026-06-15-super-agent-game-mode-design.md for the mechanics.

class ATgGame;
class ATgMissionObjective_Proximity;

namespace SuperAgent {

// ---------------------------------------------------------------------------
// A single capture point. Configure the fields and add hooks in AuthorMission()
// (SuperAgentMission.cpp). Everything below the divider is machinery-owned.
// ---------------------------------------------------------------------------
struct CapturePoint {
	// ---- config — tweak per point ----
	int   objectiveDefId    = 0;       // asm_data_set_objectives id -> HUD name + base time (see objectives.md)
	int   priority          = 0;       // unlock-chain priority; lower = unlocks earlier + shows higher in the HUD list
	int   ownerTaskForce    = 2;       // 1 = attackers, 2 = defenders. Players are TF1, so TF2-owned = "players capture it".
	float timeToCapture     = 30.0f;   // seconds to fill the bar
	bool  requireAllPlayers = false;   // freeze capture unless EVERY current player (living or dead) is on the point
	float cylRadius         = 300.0f;  // capture cylinder radius
	float cylHalfHeight     = 400.0f;  // capture cylinder half-height (tall so floor-level players count)

	// Add a hook that fires ONCE, the first time capture crosses `pct` (0..1).
	// The body runs server-side on the gameplay thread. This is the meat of the
	// mode — spawn things, flip things, taunt the players. See SuperAgentMission.cpp.
	void at(float pct, std::function<void()> action);

	// ---- machinery-owned ----
	ATgMissionObjective_Proximity* obj = nullptr;
	struct Hook { float pct; bool fired; std::function<void()> action; };
	std::vector<Hook> hooks;
};

// The two points of the mission. Configured in AuthorMission(); read everywhere.
extern CapturePoint A;   // the 5-minute hold
extern CapturePoint B;   // back to the dropship -> win

// You author this in SuperAgentMission.cpp. Called once per match during Init,
// before the objectives are spawned.
void AuthorMission();

// Spawn-table composition for the Super Agent difficulty. Maps a target spawn
// table id -> an ordered list of source table ids whose groups are concatenated
// (and renumbered) to REBUILD the target. Lets you mix existing rosters into
// richer ones without editing the DB. Authored in SuperAgentMission.cpp;
// consumed by the bot-factory spawn-table loader at map load. Only applied when
// IsActive(). A source that's also the target is read first, so it can include
// the original groups. Sources must be raw DB tables, not other composites.
const std::map<int, std::vector<int>>& SpawnTableComposites();

// ---------------------------------------------------------------------------
// Helpers callable from inside hook bodies.
// ---------------------------------------------------------------------------
namespace Adds {
	// Re-point every cached boss-room factory (the ones whose spawn table id is
	// kBossAddsTable at map load) at `tableId`, rebuild its queue, and spawn
	// immediately. 41 = bosses. This GUARANTEES a spawn at every boss-room
	// factory (unlike the engine alarm, which only pops the single closest one).
	void Unleash(int tableId);

	// Fire the engine alarm (siren + AlarmBots kismet). Cosmetic/atmospheric —
	// pair it with Unleash() for the actual spawn.
	void Alarm();
}

// Escape phase. When point A is captured, EVERY map bot factory is respawned
// fresh (current enemies cleared, a new roster spawned) so the run back to B is
// a fight. Use this remap to make that roster harder than the approach phase.
namespace Escape {
	// Map a factory's current spawn table -> the table it should use during the
	// escape phase. Factories whose current table isn't listed respawn with
	// their existing table. Author these in AuthorMission().
	void Remap(int oldTable, int newTable);

	// During the escape phase, set off a GLOBAL alarm at a random interval
	// between `minSecs` and `maxSecs` — ambushing every player from the
	// alarm-gated factory nearest to them (its own roster), like the
	// normal-mission alarm. maxSecs <= 0 disables. Author in AuthorMission().
	void PeriodicAlarm(float minSecs, float maxSecs);
}

// Log to the "superagent" channel.
void Log(const char* fmt, ...);

// ---------------------------------------------------------------------------
// Engine entry points (machinery — wired into the hooks, don't call by hand).
// ---------------------------------------------------------------------------

// True when this match runs under the custom Super Agent difficulty.
bool IsActive();

// Author the mission, spawn A + B, cache spawn/blocker geometry, force the boss
// to priority 1 and reorder the GRI list. Called once from TgGame::InitGameRepInfo.
void Init(ATgGame* Game);

// Capture gate for the ProcessEvent intercept of TgMissionObjective_Proximity.Tick.
// Returns false to FREEZE this tick's capture math (skip CallOriginal).
bool ShouldRunCapture(ATgMissionObjective_Proximity* Obj);

// Called right after CallOriginal of Tick. Drives boss-killed detection, the
// capture-% hooks, and the A-captured geometry flip.
void OnCaptureTick(ATgMissionObjective_Proximity* Obj);

}  // namespace SuperAgent
