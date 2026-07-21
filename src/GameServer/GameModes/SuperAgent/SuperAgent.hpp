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
class ATgBotFactory;

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

	// One weighted-random event. `weight` is a relative share (they need NOT sum
	// to anything); a tiny weight next to big ones = a super-rare event. Add an
	// event = add a `{weight, lambda}` line — that's the whole extension story.
	struct Event { float weight; std::function<void()> action; };

	// At `pct`, roll ONE of `events` by weight and fire it (once, like at()). Use
	// for "fun random event, one per mission" beats — the picked event fires, the
	// rest don't. Zero/negative weights are skipped. Remember to capture any
	// locals BY VALUE in the lambdas (the roll happens minutes later), e.g.
	//   A.oneOf(0.75f, {
	//       { 30.0f, [defaultDelay] { Adds::UnleashOutside(102, defaultDelay); } },
	//       { 10.0f, []             { /* the Agenderp beat */ } },
	//       {  1.0f, []             { /* super-rare event   */ } },
	//   });
	void oneOf(float pct, std::vector<Event> events);

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

// One equipment override, applied AFTER the bot's DB loadout is equipped —
// it REPLACES whatever the bot spawned holding at `equipPoint`.
//
// `equipPoint` is the ENGINE equip point, not the DB slot id. The bot loadout
// (asm_data_set_bots_data_set_bot_devices.slot_used_value_id) maps through
// BotGetEquipPointBySlot in TgGame__SpawnBotById.cpp:
//   221->1  198->2  200->3  199->4  201->5  202->6  203->7  204->8  385->9 ...
// Point 2 (DB slot 198) is the primary in-hand weapon for humanoid bots;
// point 1 (DB slot 221) is melee. Check what a bot actually carries with:
//   SELECT device_id, slot_used_value_id FROM asm_data_set_bots_data_set_bot_devices
//    WHERE bot_id = ?;
//
// NOTE: the replaced device's equip-effect buffs are NOT reverted — the old
// device is dropped from the slot, not unequipped. Harmless for stat-light
// weapons; if you swap off something with a big passive, expect it to linger.
struct DeviceSwap {
	int equipPoint = 2;      // engine equip point (2 = primary in-hand)
	int deviceId   = 0;      // asm_data_set_devices.device_id
	int deviceType = 388;    // GA_G::TGDT_ — 388 RANGED, 389 MELEE, 390 OFF_HAND, 981 SPECIALTY
	int quality    = 1162;   // same default the bot loadout query COALESCEs to
};

// Optional per-spawn modifiers for Adds::UnleashOutside. A field left at its
// default is not applied. Usage:
//   UnleashOpts o; o.scale = 1.6f; o.healthMult = 3.0f; o.name = L"Warden";
//   Adds::UnleashOutside(10011, o);
struct UnleashOpts {
	float scale      = 0.0f;   // size multiplier, mesh + collision cylinder (replicates via DrawScale)
	float speedMult  = 0.0f;   // GroundSpeed multiplier
	// Outgoing damage AND heal scale (TgEffectDamage.uc:120 / TgEffectHeal.uc:71
	// both use it). Absolute — it REPLACES the difficulty/BBM baseline seeded at
	// spawn, it does not stack on top. For a pure healer this is your heal
	// multiplier; there is no separate one.
	float damageMult = 0.0f;   // s_fDamageAdjustment
	// Health + max-health multiplier (spawns at full). Applied through props
	// 304/51, not the engine fields — see the comment at the call site.
	float healthMult = 0.0f;
	// Immune to stun AND EMP-stun (EMP bomb, Shatter bomb, ...). Stun lifetime
	// is mitigated by TgEffectGroup.CalcProtection: protection-prop raw /
	// attacker device rating = % reduction. Sets props 163 (PROTECTION_STUN)
	// and 235 (PROTECTION_EMP_STUN) so high no device rating can dent it —
	// the stun's lifetime calcs to 0 and it never applies.
	bool  stunImmune = false;
	const wchar_t* name = nullptr;   // display-name override -> the bot's PRI PlayerName

