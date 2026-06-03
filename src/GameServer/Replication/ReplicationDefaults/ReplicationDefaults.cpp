#include "src/GameServer/Replication/ReplicationDefaults/ReplicationDefaults.hpp"
#include "src/pch.hpp"
#include "src/GameServer/Utils/ObjectCache/ObjectCache.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <optional>
#include <vector>

// ═══════════════════════════════════════════════════════════════════════════
// ENGINE-MANAGED RUNTIME MUTATIONS  (CALIBRATION REFERENCE — READ BEFORE TUNING)
//
// These fields are written from UnrealScript at runtime. Static CDO baselines
// only matter until the engine path fires. If you pick a baseline that fights
// these, the engine wins — align baselines so the static + dynamic states are
// consistent. Sourced from a full grep of unrealscript/ on 2026-05-23.
//
// ─── NetPriority ──────────────────────────────────────────────────────────
//   Pawn.PossessedBy           → 3.0           (HARD; no UC reset on UnPossess)
//   Vehicle.PossessedBy        → 3.0
//   Vehicle.UnPossessed        → default.NetPriority   (reads CDO)
//   ⇒ Once any Controller possesses a Pawn (AI or player), live NetPriority
//     is permanently 3.0. CDO value only matters pre-first-possess. Setting
//     TgPawn=4.0 is decorative — engine clamps to 3 within a frame of
//     AIController::Possess.
//
// ─── NetUpdateFrequency ──────────────────────────────────────────────────
//   Pawn.PossessedBy           → 100.0
//   Pawn.UnPossessed           → 5.0           (literal, NOT default.)
//   Vehicle.PossessedBy        → 100.0
//   Vehicle.UnPossessed        → 8.0           (literal, NOT default.)
//   DroppedPickup.Landed       → 3.0           (down from CDO=8)
//   Actor.ForceNetRelevant     → 0.1           (one-shot relevance promotion)
//   ⇒ For Pawn family, CDO is only seen pre-first-possess. Pick the baseline
//     to match the UnPossess restore literal so behavior is consistent if
//     Possess ever fires before / after this patcher runs.
//
// ─── RemoteRole ──────────────────────────────────────────────────────────
//   Pawn.PossessedBy           → ROLE_AutonomousProxy on owning client
//                              → default.RemoteRole on non-owning clients
//   PlayerController.UnPossess → ROLE_SimulatedProxy
//   PlayerController.PawnDied  → ROLE_SimulatedProxy
//   GameInfo.AddInactivePRI    → ROLE_None  (PRI parked, no rep)
//   GameInfo (path 2485)       → ROLE_SimulatedProxy on PRI revive
//   ⇒ DO NOT patch RemoteRole here. Engine cycles it constantly; UnPossess
//     restores from default.; clobbering CDO would break that restore path.
//     Intentional ROLE_None classes (Scout, InterpActor, TgKAsset_ClientSideSim,
//     TgProjectile) load correctly from UC and need no help.
//
// ─── bReplicateMovement ──────────────────────────────────────────────────
//   TgAIController.Revive          → true on owned TgPawn
//   TgPlayerController.Revive      → true on owned TgPawn
//   TgPawn (death paths 2968,4423; Timer 10182) → false
//   InterpActor.ApplyCheckpointRecord → true
//   ⇒ Pawns flip false on death, true on revive. CDO=true (alive on spawn)
//     is correct; runtime owns the death/revive transition.
//
// ─── bForceNetUpdate / SetNetUpdateTime ──────────────────────────────────
//   NOT configuration — verbs. bForceNetUpdate=true on damage, possess, score,
//   team change (~31 callsites). SetNetUpdateTime schedules one specific
//   update bypassing NetUpdateFrequency. Neither belongs in this file; not
//   represented in RepDefault.
//
// ─── Fields with ZERO runtime mutation (safe to fully pin in CDO) ────────
//   bNetTemporary, bSkipActorPropertyReplication, bReplicateInstigator,
//   bOnlyRelevantToOwner, bNetInitialOnly, bCallRepNotifyOnLocalSet,
//   bUpdateSimulatedPosition (set true in 3 places, never false),
//   bReplicateRigidBodyLocation, NetUpdateTime.
//
// ═══════════════════════════════════════════════════════════════════════════


// ═══════════════════════════════════════════════════════════════════════════
// PATCH STRUCT
//
// One field per replicable property. std::nullopt = "don't override, let UC
// default win." Presets apply LEFT-TO-RIGHT with LAST-WRITE-WINS per field —
// the rightmost preset that sets a field is the value used.
// ═══════════════════════════════════════════════════════════════════════════

