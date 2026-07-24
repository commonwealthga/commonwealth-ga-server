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
//       Adds::Unleash(tableId)         spawn `tableId` from ONE boss-room
//                                      factory, picked at random per call
//                                      (41 = bosses).
//       Adds::UnleashOutside(tableId, opts)
//                                      spawn `tableId` from the factory just
//                                      outside the boss room and march the
//                                      bots in toward point A. `opts` is an
//                                      optional UnleashOpts: scale, speedMult,
//                                      damageMult, healthMult, name, VO barks,
//                                      "cheater" knobs — e.g.
//                                        UnleashOpts o; o.scale = 1.6f;
//                                        o.healthMult = 3.0f; o.name = L"Warden";
//                                        o.voCueIds = { 60, 63 };
//                                        o.voMinSecs = 6; o.voMaxSecs = 14;
//                                        Adds::UnleashOutside(10011, o);
//
//   VO bark cue ids are SEMANTIC SLOTS resolved against the bot's OWN audio
//   group, so a bot always shouts in its own voice — you cannot borrow another
//   bot's voice by id, only by giving it that bot's body mesh. A slot the bot's
//   group doesn't implement is silent. Elite Helot (bot 1321, body asm 2177,
//   audio group 99) implements:
//     54 PatrolIdle  55 TargetSighted  56 Responding    58 TargetLost
//     59 UnderAttack 60 Wilhelm        61 Retreat       62 TargetStealthed
//     63 ScoredKill  64 AggroChange    65 Incapacitated_Choke
//   (57 Strike and 66 Taunt exist for the Elite Assassin's group 98, NOT 99.)
//   o.voPitch shifts the barks (1.0 normal, >1 higher). Leaving it 0 uses the
//   cheap replicated-int path; setting it routes through Kismet_ClientPlaySound,
//   which is still positional and attached to the bot. Falls back automatically
//   if the cue can't be loaded — watch the "botvoice" channel.
//
//   o.devices swaps gear per slot, replacing what the bot spawned with. The
//   Elite Helot's primary is device 4128 at DB slot 198 = engine equip point 2,
//   so handing it an Inferno-X Cannon (2914) is:
//     o.devices = { { 2, 2914 } };          // { equipPoint, deviceId }
//   Melee is point 1. Device ids are in <repo>/equippable-devices.md; a bot's
//   own loadout is asm_data_set_bots_data_set_bot_devices.
//
//   o.noCooldowns / o.infinitePower are the retail dev-console cheat flags
//   (TgPawn m_bCheatNoRecharge / m_bCheatUseNoEnergy), both checked server-side.
//   noCooldowns is what you want when handing a bot an offhand grenade/bomb —
//   otherwise it inherits that device's normal recharge and throws once.
//
//   o.alertText pops a red center-screen heads-up for everyone when the spawn
//   lands, same styling as the escape-phase ambush warning:
//     o.alertText = L"The Agenderp has joined the match!"; o.alertSecs = 6.0f;
//
//   o.cosmeticItemIds dresses the bot — suits/helmets/flairs by ITEM id from
//   <repo>/cosmetic-items.md (e.g. 6994 = "Eternal Glory" helmet flair):
//     o.cosmeticItemIds = { 6994 };
//   ONLY works on bots whose pawn class is TgPawn_Character. A bot with its own
//   mesh (body_asm_id) has no character assembly and is skipped — the Elite
//   Helot is TgPawn_AndroidMinion, so cosmetics do NOTHING on it. Check the
//   "cosmetic-equip" log channel to see which way a given bot went.
//       Adds::UnleashOutside(tableId, { {botId, opts}, ... })
//                                      PER-BOT-ID options — one spawn table,
//                                      different treatment per enemy type.
//                                      Matched on the bot id SpawnBotById
//                                      stamps on the pawn (r_nProfileId);
//                                      botId 0 is the wildcard fallback.
//                                      Set opts.isLeader on ONE group and
//                                      opts.followLeader on others to make an
//                                      escort pack that trails the leader:
//                                        UnleashOpts boss;
//                                        boss.scale = 1.6f; boss.isLeader = true;
//                                        UnleashOpts pack;
//                                        pack.followLeader = true;
//                                        pack.behaviorFromBotId = 1412;
//                                        Adds::UnleashOutside(10012,
//                                          { { 1321, boss }, { 1431, pack } });
//                                      Followers skip the march and use the
//                                      engine's own formation follow, but ONLY
//                                      if their behavior carries destination
//                                      code 310 — donate one via
//                                      behaviorFromBotId (1412-1415 = behavior
//                                      674, 1526/1527/1541 = behavior 692).
//                                      The alert is call-level: first group
//                                      that sets alertText wins, fires once.
//       Adds::Alarm()                  fire the engine siren / AlarmBots kismet.
//       Log("...", ...)                write to the "superagent" log channel.
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

	UnleashOpts defaultDelay;
	defaultDelay.spawnDelay = 0.5f;

	A.at(0.05f, [] {
		Adds::Unleash(99, 0.2f); // colony shit
		// Adds::Alarm();
	});
	A.at(0.10f, [] {
		Adds::Unleash(101, 0.2f); // colony & scorpion
		// Adds::Alarm();
	});
	A.at(0.11f, [defaultDelay] {
		Adds::UnleashOutside(102, defaultDelay); // small wave
		// Adds::Alarm();
	});
	A.at(0.15f, [] {
		Adds::Unleash(41, 0.2f); // boss
		// Adds::Alarm();
	});
	A.at(0.20f, [] {
		Adds::Unleash(44, 0.2f); // boss
		// Adds::Alarm();
	});
	A.at(0.35f, [defaultDelay] {
		Adds::UnleashOutside(102, defaultDelay); // small wave
		// Adds::Alarm();
	});
	A.at(0.45f, [] {
		Adds::Unleash(102, 0.2f); // colony shit
		// Adds::Alarm();
	});
	A.at(0.50f, [defaultDelay] {
		Adds::UnleashOutside(102, defaultDelay); // small wave
		// Adds::Alarm();
	});
	A.at(0.55f, [] {
		Adds::Unleash(103, 0.2f); // colony shit
		// Adds::Alarm();
	});
	A.at(0.60f, [] {
		Adds::Unleash(10013); // juggernaut
		// Adds::Alarm();
	});

	A.oneOf(0.75f, {
		/*{ 0.9999999f, [] {
			UnleashOpts guardian;
			guardian.damageMult = 0.2;
			guardian.healthMult = 2;
			guardian.scale = 0.50;
			guardian.name = L"Mini Guardian";
			guardian.voPitch = 1.45f;
			guardian.alertText = L"Warning! A group of mini guardians have been spotted in the area!";
			guardian.alertSecs = 10.0f;

			Adds::UnleashOutside(193, {{1461, guardian}});
			// Adds::Alarm();
		}},*/
		{ 0.000001f, [] {
			// The Agenderp base is Shock Trooper 1570 (v153: 10012 group 1) —
			// a TgPawn_Character bot so it can WEAR the Eternal Glory helmet;
			// the old Elite Helot base (TgPawn_AndroidMinion) had no character
			// assembly. The helot VOICE stays via voFromBotId (cue slots
			// resolve against helot body asm 2177 / audio group 99 on the
			// pitched bark path).
			UnleashOpts boss;
			boss.damageMult = 0.8;
			boss.healthMult = 150;   // was 30 on the helot: 1570 base HP 1000 vs 5000, same 150k total
			boss.speedMult = 1.5;
			boss.scale = 0.75;
			boss.rotationRateYaw = 120000;
			boss.accuracy = 1.0f;
			boss.setAccuracyLoss = true;
			boss.targetSwitchSecs = 0.8f;
			boss.behaviorFromBotId = 1384;
			boss.name = L"The Agenderp";
			boss.voCueIds  = { 59, 60, 55, 64, 65 };
			boss.voMinSecs = 2.5f;
			boss.voMaxSecs = 6.0f;
			boss.voPitch = 1.35f;
			boss.voFromBotId = 1321;             // helot voice on the trooper body
			// Suit 6780 (HoloSuit - Assault Heavy - Tier 3) + Eternal Glory
			// flair 6994. Item ids from cosmetic-items.md — cosmetics.md's ID
			// column is the row PK, not item_id (its "3136"/"3347" are really
			// 6780/6994). The first cosmetic flips bValidCustomAssembly, so
			// the client builds a player-style body from the assembly instead
			// of the shock-trooper bot mesh.
			boss.cosmeticItemIds = { 6780, 6994 };
			boss.devices = { { 2, 2914 }, {3, 2498} }; // inferno + conc nade
			boss.noCooldowns = true;
			boss.infinitePower = true;
			boss.alertText = L"Warning! The Agenderp has been spotted in the area!";
			boss.alertSecs = 10.0f;
			boss.isLeader = true;

			UnleashOpts medic;
			medic.followLeader = true;
			medic.behaviorFromBotId = 1576;
			medic.devices = { { 3, 2906 } };
			medic.name = L"Pocket medic";
			medic.damageMult = 1.3;
			medic.healthMult = 10;
			medic.scale = 0.75;
			medic.speedMult = 3;
			medic.infinitePower = true;

			Adds::UnleashOutside(10012, {{1570, boss}, {1571, medic}});
			// Adds::Alarm();
		 }}
	});

	// Escape::Remap(59, 41);          // boss-room adds -> bosses

	// --- Escape-phase global alarm: a WEIGHTED POOL of ambush waves. Every
	// alarm rolls ONE wave by weight, so the run-back stays unpredictable and
	// rare waves become "encounters" players chase. Weights are relative shares,
	// not percentages — they don't have to sum to 100; each wave's odds are its
	// weight / total. tableId 0 = each factory's OWN baked roster (the classic
	// behaviour, per-map); maxSpawnPoints = how many physical spawn POINTS the
	// wave may use (0 = uncapped) — points are picked player-position-aware:
	// never on top of a player, preferring behind the team / inside a split
	// team's gap (allowBehind/allowGaps/allowAhead/allowOnTop per wave);
	// rosterMin..rosterMax multiplies the table's rolled plan; durationSecs
	// floors the countdown to the NEXT alarm (slow-to-clear waves get more
	// room); alertText nullptr = NO warning (the wave arrives UNANNOUNCED —
	// good for small surprise waves), set a string to telegraph it; opts =
	// optional per-pawn overrides (stats/name/cosmetics/devices/barks/behavior
	// — no march here).

	// A small unannounced harassment wave (no alertText → silent).
	// The classic announced alarm keeps its warning string. Keep one common case.
	Escape::AlarmWave smallWave;
	smallWave.weight = 40.0f;
	smallWave.tableId = 102;
	smallWave.maxSpawnPoints = 4;
	smallWave.durationSecs = 5.0f;
	smallWave.allowAhead = true;
	smallWave.allowBehind = true;
	smallWave.allowGaps = true;
	smallWave.allowOnTop = true;
	smallWave.rosterMin = 0.5;
	smallWave.rosterMax = 1;
	// smallWave.alertText = L"WARNING! Recursive ambush wave detected!";

	Escape::AlarmWave standard;
	standard.weight = 25.0f;
	standard.tableId = 33;
	standard.maxSpawnPoints = 4;
	standard.durationSecs = 30.0f;
	standard.allowAhead = true;
	standard.allowBehind = true;
	standard.allowGaps = true;
	standard.allowOnTop = false;
	standard.rosterMin = 0.8;
	standard.rosterMax = 1.2;
	standard.alertText = L"WARNING! Recursive ambush wave detected in the area!";

	Escape::AlarmWave elites;
	elites.weight = 10.0f;
	elites.tableId = 10011;
	elites.maxSpawnPoints = 5;
	elites.durationSecs = 60.0f;
	elites.allowAhead = true;
	elites.allowBehind = true;
	elites.allowGaps = true;
	elites.allowOnTop = false;
	elites.rosterMin = 1.5;
	elites.rosterMax = 2.5;
	elites.alertText = L"WARNING! Elite strike team detected in the area!";

	Escape::AlarmWave ants;
	ants.weight = 10.0f;
	ants.tableId = 213;
	ants.maxSpawnPoints = 2;
	ants.durationSecs = 30.0f;
	ants.allowAhead = false;
	ants.allowBehind = true;
	ants.allowGaps = true;
	ants.allowOnTop = false;
	ants.rosterMin = 7;
	ants.rosterMax = 14;
	ants.alertText = L"WARNING! Swarm of colony ants detected in the area!";

	Escape::AlarmWave ticks;
	ticks.weight = 10.0f;
	ticks.tableId = 177;
	ticks.maxSpawnPoints = 4;
	ticks.durationSecs = 30.0f;
	ticks.allowAhead = true;
	ticks.allowBehind = true;
	ticks.allowGaps = true;
	ticks.allowOnTop = false;
	ticks.rosterMin = 6;
	ticks.rosterMax = 10;
	ticks.alertText = L"WARNING! Swarm of colony ticks detected in the area!";


	Escape::AlarmWave overlord;
	overlord.weight = 1.0f;
	overlord.tableId = 233;
	overlord.maxSpawnPoints = 1;
	overlord.durationSecs = 80.0f;
	overlord.allowAhead = false;
	overlord.allowBehind = true;
	overlord.allowGaps = false;
	overlord.allowOnTop = false;
	overlord.rosterMin = 1;
	overlord.rosterMax = 1;
	overlord.alertText = L"WARNING! Colony overlord detected in the area!";

	// --- rare "encounter" waves — fill in real tables/opts and tune weights ---
	// Escape::AlarmWave elites;
	// elites.weight = 15.0f;
	// elites.tableId = 10011;                 // migrated elite squad
	// elites.maxSpawnPoints = 1;              // just one eruption point, not everywhere
	// elites.alertText = L"WARNING! Elite strike team detected!";

	Escape::PeriodicAlarm(60.0f, 100.0f, { smallWave, standard , ants, ticks, overlord  });

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
		// local test:
		// { 102, { 33, 102, 102} }, // small group of responders
		// { 33, { 33, 102, 102, 33, 102, 102} }, // large group of responders

		// live:
		{ 34, { 34, 100, 101, 104, 148 } },   // support enemies
		{ 28, { 28, 167, 157, 148} }, // normal spawns
		{ 29, { 29, 29, 212, 212, 72, 210} }, // first spawn
		{ 102, { 33, 102, 102} }, // small group of responders
		{ 33, { 33, 102, 102, 33, 102, 102} }, // large group of responders
		{ 40, { 40, 40, 100, 101, 104, 148 } }, // support enemies
		{ 58, { 58, 58, 167, 157 } }, // normal spawns + guardian

		// { 177, { 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177, 177} }, // huge amount of ticks

		// { 213, { 213, 213, 213, 213, 213, 213, 213 }}, // huge amount of ants
		// { 193, { 193, 193, 193, 193, 193, 193, 193, 193 } }, // huge amount of guardians
	};
	return composites;
}

// ============================================================================
//  EXPLODE-ON-DEATH BOTS (ragdoll suppression)
// ============================================================================
// Bots listed here die with r_eDeathReason = DR_DESPAWN: the client plays the
// mesh's 'Despawned' FX group and hides the body immediately instead of
// ragdolling a corpse. Perf knob — the ambush/alarm waves are mostly
// human-like androids, and dozens of simultaneous ragdolls tank client FPS.
// Roster: everything in raw spawn tables 33 + 102 (the responder waves)
// except the elites, which stay on the normal ragdoll death.
const std::vector<int>& SuperAgent::ExplodeOnDeathBotIds() {
	static const std::vector<int> ids = {
		1335,   // Alarm Responder (table 33)
		1349,   // Alarm Responder (table 33)
		1458,   // Sand Spider     (table 102) — mesh has no 'Despawned' FX, vanishes without a flash
		1468,   // Colony Drone    (table 102)
		1492,   // Colony Soldier  (table 102)
		// excluded: 1320 Elite Assassin, 1321 Elite Helot
	};
	return ids;
}