	// ---- "obvious cheater" knobs — each independent, all optional ----
	// Yaw turn rate in UE3 rotator units/sec (TgPawn default 20000). Push to
	// 100000+ for instant snap-flicks onto targets — the single most
	// aimbot-looking thing a bot can do.
	int   rotationRateYaw = 0;
	// TGPID_ACCURACY (prop 10), 0..1. 1.0 = laser.
	float accuracy = 0.0f;
	// Accuracy-loss props: spread added while shooting / after jumping, and
	// the cap on accumulated loss. Set to 0 for no-recoil/no-bloom. These are
	// "apply exactly this value" (not multipliers), so use the explicit flags
	// to distinguish "leave alone" from "set to zero".
	bool  setAccuracyLoss     = false;
	float accuracyLossOnShoot = 0.0f;
	float accuracyLossOnJump  = 0.0f;
	float accuracyLossMax     = 0.0f;
	// Re-roll a RANDOM living player as the bot's target every N seconds —
	// the "spinning at people across the room for no reason" tell. 0 = off.
	// Starts only once the bot is INSIDE the boss room (armed at the doorway
	// handoff), never during the march: handing the behavior a combat target
	// early stalls the patrol drive that walks it in.
	float targetSwitchSecs = 0.0f;
	// Movement tells: bunnyhop (JumpZ / AirControl multipliers) and low
	// friction so direction changes slide (strafe-jank).
	float jumpMult       = 0.0f;
	float airControlMult = 0.0f;
	float frictionMult   = 0.0f;   // sets r_fFrictionMultiplier outright
	// Retail dev-console cheat flags on TgPawn, both checked server-side.
	// noCooldowns: TgDevice.StartCooldown early-returns, so devices (including
	// offhand grenades/bombs) fire with no recharge at all. infinitePower:
	// TgPawn.ConsumePowerPool skips the drain. Covers the bot's whole loadout,
	// DB-native and swapped alike.
	bool  noCooldowns   = false;
	bool  infinitePower = false;

	// Center-screen heads-up shown to every player when the spawn lands — same
	// red styling as the escape-phase ambush warning (priority 1 Normal / type 2
	// Detrimental). nullptr = no alert. Only fires if at least one bot actually
	// spawned, so a failed unleash never announces itself.
	const wchar_t* alertText = nullptr;
	float          alertSecs = 5.0f;

	// Drip rate for Adds::UnleashOutside (seconds between spawns). 0 = the whole
	// roster spawns at once (default); >0 = trickle it in one bot every
	// `spawnDelay` seconds, so a huge wave walks into the boss room one-by-one
	// instead of swarming all at once. Wave-level like alertText: in a per-bot-id
	// group call, the FIRST group that sets it (>0) wins for the whole unleash.
	float          spawnDelay = 0.0f;

	// Cosmetics — asm_data_set_items.item_id values (see <repo>/cosmetic-items.md).
	// Suits (subtype 1008), helmets (1006) and helmet flairs (1007) all go in
	// this one list; each is dispatched by its own subtype, and the paired mesh
	// id is resolved for you, so pass the ITEM id, never the mesh asm id.
	//
	// ONLY takes effect on bots whose pawn class is TgPawn_Character. Bots with
	// their own mesh (asm_data_set_bots.body_asm_id) have no character assembly
	// — the Elite Helot is TgPawn_AndroidMinion — and are skipped with a line on
	// the "cosmetic-equip" channel.
	std::vector<int> cosmeticItemIds;

	// ---- escort wiring (multi-group unleashes only) ----
	// Set isLeader on exactly ONE group. Every group with followLeader=true gets
	// aic->m_pOwner pointed at the leader pawn and skips the march — they trail
	// the leader instead, via the engine's own SetFormationLocation (randomized
	// offset around the leader, path-aware).
	//
	// A follower ALSO needs a behavior whose destination code is 310
	// (SetMovementTarget(m_pOwner)) — most rank-and-file behaviors don't have
	// one and will simply ignore the owner. Donate one with behaviorFromBotId:
	// bots 1412-1415 (behavior 674) or 1526/1527/1541 (behavior 692) carry it.
	// Owner-relative targeting rides along for free in those behaviors: they can
	// pick the owner's last attacker / current target as their own.
	bool isLeader     = false;
	bool followLeader = false;