namespace {

struct RepDefault {
	std::optional<float> NetPriority;
	std::optional<float> NetUpdateFrequency;
	std::optional<bool>  bAlwaysRelevant;
	std::optional<bool>  bOnlyRelevantToOwner;
	std::optional<bool>  bReplicateMovement;
	std::optional<bool>  bOnlyDirtyReplication;
	std::optional<bool>  bNetTemporary;
	std::optional<bool>  bSkipActorPropertyReplication;
	std::optional<bool>  bReplicateInstigator;
	std::optional<bool>  bUpdateSimulatedPosition;
	std::optional<bool>  bReplicateRigidBodyLocation;
	std::optional<float> AlwaysRelevantDistanceSquared;
};


// ═══════════════════════════════════════════════════════════════════════════
// PRESETS — composable building blocks
//
// Atomic toggles flip a single bool. Family composites bundle the
// NetPriority/Frequency/Movement choices for a class hierarchy. Per-class
// configs list `family + atomics`.
// ═══════════════════════════════════════════════════════════════════════════

// ── Atomic boolean toggles ──────────────────────────────────────────────────
inline const RepDefault kAlwaysRelevant     { .bAlwaysRelevant = true  };
inline const RepDefault kNotAlwaysRelevant  { .bAlwaysRelevant = false };
inline const RepDefault kOwnerOnly          { .bOnlyRelevantToOwner = true  };
inline const RepDefault kBroadcast          { .bOnlyRelevantToOwner = false };
inline const RepDefault kRepInstigator      { .bReplicateInstigator = true  };
inline const RepDefault kRepMovement        { .bReplicateMovement = true   };
inline const RepDefault kNoRepMovement      { .bReplicateMovement = false  };
inline const RepDefault kNetTemporary       { .bNetTemporary = true  };
inline const RepDefault kPersistent         { .bNetTemporary = false };
inline const RepDefault kSkipActorProps     { .bSkipActorPropertyReplication = true  };
inline const RepDefault kNoSkipActorProps   { .bSkipActorPropertyReplication = false };
inline const RepDefault kUpdateSimPos       { .bUpdateSimulatedPosition = true };
inline const RepDefault kRepRigidBody       { .bReplicateRigidBodyLocation = true };

// ── Update-frequency tiers (Hz) ─────────────────────────────────────────────
inline const RepDefault kFreqIdle    { .NetUpdateFrequency = 0.1f };   // force fields, lights, volumes
inline const RepDefault kFreqSlow    { .NetUpdateFrequency = 1.0f };   // pickups, matinee, low-churn info
inline const RepDefault kFreqMed     { .NetUpdateFrequency = 5.0f };   // pawns (idle), deployables
inline const RepDefault kFreqHud     { .NetUpdateFrequency = 10.0f };  // HUD-driving info actors
inline const RepDefault kFreqFast    { .NetUpdateFrequency = 30.0f };  // active devices
inline const RepDefault kFreqMax     { .NetUpdateFrequency = 100.0f }; // base Actor default

// ── Priority tiers ──────────────────────────────────────────────────────────
inline const RepDefault kPrioBase    { .NetPriority = 1.0f };
inline const RepDefault kPrioLow     { .NetPriority = 1.4f };
inline const RepDefault kPrioMed     { .NetPriority = 2.0f };
inline const RepDefault kPrioHigh    { .NetPriority = 3.0f };
inline const RepDefault kPrioCrit    { .NetPriority = 5.0f };

// ── Family composites ───────────────────────────────────────────────────────

// Generic pawn. Engine forces NetPriority→3 and NetUpdateFreq→100 on possess,
// so this baseline is the "unpossessed pawn" tuning. Matches UnPossess's
// literal NetUpdateFreq=5 restore so a possess-then-unpossess round trip
// leaves the value consistent with what the CDO says.
inline const RepDefault kPawnFamily {
	.NetPriority = 2.0f,
	.NetUpdateFrequency = 10.0f,
	.bUpdateSimulatedPosition = true,
	.AlwaysRelevantDistanceSquared = 1280000.0f,
};

// TgPawn — adds rigid-body location rep for ragdoll / physics poses.
// NetPriority=4 is engine-clamped to 3 on possess, so it's decorative for
// any pawn that gets controlled (i.e. all of them).
inline const RepDefault kTgPawnFamily {
	.NetPriority = 4.0f,
	.bReplicateRigidBodyLocation = true,
};

inline const RepDefault kTgPawnTurretFamily {
	.bAlwaysRelevant = true,
};

// HUD-critical info actor: GRI, PRI, TaskForce, TeamInfo. The whole symptom
// set ("HP=0 on HUD, wrong team colors, no game info on HUD") resolves here.
// Must reach every client every tick, must replicate properties (so default
// bSkipActorPropertyReplication=true inherited from Info is overridden).
inline const RepDefault kHudInfo {
	.NetPriority = 5.0f,
	.NetUpdateFrequency = 2.0f,
	.bAlwaysRelevant = true,
	.bReplicateMovement = false,
	.bOnlyDirtyReplication = false,
	.bSkipActorPropertyReplication = false,
};

// Inventory base — owner-only, doesn't move in the world.
inline const RepDefault kInventoryFamily {
	.NetPriority = 1.4f,
	.NetUpdateFrequency = 1.0f,
	.bOnlyRelevantToOwner = true,
	.bReplicateMovement = false,
};

// TgDevice (Weapon subclass). Visible to all players (everyone sees your
// weapon model), instigator replicated for kill credit + FX origin.
inline const RepDefault kDeviceFamily {
	.NetPriority = 3.0f,
	.NetUpdateFrequency = 1.0f,
	.bOnlyRelevantToOwner = true,
	.bReplicateMovement = false,
	.bReplicateInstigator = true,
};

// Deployables — placed actors that everyone in range needs to see consistently.
inline const RepDefault kDeployableFamily {
	.NetPriority = 1.4f,
	.NetUpdateFrequency = 5.0f,
	.bReplicateInstigator = true,
};

// Projectiles — short-lived one-shots, replicated once and destroyed.
inline const RepDefault kProjectileFamily {
	.NetPriority = 2.5f,
	.NetUpdateFrequency = 1.0f,
	.bAlwaysRelevant = true,
	.bNetTemporary = true,
	.bReplicateInstigator = true,
};

// Per-pawn effect router. Owner-attached, no movement.
inline const RepDefault kEffectManagerFamily {
	.NetPriority = 1.1f,
	.NetUpdateFrequency = 10.0f,
	.bReplicateMovement = false,
	.bReplicateInstigator = true,
};

// World-level singletons (WindManager, TimerManager) — always relevant, slow.
inline const RepDefault kSingletonFamily {
	.NetPriority = 2.0f,
	.NetUpdateFrequency = 1.0f,
	.bAlwaysRelevant = true,
	.bReplicateMovement = false,
};

// Static decoration / kismet-driven volumes that need property rep but no
// movement updates.
inline const RepDefault kStaticVolumeFamily {
	.NetPriority = 1.0f,
	.NetUpdateFrequency = 0.1f,
	.bAlwaysRelevant = true,
};


// ═══════════════════════════════════════════════════════════════════════════
// PER-CLASS CONFIG TABLE
//
// One line per class. Presets apply left-to-right; later entries override
// earlier ones per-field. Empty preset list `{}` would mean "documented as
// considered, no overrides applied" — but currently every entry has at
// least one preset.
//
// CDO FullName format: "<ClassName> <Package>.Default__<ClassName>"
// e.g. "TgPawn TgGame.Default__TgPawn"
// ═══════════════════════════════════════════════════════════════════════════

struct ClassConfig {
	const char* cdoFullName;
	std::vector<const RepDefault*> presets;
};

static const std::vector<ClassConfig>& Configs() {
	// Class list cross-checked against Actor__GetOptimizedRepListV2.cpp
	// g_blockClassTable (the dispatch table for the per-class rep handlers).
	// If it's there, it can replicate — so include it. Skipped on purpose:
	//   • Scout, InterpActor, TgKAsset_ClientSideSim, TgProjectile family —
	//     UC sets RemoteRole=ROLE_None at this class or above; per project
	//     decision we don't patch RemoteRole, so changes here wouldn't take
	//     effect. (Listed in TgProj_* group anyway so settings activate if
	//     you ever lift the RemoteRole gate at spawn time.)
	//   • Ambient sound / fog / light actors — UC defaults are fine.
	//   • GameInfo subclasses — server-side game logic, don't replicate.
	//
	// Order: hierarchy-grouped so a scan top-to-bottom mirrors the class tree.
	static const std::vector<ClassConfig> kConfigs = {

		// ═══════════════════════════════════════════════════════════════════
		// HUD / GAME-STATE CRITICAL  (the symptom-fix targets — HP, team
		// colors, game info on HUD all live here)
		// ═══════════════════════════════════════════════════════════════════
		{ "GameReplicationInfo Engine.Default__GameReplicationInfo",         { &kHudInfo                                          } },
		{ "TgRepInfo_Game TgGame.Default__TgRepInfo_Game",                   { &kHudInfo                                          } },
		{ "TgRepInfo_GameOpenWorld TgGame.Default__TgRepInfo_GameOpenWorld", { &kHudInfo                                          } },
		{ "PlayerReplicationInfo Engine.Default__PlayerReplicationInfo",     { &kHudInfo                                          } },
		{ "TgRepInfo_Player TgGame.Default__TgRepInfo_Player",               { &kHudInfo, &kRepInstigator                         } },
		{ "TeamInfo Engine.Default__TeamInfo",                               { &kHudInfo                                          } },
		{ "TgRepInfo_TaskForce TgGame.Default__TgRepInfo_TaskForce",         { &kHudInfo                                          } },
		{ "TgRepInfo_Deployable TgGame.Default__TgRepInfo_Deployable",       { &kHudInfo                                          } },
		{ "TgRepInfo_Beacon TgGame.Default__TgRepInfo_Beacon",               { &kHudInfo                                          } },

		// ═══════════════════════════════════════════════════════════════════
		// CONTROLLERS
		// ═══════════════════════════════════════════════════════════════════
		{ "PlayerController Engine.Default__PlayerController",               { &kPrioHigh, &kFreqMed                              } },
		{ "TgPlayerController TgGame.Default__TgPlayerController",           { &kPrioHigh, &kFreqMed                              } },
		// AIControllers don't replicate to clients in this engine, but
		// keeping them here as a no-op anchor in case that ever changes.
		// (Empty list = "documented as considered, no overrides applied.")
		{ "TgAIController TgGame.Default__TgAIController",                   { }                                                  } ,

		// ═══════════════════════════════════════════════════════════════════
		// PAWNS  —  baseline applies to every replicable pawn class. Engine
		// clamps NetPriority→3 + NetUpdateFreq→100 on possess, so the CDO
		// numbers here are the pre-possess / between-possess values.
		// ═══════════════════════════════════════════════════════════════════

		// ── Engine base + TgPawn root ─────────────────────────────────────
		{ "Pawn Engine.Default__Pawn",                                       { &kPawnFamily                                       } },
		{ "TgPawn TgGame.Default__TgPawn",                                   { &kPawnFamily, &kTgPawnFamily                       } },

		// ── Vehicles (extend Pawn) ────────────────────────────────────────
		{ "Vehicle Engine.Default__Vehicle",                                 { &kPawnFamily                                       } },
		{ "SVehicle Engine.Default__SVehicle",                               { &kPawnFamily                                       } },
		{ "GameVehicle GameFramework.Default__GameVehicle",                  { &kPawnFamily                                       } },

		// ── Humanoid line: TgPawn_Character + subclasses ──────────────────
		{ "TgPawn_Character TgGame.Default__TgPawn_Character",               { &kPawnFamily, &kTgPawnFamily                       } },
		{ "TgPawn_AndroidMinion TgGame.Default__TgPawn_AndroidMinion",       { &kPawnFamily, &kTgPawnFamily                       } },
		{ "TgPawn_Brawler TgGame.Default__TgPawn_Brawler",                   { &kPawnFamily, &kTgPawnFamily                       } },
		{ "TgPawn_CTR TgGame.Default__TgPawn_CTR",                           { &kPawnFamily, &kTgPawnFamily                       } },
		{ "TgPawn_ColonyEye TgGame.Default__TgPawn_ColonyEye",               { &kPawnFamily, &kTgPawnFamily                       } },
		{ "TgPawn_Dismantler TgGame.Default__TgPawn_Dismantler",             { &kPawnFamily, &kTgPawnFamily                       } },
		{ "TgPawn_DuneCommander TgGame.Default__TgPawn_DuneCommander",       { &kPawnFamily, &kTgPawnFamily                       } },
		{ "TgPawn_Elite_Alchemist TgGame.Default__TgPawn_Elite_Alchemist",   { &kPawnFamily, &kTgPawnFamily                       } },
		{ "TgPawn_Elite_Assassin TgGame.Default__TgPawn_Elite_Assassin",     { &kPawnFamily, &kTgPawnFamily                       } },
		{ "TgPawn_Hunter TgGame.Default__TgPawn_Hunter",                     { &kPawnFamily, &kTgPawnFamily                       } },
		{ "TgPawn_Inquisitor TgGame.Default__TgPawn_Inquisitor",             { &kPawnFamily, &kTgPawnFamily                       } },
		{ "TgPawn_Interact_NPC TgGame.Default__TgPawn_Interact_NPC",         { &kPawnFamily, &kTgPawnFamily, &kAlwaysRelevant     } },
		{ "TgPawn_Juggernaut TgGame.Default__TgPawn_Juggernaut",             { &kPawnFamily, &kTgPawnFamily                       } },
		{ "TgPawn_Marauder TgGame.Default__TgPawn_Marauder",                 { &kPawnFamily, &kTgPawnFamily                       } },
		{ "TgPawn_NPC TgGame.Default__TgPawn_NPC",                           { &kPawnFamily, &kTgPawnFamily, &kAlwaysRelevant     } },
		{ "TgPawn_Raptor TgGame.Default__TgPawn_Raptor",                     { &kPawnFamily, &kTgPawnFamily                       } },
		{ "TgPawn_Reaper TgGame.Default__TgPawn_Reaper",                     { &kPawnFamily, &kTgPawnFamily                       } },
		{ "TgPawn_RecursiveSpawner TgGame.Default__TgPawn_RecursiveSpawner", { &kPawnFamily, &kTgPawnFamily                       } },
		{ "TgPawn_Sniper TgGame.Default__TgPawn_Sniper",                     { &kPawnFamily, &kTgPawnFamily                       } },
		{ "TgPawn_SonoranCommander TgGame.Default__TgPawn_SonoranCommander", { &kPawnFamily, &kTgPawnFamily                       } },
		{ "TgPawn_Switchblade TgGame.Default__TgPawn_Switchblade",           { &kPawnFamily, &kTgPawnFamily                       } },
		{ "TgPawn_ThinkTank TgGame.Default__TgPawn_ThinkTank",               { &kPawnFamily, &kTgPawnFamily                       } },
		{ "TgPawn_UberWalker TgGame.Default__TgPawn_UberWalker",             { &kPawnFamily, &kTgPawnFamily                       } },
		{ "TgPawn_Vanguard TgGame.Default__TgPawn_Vanguard",                 { &kPawnFamily, &kTgPawnFamily                       } },
		{ "TgPawn_Vulcan TgGame.Default__TgPawn_Vulcan",                     { &kPawnFamily, &kTgPawnFamily                       } },
		{ "TgPawn_Warlord TgGame.Default__TgPawn_Warlord",                   { &kPawnFamily, &kTgPawnFamily                       } },

		// ── Bosses (UC defaults bAlwaysRelevant=true; explicit here) ──────
		{ "TgPawn_Boss TgGame.Default__TgPawn_Boss",                         { &kPawnFamily, &kTgPawnFamily, &kAlwaysRelevant     } },
		{ "TgPawn_Boss_Destroyer TgGame.Default__TgPawn_Boss_Destroyer",     { &kPawnFamily, &kTgPawnFamily, &kAlwaysRelevant     } },
		{ "TgPawn_FlyingBoss TgGame.Default__TgPawn_FlyingBoss",             { &kPawnFamily, &kTgPawnFamily, &kAlwaysRelevant     } },
		// Guardian: UC declares bAlwaysRelevant=false; don't add the toggle.
		{ "TgPawn_Guardian TgGame.Default__TgPawn_Guardian",                 { &kPawnFamily, &kTgPawnFamily                       } },

		// ── Robots + Scanners (UC defaults bAlwaysRelevant=true on Scanner)
		{ "TgPawn_Robot TgGame.Default__TgPawn_Robot",                       { &kPawnFamily, &kTgPawnFamily                       } },
		{ "TgPawn_Scanner TgGame.Default__TgPawn_Scanner",                   { &kPawnFamily, &kTgPawnFamily, &kAlwaysRelevant     } },
		{ "TgPawn_ScannerRecursive TgGame.Default__TgPawn_ScannerRecursive", { &kPawnFamily, &kTgPawnFamily, &kAlwaysRelevant     } },
		{ "TgPawn_TreadRobot TgGame.Default__TgPawn_TreadRobot",             { &kPawnFamily, &kTgPawnFamily                       } },
		{ "TgPawn_Detonator TgGame.Default__TgPawn_Detonator",               { &kPawnFamily, &kTgPawnFamily                       } },
		{ "TgPawn_Destructible TgGame.Default__TgPawn_Destructible",         { &kPawnFamily, &kTgPawnFamily                       } },
		{ "TgPawn_Iris TgGame.Default__TgPawn_Iris",                         { &kPawnFamily, &kTgPawnFamily                       } },

		// ── Flying / Hover ─────────────────────────────────────────────────
		{ "TgPawn_Hover TgGame.Default__TgPawn_Hover",                       { &kPawnFamily, &kTgPawnFamily                       } },
		{ "TgPawn_HoverShieldSphere TgGame.Default__TgPawn_HoverShieldSphere", { &kPawnFamily, &kTgPawnFamily                     } },
		{ "TgPawn_AttackTransport TgGame.Default__TgPawn_AttackTransport",   { &kPawnFamily, &kTgPawnFamily                       } },
		{ "TgPawn_NewWasp TgGame.Default__TgPawn_NewWasp",                   { &kPawnFamily, &kTgPawnFamily                       } },
		{ "TgPawn_WaspSpawner TgGame.Default__TgPawn_WaspSpawner",           { &kPawnFamily, &kTgPawnFamily                       } },
		{ "TgPawn_AVCompositeWalker TgGame.Default__TgPawn_AVCompositeWalker", { &kPawnFamily, &kTgPawnFamily                     } },

		// ── Siege family ───────────────────────────────────────────────────
		{ "TgPawn_Siege TgGame.Default__TgPawn_Siege",                       { &kPawnFamily, &kTgPawnFamily                       } },
		{ "TgPawn_SiegeBarrage TgGame.Default__TgPawn_SiegeBarrage",         { &kPawnFamily, &kTgPawnFamily                       } },
		{ "TgPawn_SiegeHover TgGame.Default__TgPawn_SiegeHover",             { &kPawnFamily, &kTgPawnFamily                       } },
		{ "TgPawn_SiegeRapidFire TgGame.Default__TgPawn_SiegeRapidFire",     { &kPawnFamily, &kTgPawnFamily                       } },

		// ── Turret family (stationary; client trace-relevance is fine) ────
		{ "TgPawn_Turret TgGame.Default__TgPawn_Turret",                     { &kPawnFamily, &kTgPawnFamily, &kTgPawnTurretFamily                       } },
		{ "TgPawn_TurretAVAFlak TgGame.Default__TgPawn_TurretAVAFlak",       { &kPawnFamily, &kTgPawnFamily, &kTgPawnTurretFamily                       } },
		{ "TgPawn_TurretAVARocket TgGame.Default__TgPawn_TurretAVARocket",   { &kPawnFamily, &kTgPawnFamily, &kTgPawnTurretFamily                       } },
		{ "TgPawn_TurretFlak TgGame.Default__TgPawn_TurretFlak",             { &kPawnFamily, &kTgPawnFamily, &kTgPawnTurretFamily                       } },
		{ "TgPawn_TurretFlame TgGame.Default__TgPawn_TurretFlame",           { &kPawnFamily, &kTgPawnFamily, &kTgPawnTurretFamily                       } },
		{ "TgPawn_TurretPlasma TgGame.Default__TgPawn_TurretPlasma",         { &kPawnFamily, &kTgPawnFamily, &kTgPawnTurretFamily                       } },

		// ── Spawned helpers / pets / ambush ────────────────────────────────
		{ "TgPawn_Ambush TgGame.Default__TgPawn_Ambush",                     { &kPawnFamily, &kTgPawnFamily                       } },
		{ "TgPawn_Tentacle TgGame.Default__TgPawn_Tentacle",                 { &kPawnFamily, &kTgPawnFamily                       } },
		{ "TgPawn_EscortRobot TgGame.Default__TgPawn_EscortRobot",           { &kPawnFamily, &kTgPawnFamily                       } },
		{ "TgPawn_GroundPetA TgGame.Default__TgPawn_GroundPetA",             { &kPawnFamily, &kTgPawnFamily                       } },
		{ "TgPawn_VanityPet TgGame.Default__TgPawn_VanityPet",               { &kPawnFamily, &kTgPawnFamily                       } },
		{ "TgPawn_Remote TgGame.Default__TgPawn_Remote",                     { &kPawnFamily, &kTgPawnFamily                       } },
		{ "TgPawn_Widow TgGame.Default__TgPawn_Widow",                       { &kPawnFamily, &kTgPawnFamily                       } },

		// ═══════════════════════════════════════════════════════════════════
		// INVENTORY  /  WEAPONS  (TgDevice family)
		// ═══════════════════════════════════════════════════════════════════
		{ "Inventory Engine.Default__Inventory",                             { &kInventoryFamily                                  } },
		{ "InventoryManager Engine.Default__InventoryManager",               { &kInventoryFamily, &kRepInstigator                 } },
		{ "TgInventoryManager TgGame.Default__TgInventoryManager",           { &kInventoryFamily, &kRepInstigator, &kBroadcast    } },
		{ "Weapon Engine.Default__Weapon",                                   { &kDeviceFamily                                     } },
		{ "TgDevice TgGame.Default__TgDevice",                               { &kDeviceFamily                                     } },
		{ "TgDevice_Grenade TgGame.Default__TgDevice_Grenade",               { &kDeviceFamily                                     } },
		{ "TgDevice_HitPulse TgGame.Default__TgDevice_HitPulse",             { &kDeviceFamily                                     } },
		{ "TgDevice_MeleeDualWield TgGame.Default__TgDevice_MeleeDualWield", { &kDeviceFamily                                     } },
		{ "TgDevice_Morale TgGame.Default__TgDevice_Morale",                 { &kDeviceFamily                                     } },
		{ "TgDevice_NewMelee TgGame.Default__TgDevice_NewMelee",             { &kDeviceFamily                                     } },
		{ "TgDevice_NewRange TgGame.Default__TgDevice_NewRange",             { &kDeviceFamily                                     } },

		// ═══════════════════════════════════════════════════════════════════
		// DEPLOYABLES
		// ═══════════════════════════════════════════════════════════════════
		{ "TgDeployable TgGame.Default__TgDeployable",                       { &kDeployableFamily                                 } },
		{ "TgDeploy_Artillery TgGame.Default__TgDeploy_Artillery",           { &kDeployableFamily, &kAlwaysRelevant               } },
		{ "TgDeploy_Beacon TgGame.Default__TgDeploy_Beacon",                 { &kDeployableFamily, &kAlwaysRelevant               } },
		{ "TgDeploy_BeaconEntrance TgGame.Default__TgDeploy_BeaconEntrance", { &kDeployableFamily, &kAlwaysRelevant               } },
		{ "TgDeploy_DestructibleCover TgGame.Default__TgDeploy_DestructibleCover", { &kDeployableFamily, &kAlwaysRelevant         } },
		{ "TgDeploy_ForceField TgGame.Default__TgDeploy_ForceField",         { &kDeployableFamily, &kAlwaysRelevant               } },
		{ "TgDeploy_Sensor TgGame.Default__TgDeploy_Sensor",                 { &kDeployableFamily                                 } },
		{ "TgDeploy_SweepSensor TgGame.Default__TgDeploy_SweepSensor",       { &kDeployableFamily, &kUpdateSimPos                 } },

		// ═══════════════════════════════════════════════════════════════════
		// PROJECTILES
		// ═══════════════════════════════════════════════════════════════════
		{ "Projectile Engine.Default__Projectile",                           { &kProjectileFamily                                 } },
		{ "TgProjectile TgGame.Default__TgProjectile",                       { &kProjectileFamily                                 } },
		{ "TgProj_Bot TgGame.Default__TgProj_Bot",                           { &kProjectileFamily                                 } },
		{ "TgProj_Bounce TgGame.Default__TgProj_Bounce",                     { &kProjectileFamily, &kPersistent                   } },
		{ "TgProj_Deployable TgGame.Default__TgProj_Deployable",             { &kProjectileFamily, &kPersistent, &kUpdateSimPos   } },
		{ "TgProj_FreeGrenade TgGame.Default__TgProj_FreeGrenade",           { &kProjectileFamily, &kPersistent                   } },
		{ "TgProj_Grapple TgGame.Default__TgProj_Grapple",                   { &kProjectileFamily                                 } },
		{ "TgProj_Grenade TgGame.Default__TgProj_Grenade",                   { &kProjectileFamily, &kPersistent                   } },
		{ "TgProj_Missile TgGame.Default__TgProj_Missile",                   { &kProjectileFamily, &kPersistent                   } },
		{ "TgProj_Mortar TgGame.Default__TgProj_Mortar",                     { &kProjectileFamily, &kPersistent                   } },
		{ "TgProj_Net TgGame.Default__TgProj_Net",                           { &kProjectileFamily                                 } },
		{ "TgProj_Rocket TgGame.Default__TgProj_Rocket",                     { &kProjectileFamily                                 } },
		{ "TgProj_StraightTeleporter TgGame.Default__TgProj_StraightTeleporter", { &kProjectileFamily                             } },
		{ "TgProj_Teleporter TgGame.Default__TgProj_Teleporter",             { &kProjectileFamily                                 } },

		// ═══════════════════════════════════════════════════════════════════
		// MISSION OBJECTIVES  (drive map kismet; clients need consistent view)
		// ═══════════════════════════════════════════════════════════════════
		{ "TgMissionObjective TgGame.Default__TgMissionObjective",           { &kPrioMed, &kFreqSlow, &kAlwaysRelevant            } },
		{ "TgBaseObjective_CTFBot TgGame.Default__TgBaseObjective_CTFBot",   { &kPrioMed, &kFreqSlow, &kAlwaysRelevant            } },
		{ "TgBaseObjective_KOTH TgGame.Default__TgBaseObjective_KOTH",       { &kPrioMed, &kFreqSlow, &kAlwaysRelevant            } },
		{ "TgMissionObjective_Bot TgGame.Default__TgMissionObjective_Bot",   { &kPrioMed, &kFreqSlow, &kAlwaysRelevant            } },
		{ "TgMissionObjective_Escort TgGame.Default__TgMissionObjective_Escort", { &kPrioMed, &kFreqSlow, &kAlwaysRelevant        } },
		{ "TgMissionObjective_Kismet TgGame.Default__TgMissionObjective_Kismet", { &kPrioMed, &kFreqSlow, &kAlwaysRelevant        } },
		{ "TgMissionObjective_Proximity TgGame.Default__TgMissionObjective_Proximity", { &kPrioMed, &kFreqSlow, &kAlwaysRelevant  } },
		{ "TgObjectiveAssignment TgGame.Default__TgObjectiveAssignment",     { &kPrioMed, &kFreqSlow, &kAlwaysRelevant            } },

		// ═══════════════════════════════════════════════════════════════════
		// EFFECTS / SINGLETONS / MANAGERS
		// ═══════════════════════════════════════════════════════════════════
		{ "TgEffectManager TgGame.Default__TgEffectManager",                 { &kEffectManagerFamily                              } },
		{ "TgTimerManager TgGame.Default__TgTimerManager",                   { &kSingletonFamily                                  } },
		{ "TgWindManager TgGame.Default__TgWindManager",                     { &kSingletonFamily, &kUpdateSimPos                  } },
		{ "TgTeamBeaconManager TgGame.Default__TgTeamBeaconManager",         { &kSingletonFamily                                  } },

		// ═══════════════════════════════════════════════════════════════════
		// WORLD ACTORS / VOLUMES / DOORS / DECORATIONS
		// ═══════════════════════════════════════════════════════════════════
		{ "TgDroppedItem TgGame.Default__TgDroppedItem",                     { &kPrioLow, &kFreqMed, &kAlwaysRelevant             } },
		{ "TgChestActor TgGame.Default__TgChestActor",                       { &kPrioLow, &kFreqSlow, &kAlwaysRelevant            } },
		{ "TgHexLandMarkActor TgGame.Default__TgHexLandMarkActor",           { &kPrioLow, &kAlwaysRelevant                        } },
		{ "TgTeamMarker TgGame.Default__TgTeamMarker",                       { &kPrioLow, &kOwnerOnly                             } },
		{ "TgDoor TgGame.Default__TgDoor",                                   { &kAlwaysRelevant                                   } },
		{ "TgDoorMarker TgGame.Default__TgDoorMarker",                       { &kFreqSlow, &kAlwaysRelevant                       } },
		{ "TgSkydiveTarget TgGame.Default__TgSkydiveTarget",                 { &kAlwaysRelevant                                   } },
		{ "TgMeshAssembly TgGame.Default__TgMeshAssembly",                   { &kAlwaysRelevant                                   } },
		{ "TgObjectiveAttachActor TgGame.Default__TgObjectiveAttachActor",   { &kUpdateSimPos                                     } },
		{ "TgDynamicSMActor TgGame.Default__TgDynamicSMActor",               { &kAlwaysRelevant                                   } },
		{ "TgDynamicDestructible TgGame.Default__TgDynamicDestructible",     { &kPrioLow                                          } },
		{ "TgFracturedStaticMeshActor TgGame.Default__TgFracturedStaticMeshActor", { &kAlwaysRelevant, &kNoRepMovement            } },
		{ "TgInterpActor TgGame.Default__TgInterpActor",                     { }                                                  } ,
		{ "TgFlagCaptureVolume TgGame.Default__TgFlagCaptureVolume",         { &kStaticVolumeFamily, &kNoSkipActorProps           } },
		{ "TgModifyPawnPropertiesVolume TgGame.Default__TgModifyPawnPropertiesVolume", { &kNoSkipActorProps                       } },
		{ "TgSkydivingVolume TgGame.Default__TgSkydivingVolume",             { &kNoSkipActorProps                                 } },
	};
	return kConfigs;
}


// ═══════════════════════════════════════════════════════════════════════════
// MERGE + APPLY
//
// Overlay = "later wins per field"; absent (nullopt) fields don't override.
// Apply writes only the fields that the merged result has set, leaving the
// rest of the CDO alone.
// ═══════════════════════════════════════════════════════════════════════════

static void OverlayWith(RepDefault& dst, const RepDefault& src) {
	if (src.NetPriority.has_value())                   dst.NetPriority                   = src.NetPriority;
	if (src.NetUpdateFrequency.has_value())            dst.NetUpdateFrequency            = src.NetUpdateFrequency;
	if (src.bAlwaysRelevant.has_value())               dst.bAlwaysRelevant               = src.bAlwaysRelevant;
	if (src.bOnlyRelevantToOwner.has_value())          dst.bOnlyRelevantToOwner          = src.bOnlyRelevantToOwner;
	if (src.bReplicateMovement.has_value())            dst.bReplicateMovement            = src.bReplicateMovement;
	if (src.bOnlyDirtyReplication.has_value())         dst.bOnlyDirtyReplication         = src.bOnlyDirtyReplication;
	if (src.bNetTemporary.has_value())                 dst.bNetTemporary                 = src.bNetTemporary;
	if (src.bSkipActorPropertyReplication.has_value()) dst.bSkipActorPropertyReplication = src.bSkipActorPropertyReplication;
	if (src.bReplicateInstigator.has_value())          dst.bReplicateInstigator          = src.bReplicateInstigator;
	if (src.bUpdateSimulatedPosition.has_value())      dst.bUpdateSimulatedPosition      = src.bUpdateSimulatedPosition;
	if (src.bReplicateRigidBodyLocation.has_value())   dst.bReplicateRigidBodyLocation   = src.bReplicateRigidBodyLocation;
	if (src.AlwaysRelevantDistanceSquared.has_value()) dst.AlwaysRelevantDistanceSquared = src.AlwaysRelevantDistanceSquared;
}

static void ApplyToCdo(AActor* cdo, const RepDefault& m) {
	if (m.NetPriority.has_value())                   cdo->NetPriority                   = *m.NetPriority;
	if (m.NetUpdateFrequency.has_value())            cdo->NetUpdateFrequency            = *m.NetUpdateFrequency;
	if (m.bAlwaysRelevant.has_value())               cdo->bAlwaysRelevant               = *m.bAlwaysRelevant               ? 1 : 0;
	if (m.bOnlyRelevantToOwner.has_value())          cdo->bOnlyRelevantToOwner          = *m.bOnlyRelevantToOwner          ? 1 : 0;
	if (m.bReplicateMovement.has_value())            cdo->bReplicateMovement            = *m.bReplicateMovement            ? 1 : 0;
	if (m.bOnlyDirtyReplication.has_value())         cdo->bOnlyDirtyReplication         = *m.bOnlyDirtyReplication         ? 1 : 0;
	if (m.bNetTemporary.has_value())                 cdo->bNetTemporary                 = *m.bNetTemporary                 ? 1 : 0;
	if (m.bSkipActorPropertyReplication.has_value()) cdo->bSkipActorPropertyReplication = *m.bSkipActorPropertyReplication ? 1 : 0;
	if (m.bReplicateInstigator.has_value())          cdo->bReplicateInstigator          = *m.bReplicateInstigator          ? 1 : 0;
	if (m.bUpdateSimulatedPosition.has_value())      cdo->bUpdateSimulatedPosition      = *m.bUpdateSimulatedPosition      ? 1 : 0;
	if (m.bReplicateRigidBodyLocation.has_value())   cdo->bReplicateRigidBodyLocation   = *m.bReplicateRigidBodyLocation   ? 1 : 0;
	if (m.AlwaysRelevantDistanceSquared.has_value()) {
		APawn* pcdo = (APawn*)cdo;
		pcdo->AlwaysRelevantDistanceSquared = *m.AlwaysRelevantDistanceSquared;
	}
}

} // namespace


void ReplicationDefaults::Apply() {
	int patched = 0;
	int missed  = 0;

	for (const auto& cfg : Configs()) {
		UObject* cdo = ObjectCache::Find(cfg.cdoFullName);
		if (!cdo) {
			Logger::Log("replication", "[ReplicationDefaults] MISS  %s\n", cfg.cdoFullName);
			++missed;
			continue;
		}

		RepDefault merged{};
		for (const RepDefault* preset : cfg.presets) {
			OverlayWith(merged, *preset);
		}
		ApplyToCdo(reinterpret_cast<AActor*>(cdo), merged);
		Logger::Log("replication", "[ReplicationDefaults] patched %s\n", cfg.cdoFullName);
		++patched;
	}

	Logger::Log("replication", "[ReplicationDefaults] done: patched=%d missed=%d (of %d configs)\n",
	            patched, missed, (int)Configs().size());
}