	// Periodic VO barks: every [voMinSecs, voMaxSecs] the bot shouts one cue
	// picked at random from voCueIds. voMaxSecs <= 0 = off.
	//
	// Ids are asm_data_set_sound_cues.sound_cue_id, which are semantic SLOTS —
	// each id means the same thing everywhere (60 = Bot_Wilhelm, 63 =
	// Bot_ScoredKill, 64 = Bot_AggroChange) but is resolved against the PAWN'S
	// OWN audio group, so the bot always shouts in its own voice. A slot the
	// bot's group doesn't implement is simply silent (e.g. Elite Helot, group
	// 99, has no 57/66). Query the group for a body mesh with:
	//   SELECT c.sound_cue_id, c.sound_cue_name FROM asm_data_set_sound_cues c
	//    JOIN asm_data_set_asm_mesh_audio_groups g USING (audio_group_id)
	//   WHERE g.asm_id = <bots.body_asm_id>;
	std::vector<int> voCueIds;
	float voMinSecs = 0.0f;
	float voMaxSecs = 0.0f;
	// Pitch shift for the barks. 0 (default) = off: barks take the cheap
	// replicated-int path with the cue played as authored. Set it (1.0 normal,
	// >1 higher, <1 lower) to route through Kismet_ClientPlaySound instead,
	// which carries an explicit PitchMultiplier. Still positional — the sound
	// is attached to the bot and tracks it. If the cue can't be resolved or
	// loaded, it silently falls back to the plain path rather than going quiet.
	float voPitch = 0.0f;

	// Equipment overrides — swap a bot's gear per slot. Empty = keep its DB
	// loadout. See DeviceSwap above. e.g. give an Elite Helot (primary = device
	// 4128, DB slot 198 -> point 2) an Inferno-X Cannon instead:
	//   o.devices = { { /*equipPoint=*/2, /*deviceId=*/2914 } };
	std::vector<DeviceSwap> devices;

	// Run a DIFFERENT bot's AI: re-runs TgAIController::InitBehavior with the
	// donor bot id's config, so the spawned bot keeps its own body/stats but
	// thinks like the donor (e.g. give an Elite Helot a support Guardian's
	// brain). 0 = keep its own. The donor must be a bot id the client has
	// marshalled (any id that appears in a spawn table does).
	int behaviorFromBotId = 0;
};

// One enemy type's slice of a multi-group unleash: the bot id it applies to,
// and the options for it. botId 0 = wildcard (anything not matched by another
// group). See Adds::UnleashOutside's group overload.
struct UnleashGroup {
	int         botId;   // asm_data_set_bots.bot_id
	UnleashOpts opts;
};

namespace Adds {
	// Pick ONE cached boss-room factory (the ones whose spawn table id is
	// kBossAddsTable at map load) at random — re-randomized every call — re-point
	// it at `tableId`, rebuild its queue, and spawn. 41 = bosses. One factory per
	// call keeps the wave size fixed regardless of how many alarm spawn points
	// the map bakes in (some have 3+).
	//
	// spawnDelay (seconds): 0 = the whole roster lands at once (instant, default);
	// >0 = DRIP it in, one bot every `spawnDelay` seconds, e.g.
	//   Adds::Unleash(99, 1.5f);   // colony wave trickles in ~1 bot / 1.5s
	// Keep it short relative to the gap before the next unleash on the boss-room
	// factories — a later Unleash re-tables the factory and drops any of this
	// wave still waiting in the queue. (Marching UnleashOutside spawns stay
	// instant; their walk-in already paces them.)
	void Unleash(int tableId, float spawnDelay = 0.0f);

	// Unleash `tableId` from the OUTSIDE factory — the normal (non-alarm,
	// non-pet) factory whose spawn nav point is nearest a boss-room entrance,
	// where "entrance" = a one-way ATgModifyPawnPropertiesVolume facing the
	// boss objective (cached at Init) — and march the spawns into the boss
	// room (patrol drive to a baked in-room nav + two-leg trigger march).
	// For "a big enemy walks in menacingly" beats. `opts` optionally rescales
	// / renames the spawns — see UnleashOpts above. Set opts.spawnDelay > 0 to
	// DRIP the wave in one bot at a time instead of all at once (see UnleashOpts).
	void UnleashOutside(int tableId, const UnleashOpts& opts = UnleashOpts());

	// Same, but with PER-BOT-ID options — one spawn table, different treatment
	// per enemy type. A pawn is matched to its group by the bot id SpawnBotById
	// stamped on it (pawn->r_nProfileId); a group with botId 0 is the wildcard
	// fallback for anything unmatched.
	//
	//   UnleashOpts boss;  boss.scale = 1.6f; boss.isLeader = true;
	//   UnleashOpts pack;  pack.scale = 0.8f; pack.followLeader = true;
	//                      pack.behaviorFromBotId = 1412;   // needs dest code 310
	//   Adds::UnleashOutside(10012, { { 1321, boss }, { 1431, pack } });
	//
	// The spawn alert (alertText/alertSecs) is call-level: the FIRST group that
	// sets alertText wins, and it fires once for the whole unleash. Likewise the
	// FIRST group whose opts.spawnDelay > 0 sets the whole wave's drip rate.
	void UnleashOutside(int tableId, const std::vector<UnleashGroup>& groups);

	// Fire the engine alarm (siren + AlarmBots kismet). Cosmetic/atmospheric —
	// pair it with Unleash() for the actual spawn.
	void Alarm();

	// Queried by the TgBotFactory::ResetQueue hook. True when SuperAgent has
	// taken `f` over (any Unleash / escape wave / escape respawn) and the call
	// is NOT SuperAgent's own — such external rebuilds are dropped. Rationale:
	// UC TgBotEncounterVolume.CheckTouching fires StartEncounter -> ResetQueue(0)
	// on every member factory with no live bots whenever the human count in the
	// volume changes (death/respawn/walk-in), wiping in-flight wave queues; the
	// native ActivateAlarm retargets factories through the same call.
	bool BlockExternalReset(ATgBotFactory* f);

	// Queried by the TgBotFactory::SpawnNextBot hook. True when `f`'s CURRENT
	// queue was authored by Unleash/UnleashOutside — those spawns are the wave
	// itself, not alarm responders, so the hook skips the m_bAlarmBot stamp
	// (which would expose them to the behavior stand-down/despawn actions,
	// ExecuteAction 1318 DespawnAlarmBots / 1272 Despawn). Escape alarm waves
	// (FireGlobalAlarm) and untouched factories keep retail marking.
	bool SuppressAlarmMark(ATgBotFactory* f);
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
	// normal-mission alarm. Every alarm is telegraphed 5s ahead by a
	// center-screen warning toast. maxSecs <= 0 disables. Author in
	// AuthorMission().
	void PeriodicAlarm(float minSecs, float maxSecs);

	// One possible flavour of escape-phase ambush. Give PeriodicAlarm a pool of
	// these and each alarm picks ONE at weighted random — so the run-back stays
	// unpredictable, and rare high-weight-denominator waves become "encounters"
	// players chase. A field left at default keeps the legacy behaviour.
	struct AlarmWave {
		int   tableId      = 0;         // spawn table to fire; 0 = each factory's OWN default roster
		float weight       = 1.0f;      // relative selection weight (share of the total)
		int   maxFactories = 0;         // cap how many factories erupt; 0 = unlimited (nearest to each player)
		// Heads-up toast shown 5s before the wave. nullptr = NO warning — the
		// wave arrives UNANNOUNCED (good for small surprise waves). Set a string
		// to telegraph it. alertSecs = how long it stays up (5s lead is fixed).
		const wchar_t* alertText = nullptr;
		float          alertSecs = 5.0f;
		// Optional per-pawn overrides applied to THIS wave's fresh spawns —
		// scale / stats / name / cosmetics / devices / barks / behavior swap.
		// The march & leader/follow fields are ignored here (escape spawns fight
		// in place, they don't march the boss-room route).
		UnleashOpts opts;
	};

	// Weighted-pool form. Each alarm rolls one AlarmWave from `waves` by weight.
	// An empty pool behaves exactly like the plain overload above.
	void PeriodicAlarm(float minSecs, float maxSecs, const std::vector<AlarmWave>& waves);
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
