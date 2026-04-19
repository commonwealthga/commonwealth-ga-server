#include "src/GameServer/TgGame/TgProj_Deployable/SpawnDeployable/TgProj_Deployable__SpawnDeployable.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/GameServer/Engine/Actor/SetTimer/Actor__SetTimer.hpp"
#include "src/Database/Database.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include "src/Utils/Macros.hpp"

namespace {
// deployable_id → class full name (literal). Cached to avoid DB round-trips
// on every spawn. The asm_data_set_deployables table is static reference data
// loaded from assembly.dat at startup; no invalidation needed.
std::unordered_map<int, const char*> g_deployableClassCache;
}

// Map class_res_id (FK into asm_data_set_resources) to the UC class full-name
// literal. Matches the SpawnBotById pattern. Only 9 distinct class_res_id
// values exist across all 184 deployable rows in the DB.
static const char* DeployableClassNameForResId(int class_res_id) {
	switch (class_res_id) {
		case 11:   return "Class TgGame.TgDeployable";
		case 221:  return "Class TgGame.TgDeploy_Beacon";
		case 225:  return "Class TgGame.TgDeploy_ForceField";
		case 468:  return "Class TgGame.TgDeploy_SweepSensor";
		case 1617: return "Class TgGame.TgDeploy_Sensor";
		case 1907: return "Class TgGame.TgDeploy_DestructibleCover";
		case 2120: return "Class TgGame.TgDeploy_BeaconEntrance";
		case 3335: return "Class TgGame.TgDeploy_PickupFlag";
		case 5926: return "Class TgGame.TgDeploy_Artillery";
		default:   return "Class TgGame.TgDeployable";
	}
}

const char* TgProj_Deployable__SpawnDeployable::GetDeployableClassName(int nDeployableId) {
	auto it = g_deployableClassCache.find(nDeployableId);
	if (it != g_deployableClassCache.end()) return it->second;

	int class_res_id = 11; // Fallback → base TgDeployable.
	sqlite3* db = Database::GetConnection();
	sqlite3_stmt* stmt;
	if (sqlite3_prepare_v2(db,
		"SELECT class_res_id FROM asm_data_set_deployables WHERE deployable_id = ? LIMIT 1",
		-1, &stmt, nullptr) == SQLITE_OK) {
		sqlite3_bind_int(stmt, 1, nDeployableId);
		if (sqlite3_step(stmt) == SQLITE_ROW) {
			if (sqlite3_column_type(stmt, 0) != SQLITE_NULL) {
				class_res_id = sqlite3_column_int(stmt, 0);
			}
		}
		sqlite3_finalize(stmt);
	}
	const char* name = DeployableClassNameForResId(class_res_id);
	g_deployableClassCache[nDeployableId] = name;
	return name;
}

namespace {
// Cylinder cache keyed by deployable_id — avoids re-running the DB join on
// every deploy. Stored as packed (radius, halfHeight) floats.
struct CylinderDims { float radius; float halfHeight; };
std::unordered_map<int, CylinderDims> g_deployableCylinderCache;
}

void TgProj_Deployable__SpawnDeployable::GetDeployableCollisionCylinder(
	int nDeployableId, float* outRadius, float* outHalfHeight)
{
	// UC default on Class TgGame.TgDeployable.CollisionCylinder: h=10, r=12.
	// This is what every deploy subclass inherits unless it overrides — and
	// none of them do (verified across TgDeploy_Beacon/Sensor/ForceField/
	// Artillery/DestructibleCover/BeaconEntrance/SweepSensor UC defaults).
	constexpr float kUCDefaultRadius     = 12.0f;
	constexpr float kUCDefaultHalfHeight = 10.0f;

	auto it = g_deployableCylinderCache.find(nDeployableId);
	if (it != g_deployableCylinderCache.end()) {
		*outRadius     = it->second.radius;
		*outHalfHeight = it->second.halfHeight;
		return;
	}

	float radius     = kUCDefaultRadius;
	float halfHeight = kUCDefaultHalfHeight;
	sqlite3* db = Database::GetConnection();
	if (db) {
		sqlite3_stmt* stmt = nullptr;
		if (sqlite3_prepare_v2(db,
			"SELECT am.collision_radius, am.collision_height "
			"FROM asm_data_set_deployables d "
			"JOIN asm_data_set_assembly_meshes am ON d.asm_id = am.asm_id "
			"WHERE d.deployable_id = ? LIMIT 1;",
			-1, &stmt, nullptr) == SQLITE_OK) {
			sqlite3_bind_int(stmt, 1, nDeployableId);
			if (sqlite3_step(stmt) == SQLITE_ROW) {
				float dbRadius = (float)sqlite3_column_double(stmt, 0);
				float dbHeight = (float)sqlite3_column_double(stmt, 1);
				// Only accept DB values when non-zero; zeros mean "no data"
				// (mines/grenades/sweep-sensors) → stick with the UC default.
				// DB stores FULL cylinder height while UE3/UC convention uses
				// half-height for UCylinderComponent::CollisionHeight — the
				// client does the same *0.5 when taking the mesh override (see
				// DAT_1168bac0 = 0.5f in UpdateDeployModeStatus).
				if (dbRadius > 0.0f) radius     = dbRadius;
				if (dbHeight > 0.0f) halfHeight = dbHeight * 0.5f;
			}
			sqlite3_finalize(stmt);
		}
	}

	g_deployableCylinderCache[nDeployableId] = { radius, halfHeight };
	*outRadius     = radius;
	*outHalfHeight = halfHeight;
}

bool TgProj_Deployable__SpawnDeployable::IsForceFieldDeployableId(int nDeployableId) {
	const char* clsName = GetDeployableClassName(nDeployableId);
	if (!clsName) return false;
	return strstr(clsName, "TgDeploy_ForceField") != nullptr;
}

void TgProj_Deployable__SpawnDeployable::GetTimerBombParams(
	int nDeployableId, float* outActivationSecs, float* outDamageRadiusUU)
{
	// Defaults when DB row is missing or prop not populated. Picked to match
	// the old hardcode (~EMP-bomb class values) so an unknown/corrupt bomb
	// still explodes with *some* radius rather than 0.
	constexpr float kDefaultActivation  = 3.0f;
	constexpr float kDefaultRadiusUU    = 320.0f;  // 20 ft × 16 uu/ft
	// DB stores DAMAGE_RADIUS in the same feet convention as PROXIMITY_DISTANCE;
	// ATgDeployable::ApplyProperty for prop 8 multiplies by 16 before writing
	// s_fProximityRadius — replicate that here so our server-side write lands
	// in the same units the client's proximity display consumes.
	constexpr float kFeetToUU = 16.0f;

	struct BombParams { float activation; float radius; };
	static std::unordered_map<int, BombParams> s_cache;
	auto it = s_cache.find(nDeployableId);
	if (it != s_cache.end()) {
		*outActivationSecs  = it->second.activation;
		*outDamageRadiusUU  = it->second.radius;
		return;
	}

	float activation = kDefaultActivation;
	float radiusUU   = kDefaultRadiusUU;
	sqlite3* db = Database::GetConnection();
	if (db) {
		// Join deployables → device_mode_properties to pull both props in one
		// round-trip.  GROUP BY prop_id collapses multi-mode devices — bombs
		// only have one mode anyway, but the bomb's device is sometimes shared
		// with a pickup/variant row; prefer the first seen value.
		sqlite3_stmt* stmt = nullptr;
		if (sqlite3_prepare_v2(db,
			"SELECT p.prop_id, MIN(p.base_value) "
			"FROM asm_data_set_deployables d "
			"JOIN asm_data_set_device_mode_properties p ON d.device_id = p.device_id "
			"WHERE d.deployable_id = ? AND p.prop_id IN (6, 7) "
			"GROUP BY p.prop_id;",
			-1, &stmt, nullptr) == SQLITE_OK) {
			sqlite3_bind_int(stmt, 1, nDeployableId);
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				int propId = sqlite3_column_int(stmt, 0);
				float val  = (float)sqlite3_column_double(stmt, 1);
				if (propId == 7 && val > 0.0f) activation = val;           // REMOTE_ACTIVATION_TIME (secs)
				if (propId == 6 && val > 0.0f) radiusUU   = val * kFeetToUU; // DAMAGE_RADIUS (feet → uu)
			}
			sqlite3_finalize(stmt);
		}
	}

	s_cache[nDeployableId] = { activation, radiusUU };
	*outActivationSecs  = activation;
	*outDamageRadiusUU  = radiusUU;
}

bool TgProj_Deployable__SpawnDeployable::IsTimerBombDeployableId(int nDeployableId) {
	// Cache keyed by id (positive for bomb, zero for non-bomb). Use a sentinel
	// in the existing cylinder cache? No — separate map to avoid ambiguity.
	static std::unordered_map<int, int> s_cache;  // value: 0 = non-bomb, 1 = bomb, -1 = uninit
	auto it = s_cache.find(nDeployableId);
	if (it != s_cache.end()) return it->second == 1;

	int bombFlag = 0;
	sqlite3* db = Database::GetConnection();
	if (db) {
		sqlite3_stmt* stmt = nullptr;
		if (sqlite3_prepare_v2(db,
			"SELECT show_countdown_timer_flag FROM asm_data_set_deployables "
			"WHERE deployable_id = ? LIMIT 1;",
			-1, &stmt, nullptr) == SQLITE_OK) {
			sqlite3_bind_int(stmt, 1, nDeployableId);
			if (sqlite3_step(stmt) == SQLITE_ROW) {
				bombFlag = sqlite3_column_int(stmt, 0);
			}
			sqlite3_finalize(stmt);
		}
	}
	s_cache[nDeployableId] = bombFlag;
	return bombFlag == 1;
}

bool TgProj_Deployable__SpawnDeployable::IsBeaconDeployable(ATgDeployable* Deployable) {
	if (!Deployable || !Deployable->Class) return false;
	const char* fn = Deployable->Class->GetFullName();
	if (!fn) return false;
	// Matches "Class TgGame.TgDeploy_Beacon" and any future subclass whose
	// full name still contains the "TgDeploy_Beacon" token (e.g. a subclassed
	// beacon variant). Deliberately excludes BeaconEntrance via boundary check.
	const char* m = strstr(fn, "TgDeploy_Beacon");
	if (!m) return false;
	// Reject BeaconEntrance (starts same way but is not a placeable beacon).
	return strstr(m, "BeaconEntrance") == nullptr;
}

// Shared spawn helper: spawn + initialise a deployable of the right class.
// Called from both SpawnDeployable (projectile path) and TgDeviceFire::Deploy (custom-fire path).
ATgDeployable* TgProj_Deployable__SpawnDeployable::SpawnDeployableActor(
	ATgPawn* pawn, int deployableId, FVector vLocation, FVector vNormal)
{
	if (!pawn) return nullptr;

	// Resolve UC class via DB lookup on asm_data_set_deployables.class_res_id.
	// Previously we reinterpreted cfg+0x10 as a wchar_t* class-name pointer and
	// wcscmp'd it — but cfg+0x10 is not a string pointer in this build (crash:
	// small-int value `0x1797` dereferenced as wchar_t*). The DB has the
	// authoritative mapping for all 184 deployable rows.
	const char* clsName = GetDeployableClassName(deployableId);
	UClass* cls = ClassPreloader::GetClass(clsName);
	bool bIsBeacon = (strcmp(clsName, "Class TgGame.TgDeploy_Beacon") == 0);

	// Ground-to-center lift, applied uniformly for every caller (instant-deploy
	// trace path AND projectile-impact path).  Callers pass a surface-level
	// point (trace ground contact or projectile impact); the deployable's Actor
	// Location must be at the cylinder CENTER, so lift by halfHeight plus a
	// small floor buffer (_DAT_1197edd8 = 5.0f in the client's placement trace;
	// likely the game's standard ground-penetration padding).  Without this
	// deployables spawned with half their cylinder below terrain.  Was
	// previously a hardcoded `vLocation.Z += 5.0f` beacon-only fudge.
	{
		float radius = 0.f, halfHeight = 0.f;
		GetDeployableCollisionCylinder(deployableId, &radius, &halfHeight);
		vLocation.Z += halfHeight + 5.0f;
	}

	Logger::Log(GetLogChannel(),
		"SpawnDeployableActor: class=%s deployableId=%d%s\n",
		clsName, deployableId,
		bIsBeacon ? " (beacon-register path)" : "");

	if (!cls) {
		Logger::Log(GetLogChannel(),
			"SpawnDeployableActor: class not preloaded (%s) — falling back to base TgDeployable\n",
			clsName);
		cls = ClassPreloader::GetTgDeployableClass();
		bIsBeacon = false;
	}
	if (!cls) {
		Logger::Log(GetLogChannel(), "SpawnDeployableActor: NULL class, aborting\n");
		return nullptr;
	}

	// Deployable facing: use the CONTROLLER's rotation (view/aim direction), not
	// the Pawn's body rotation.  Pawn->Rotation lags or snaps in 3rd-person movement
	// (body turns with movement, not with aim); Controller->Rotation tracks the
	// player's actual look direction at spawn time.  Zero out pitch/roll so deployables
	// (turrets in particular) stand upright on the ground.
	FRotator spawnRot = pawn->Controller ? pawn->Controller->Rotation : pawn->Rotation;
	spawnRot.Pitch = 0;
	spawnRot.Roll  = 0;

	ATgDeployable* Deployable = (ATgDeployable*)pawn->Spawn(
		cls,
		pawn,
		FName(),
		vLocation,
		spawnRot,
		nullptr,
		1
	);

	if (!Deployable) {
		Logger::Log(GetLogChannel(), "SpawnDeployableActor: Spawn returned null\n");
		return nullptr;
	}

	// Instigator propagation fix — matches the projectile pattern
	// (reference_projectile_instigator_propagation.md).  UE3's standard
	// execSpawn is supposed to propagate the spawning pawn as Instigator, but
	// this binary's copy doesn't (or gates it on a flag we don't set).  We see
	// Deployable->Instigator==null after Spawn even though the caller IS an
	// ATgPawn.  With Instigator null:
	//   - Client's team-color resolution falls back to "enemy" (the DRI's
	//     r_InstigatorInfo chain would cover this if the DRI replicated
	//     correctly, but an extra failsafe via the deployable's own Instigator
	//     never hurts).
	//   - Any server-side code reading `deployable->Instigator` as the damage
	//     attributor sees null → hit events attributed to "world".
	//
	// bReplicateInstigator=true is set on TgDeployable defaults (UC:2178), so
	// replication of this field is already wired — we just need it non-null
	// on the server side.  Owner was set via the Spawn parameter; belt-and-
	// braces re-check it here too.
	APawn* spawnedInstigator = Deployable->Instigator;
	AActor* spawnedOwner     = Deployable->Owner;
	if (!Deployable->Instigator) Deployable->Instigator = pawn;
	if (!Deployable->Owner)      Deployable->Owner      = pawn;
	Deployable->bNetDirty        = 1;
	Deployable->bForceNetUpdate  = 1;

	Logger::Log("team_colors",
		"[Instigator-fix] deployable=0x%p  Spawn returned Instigator=%p Owner=%p  → patched to Instigator=%p Owner=%p  (pawn=%p)\n",
		Deployable, spawnedInstigator, spawnedOwner,
		Deployable->Instigator, Deployable->Owner, pawn);

	ATgRepInfo_Player* pawnrep = (ATgRepInfo_Player*)pawn->PlayerReplicationInfo;

	Deployable->eventInitReplicationInfo();
	if (pawnrep && pawnrep->r_TaskForce) {
		Deployable->SetTaskForceNumber(pawnrep->r_TaskForce->r_nTaskForce);
	}

	// Team-ownership fix (long-standing bug: deployables rendered as enemy
	// to their own team, repair arm refused to target them).
	//
	// Client decides friendship via TgDeployable::IsFriendlyWithLocalPawn
	// (0x10a19460), which ultimately calls TgRepInfo_Game::GetTaskForceFor
	// (0x109f1fa0) on the deployable.  For a deployable, that function:
	//   iVar1 = deployable->r_DRI
	//   if (iVar1->r_bOwnedByTaskforce & 1) return iVar1->r_TaskforceInfo
	//   if (iVar1->r_InstigatorInfo != 0)   return iVar1->r_InstigatorInfo->r_TaskForce
	//   fall through to 0 (→ enemy)
	//
	// So **either** the r_bOwnedByTaskforce + r_TaskforceInfo path OR the
	// r_InstigatorInfo path will resolve the team.  We want both populated
	// for redundancy — bit-field writes don't always carry their specific
	// +0xAC dirty mask, so relying on only the primary path is fragile.
	//
	// Earlier attempt used TgRepInfo_Deployable::SetTaskForce (native
	// 0x109ee560), but that native explicitly **zeros r_InstigatorInfo**
	// when it writes, killing the fallback path.  UC InitReplicationInfo's
	// AddMinion branch (taken when s_DeployFactory is null — every player-
	// spawned deployable) leaves r_InstigatorInfo pointing at the owner's
	// PRI; we now preserve that AND explicitly set the primary path.
	if (Deployable->r_DRI && pawnrep && pawnrep->r_TaskForce) {
		ATgRepInfo_Deployable* dri = Deployable->r_DRI;
		dri->r_bOwnedByTaskforce = 1;
		dri->r_TaskforceInfo   = pawnrep->r_TaskForce;
		dri->r_InstigatorInfo  = pawnrep;
		// C++ writes bypass UC's per-property dirty-bit setters, so the engine's
		// differential replication may see "nothing dirty" and skip these fields.
		// Mirror what the native SetTaskForce does (`+0xAC |= 0x100000` for
		// r_TaskforceInfo), and force a full initial replication by restoring
		// bNetInitial so the engine sends all CPF_Net properties in the next
		// pass regardless of which specific bits are set.
		*(unsigned int*)((char*)dri + 0xAC) |= 0x100000;  // r_TaskforceInfo dirty
		dri->bNetInitial    = 1;
		dri->bNetDirty      = 1;
		dri->bForceNetUpdate = 1;

		// DRI rep-order fix — root cause of the lingering "enemy colors" bug.
		//
		// On the client, `dri.r_DeployableOwner` is an ObjectProperty that
		// points back to the deployable.  UE3 resolves ObjectProperty NetGUIDs
		// at serialization time: if the referenced actor hasn't been
		// replicated to the client yet, the reference serializes as null and
		// NEVER retries (the engine only re-sends a property when its value
		// changes, and the server-side pointer isn't changing).
		//
		// ReplicationInfo's default NetPriority (3.0) is HIGHER than
		// TgDeployable's (1.4), so the engine picks the DRI first in the
		// per-tick relevance queue.  Result: DRI's first bunch (which carries
		// r_DeployableOwner) is sent BEFORE the deployable has a channel →
		// r_DeployableOwner lands null on the client → `c_bReceivedOwner`
		// stays false → the DRI's own ReplicatedEvent handlers for
		// r_TaskforceInfo / r_bOwnedByTaskforce never call
		// `r_DeployableOwner.NotifyGroupChanged()`, so the material is never
		// recalculated once those fields finally arrive.  The initial
		// fallback (r_bInitialIsEnemy=1 → enemy) becomes permanent.
		//
		// Lowering DRI NetPriority below the deployable's forces the engine
		// to replicate the deployable first; its NetGUID is cached on the
		// client, and when the DRI's r_DeployableOwner is serialized a tick
		// later it resolves correctly → `c_bReceivedOwner` flips true →
		// subsequent r_TaskforceInfo / r_bOwnedByTaskforce repnotifies fire
		// NotifyGroupChanged → RecalculateMaterial runs with the correct
		// friendship answer.
		//
		// Value 1.0 comfortably beneath TgDeployable's 1.4 without starving
		// the DRI for bandwidth (NetUpdateFrequency kept at 30 Hz so it
		// still catches up within a frame of the deployable).
		dri->NetUpdateFrequency = 30.0f;
		dri->NetPriority        = 1.0f;   // < TgDeployable.NetPriority=1.4
		dri->bAlwaysRelevant    = 1;
		dri->bTearOff           = 0;
		dri->bOnlyDirtyReplication = 0;
		dri->bSkipActorPropertyReplication = 0;
		dri->Role               = 3;   // ROLE_Authority
		dri->RemoteRole         = 1;   // ROLE_SimulatedProxy
	}
	// r_bInitialIsEnemy chooses the initial material color BEFORE the DRI
	// has replicated.  Client's IsFriendlyWithLocalPawn short-circuits to
	// `!r_bInitialIsEnemy` when r_DRI is null:
	//   r_bInitialIsEnemy=0 → initial render = friendly (this setting)
	//   r_bInitialIsEnemy=1 → initial render = enemy (previous setting)
	//
	// We use 0 as the least-wrong default.  The DRI-based re-check chain
	// that's supposed to update the material once team fields replicate is
	// broken on this binary (UC's c_bReceivedOwner flips 0→1 so UC *is*
	// calling NotifyGroupChanged, but RecalculateMaterial never actually
	// flips the visual on the client — investigation pending).  With
	// r_bInitialIsEnemy=0 the user's own deployables render friendly as
	// expected; enemy-team viewers may see friendly briefly until their
	// DRI path catches up, which is acceptable until the deeper bug is
	// fixed.  Don't change this back without confirming the DRI-driven
	// material swap actually works client-side.
	Deployable->r_bInitialIsEnemy = 0;

	// Read-back diag: confirm the team-ownership fix actually landed on the
	// server and the DRI is in a replicate-able state. If r_bOwnedByTaskforce
	// stays 0, our SDK bit write isn't hitting the expected offset. If
	// r_TaskforceInfo stays null, SetTaskForce ProcessEvent dispatch failed
	// or silently no-op'd (e.g. UFunction cache miss). If Role != 3 /
	// RemoteRole == 0 on DRI, it won't replicate → client's r_DRI stays null
	// → IsFriendlyWithLocalPawn falls back to !r_bInitialIsEnemy = enemy.
	{
		ATgRepInfo_Deployable* dri = Deployable->r_DRI;
		if (!dri) {
			Logger::Log("team_colors",
				"[team-fix readback] DRI IS NULL after eventInitReplicationInfo — fix skipped\n");
		} else {
			ATgRepInfo_TaskForce* tfRead = dri->r_TaskforceInfo;
			ATgRepInfo_TaskForce* tfExpected = pawnrep ? pawnrep->r_TaskForce : nullptr;
			int tfExpNum = tfExpected ? tfExpected->r_nTaskForce : -1;
			int tfReadNum = tfRead ? tfRead->r_nTaskForce : -1;
			int attackersNum = GTeamsData.Attackers ? GTeamsData.Attackers->r_nTaskForce : -1;
			int defendersNum = GTeamsData.Defenders ? GTeamsData.Defenders->r_nTaskForce : -1;

			Logger::Log("team_colors",
				"[team-fix readback] deployable=0x%p class=%s\n"
				"                    DRI=0x%p  r_bOwnedByTaskforce=%d  r_TaskforceInfo=0x%p (num=%d)\n"
				"                    r_InstigatorInfo=0x%p  r_DeployableOwner=0x%p\n"
				"                    DRI.Role=%d  RemoteRole=%d  bAlwaysRelevant=%d  bNetInitial=%d  bNetDirty=%d\n"
				"                    expected tf=0x%p (num=%d)  pawnrep=0x%p\n"
				"                    GTeamsData.Attackers=0x%p (num=%d)  Defenders=0x%p (num=%d)\n"
				"                    deployable.r_bInitialIsEnemy=%d\n",
				Deployable,
				Deployable->Class ? Deployable->Class->GetFullName() : "<null>",
				dri, (int)dri->r_bOwnedByTaskforce, tfRead, tfReadNum,
				dri->r_InstigatorInfo, dri->r_DeployableOwner,
				(int)dri->Role, (int)dri->RemoteRole, (int)dri->bAlwaysRelevant,
				(int)dri->bNetInitial, (int)dri->bNetDirty,
				tfExpected, tfExpNum, pawnrep,
				GTeamsData.Attackers, attackersNum,
				GTeamsData.Defenders, defendersNum,
				(int)Deployable->r_bInitialIsEnemy);
		}
	}

	TARRAY_INIT(pawn, s_SelfDeployableList, ATgDeployable*, 0x1514, 255);
	TARRAY_ADD(s_SelfDeployableList, Deployable);

	// r_nDeployableId MUST be set before ApplyDeployableSetup/InitializeDefaultProps —
	// the native ApplyDeployableSetup uses it to look up the cfg; our
	// InitializeDefaultProps uses it to query asm_data_set_deployables for HP.
	Deployable->r_nDeployableId    = deployableId;
	Deployable->m_bInDestroyedState = 0;
	Deployable->r_bTakeDamage      = 1;
	Deployable->s_bIsActivated     = 1;
	Deployable->m_bIsDeployed      = 0;
	Deployable->bOnlyDirtyReplication = 0;
	Deployable->Role               = 3;
	Deployable->RemoteRole         = 1;
	Deployable->bNetInitial        = 1;
	Deployable->bNetDirty          = 1;
	Deployable->bForceNetUpdate    = 1;
	Deployable->bAlwaysRelevant    = 1;

	// Wire the effect-manager owner fields so applied effects know who to
	// report to / reverse against. The EffectManager subobject is auto-spawned
	// by UE3 as part of the actor's default-properties chain.
	if (Deployable->r_EffectManager) {
		Deployable->r_EffectManager->r_Owner    = (AActor*)Deployable;
		Deployable->r_EffectManager->SetOwner((AActor*)Deployable);
		Deployable->r_EffectManager->Base       = (AActor*)Deployable;
		Deployable->r_EffectManager->Role       = 3;
		Deployable->r_EffectManager->RemoteRole = 1;
		Deployable->r_EffectManager->bNetInitial = 1;
		Deployable->r_EffectManager->bNetDirty   = 1;
	}

	// Phase 2: drive the native setup chain. ApplyDeployableSetup looks up the
	// cfg via FN_LOOKUP_DEPLOYABLE_CFG(this->r_nDeployableId) and dispatches
	// vtable slot 250 (InitializeFromDeployableDat @ 0x10a243e0), which:
	//   - wires m_FireMode from cfg+0x28 (the deployable's own weapon/emitter)
	//   - loads c_Mesh (cfg+0x20) and m_DestroyedMesh
	//   - loads c_Form (cfg+0x2C), c_FireFx
	//   - loads m_EquipEffect (cfg+0xE0) — the passive aura, not yet applied
	//   - copies m_bConsumedOnFire, m_bDestroyOnOwnerDeathFlag bits from cfg
	//   - fires UC OnDeployableInitialize and post-init ProcessEvents
	// Only on the server — UC's ReplicatedEvent('r_nDeployableId') triggers
	// the client-side equivalent once the id replicates.
	//
	// Runs for all deployables. In an earlier iteration this crashed for
	// medical stations inside TgAssemblyMisc.cpp:253 (LoadObject → null →
	// CrashProgram("newObj")), which is why the TgAssemblyMisc__LoadAssetRefs
	// and Core__LoadObject diagnostic hooks exist. The crash stopped
	// reproducing once the spawn order was fixed (DB-based class resolution
	// + deferred InitializeDefaultProps + effect-manager owner wiring); the
	// hooks stay registered in case it re-surfaces.
	Deployable->ApplyDeployableSetup();

	// Phase 3: populate s_Properties from asm_data_set_deployables (HP, etc).
	// Our hook replaces the no-op native stub at 0x10a194d0. It's safe to
	// call even though UC PostBeginPlay already invoked it during Spawn() —
	// the hook early-returns when s_Properties is already populated.
	Deployable->InitializeDefaultProps();

	// Phase 4: fire the passive equip effect (e.g. station protections) if
	// ApplyDeployableSetup wired one. UC eventApplyEquipEffects runs
	// ProcessEffect(m_EquipEffect, false, self) → the effect manager applies
	// every effect in the group against the deployable as target.
	// m_EquipEffect is only populated when ApplyDeployableSetup ran, so this
	// currently only fires for beacons. Non-beacons get no passive equip
	// effect until Phase 2 can run for them.
	if (Deployable->m_EquipEffect) {
		Deployable->eventApplyEquipEffects();
	}

	Logger::Log(GetLogChannel(),
		"SpawnDeployableActor: spawned 0x%p deployableId=%d taskForce=%d hp=%d props=%d fireMode=%p equipEffect=%p mesh=%p\n",
		Deployable, deployableId,
		(pawnrep && pawnrep->r_TaskForce) ? pawnrep->r_TaskForce->r_nTaskForce : -1,
		Deployable->r_nHealth,
		Deployable->s_Properties.Data ? Deployable->s_Properties.Num() : -1,
		(void*)Deployable->m_FireMode,
		(void*)Deployable->m_EquipEffect,
		(void*)Deployable->c_Mesh);

	// Deploy-phase diagnostics: after ApplyDeployableSetup, UC's
	// Deploy.BeginState will read m_fDefaultDeployAnimLength (populated by
	// the native mesh loader from the asset's "Deploy" anim) and fall back
	// to it if r_fTimeToDeploySecs is 0. Print the key fields so we can see
	// whether the mesh loaded server-side.
	//
	// IMPORTANT: GetFullName() uses a shared static buffer — multiple calls
	// in one log statement clobber each other (reference_getfullname_shared_buffer).
	// Copy each result into its own std::string before formatting.
	auto readObjInfo = [](UObject* obj, std::string& outObj, std::string& outClass) {
		if (!obj) { outObj = "<null>"; outClass = "<null>"; return; }
		const char* oname = obj->GetFullName();
		outObj = oname ? oname : "<?>";
		if (obj->Class) {
			const char* cname = obj->Class->GetFullName();
			outClass = cname ? cname : "<?>";
		} else {
			outClass = "<null-class>";
		}
	};
	std::string meshObj, meshClass;
	std::string destroyedObj, destroyedClass;
	std::string formObj, formClass;
	std::string fireModeObj, fireModeClass;
	std::string equipEffectObj, equipEffectClass;
	readObjInfo((UObject*)Deployable->c_Mesh,         meshObj,        meshClass);
	readObjInfo((UObject*)Deployable->m_DestroyedMesh, destroyedObj,   destroyedClass);
	readObjInfo((UObject*)Deployable->c_Form,         formObj,        formClass);
	readObjInfo((UObject*)Deployable->m_FireMode,     fireModeObj,    fireModeClass);
	readObjInfo((UObject*)Deployable->m_EquipEffect,  equipEffectObj, equipEffectClass);

	Logger::Log("deploy_phase",
		"[Deploy-phase diag] deployableId=%d\n"
		"                    c_Mesh          = %p  obj='%s'  class='%s'\n"
		"                    m_DestroyedMesh = %p  obj='%s'  class='%s'\n"
		"                    c_Form          = %p  obj='%s'  class='%s'\n"
		"                    m_FireMode      = %p  obj='%s'  class='%s'\n"
		"                    m_EquipEffect   = %p  obj='%s'  class='%s'\n"
		"                    m_fDefaultDeployAnimLength = %.3f\n"
		"                    r_fTimeToDeploySecs        = %.3f\n"
		"                    r_fDeployRate              = %.3f\n"
		"                    m_bIsDeployed              = %d\n"
		"                    BuildPhase ready? %s\n",
		deployableId,
		(void*)Deployable->c_Mesh,          meshObj.c_str(),        meshClass.c_str(),
		(void*)Deployable->m_DestroyedMesh, destroyedObj.c_str(),   destroyedClass.c_str(),
		(void*)Deployable->c_Form,          formObj.c_str(),        formClass.c_str(),
		(void*)Deployable->m_FireMode,      fireModeObj.c_str(),    fireModeClass.c_str(),
		(void*)Deployable->m_EquipEffect,   equipEffectObj.c_str(), equipEffectClass.c_str(),
		Deployable->m_fDefaultDeployAnimLength,
		Deployable->r_fTimeToDeploySecs,
		Deployable->r_fDeployRate,
		(int)Deployable->m_bIsDeployed,
		(Deployable->m_fDefaultDeployAnimLength > 0.0f || Deployable->r_fTimeToDeploySecs > 0.0f)
			? "YES — UC Deploy state will tick"
			: "NO  — UC Deploy state will instant-complete (anim length = 0)");

	// For beacons specifically: register with the TgTeamBeaconManager so
	// GetBeacon() returns non-null → BeaconEntrance::HasExit() returns true →
	// teleport fires; and r_BeaconStatus replicates for client UI. This block
	// is gated strictly on beacon-class membership now, not on "specialised
	// class exists" — Sensors/ForceFields/etc. no longer corrupt r_Beacon.
	if (bIsBeacon) {
		ATgTeamBeaconManager* beaconMgr = pawnrep ? pawnrep->r_TaskForce->r_BeaconManager : nullptr;
		if (beaconMgr) {
			// Invalidate any previously deployed beacon so the old pointer doesn't linger.
			if (beaconMgr->r_Beacon && beaconMgr->r_Beacon != (ATgDeploy_Beacon*)Deployable) {
				beaconMgr->r_Beacon->m_bInDestroyedState = 1;
			}

			// Set s_DeployFactory before calling RegisterBeacon so CheckBeacon passes inside it.
			if (beaconMgr->s_BeaconFactoryList.Num() > 0) {
				Deployable->s_DeployFactory = (ATgActorFactory*)beaconMgr->s_BeaconFactoryList.Data[0];
			} else {
				Logger::Log(GetLogChannel(), "SpawnDeployableActor: WARNING no TgBeaconFactory in level\n");
			}

			Deployable->m_bInDestroyedState = 0;
			beaconMgr->r_Beacon       = (ATgDeploy_Beacon*)Deployable;
			beaconMgr->r_BeaconHolder = nullptr;
			beaconMgr->bNetDirty      = 1;
			beaconMgr->bForceNetUpdate = 1;

			beaconMgr->RegisterBeacon((ATgDeploy_Beacon*)Deployable, 1);

			// r_DRI team-ownership already wired above (general path applies
			// r_bOwnedByTaskforce=1 + SetTaskForce + r_bInitialIsEnemy=1 for
			// every deployable). RegisterBeacon may have mutated DRI state —
			// force another net update pass so the updated beacon-manager
			// fields replicate alongside.
			Deployable->bNetDirty          = 1;
			Deployable->bForceNetUpdate    = 1;

			Logger::Log(GetLogChannel(),
				"SpawnDeployableActor: registered beacon 0x%p with BeaconManager 0x%p, factory=0x%p, status=%d\n",
				Deployable, beaconMgr, Deployable->s_DeployFactory, (int)beaconMgr->r_BeaconStatus);
		} else {
			Logger::Log(GetLogChannel(), "SpawnDeployableActor: WARNING no BeaconManager for taskForce\n");
		}
	}

	// Timer-bomb pragmatic wiring: the real bomb pipeline would set
	// s_fActivationTime / s_fPersistTime / r_nTickingTime via native config
	// driven by asm data, then let UC's Active state transition through
	// DeviceBuildup → DeviceFiring → FireAmmunitionDeployable for the
	// explosion.  None of that happens on our server because (a) the source
	// data for activation time isn't populated in the DB rows we've found, and
	// (b) UC state machine bytecode on bomb spawn isn't reliably driven.
	//
	// Minimum viable fix: hardcode a 3-second countdown, set the replicated
	// fields so the client shows the ticking HUD and renders the bomb, then
	// SetTimer('StartFire') to let UC's state machine take the explosion path
	// after the delay.  If that UC chain doesn't fire an effect server-side,
	// we iterate — but first we see whether it does.
	//
	// Discriminator: show_countdown_timer_flag=1 in asm_data_set_deployables.
	// Covers EMP Bomb, Shatter Bomb, Fire Bomb, Venom Bomb, Graviton Bomb.
	// Excludes mines (trigger on proximity), stations, beacons, deconstructor.
	if (IsTimerBombDeployableId(deployableId)) {
		// Pull per-bomb params from asm_data_set_device_mode_properties via the
		// deployable's device (show_countdown_timer_flag=1 in deployables, then
		// prop 7 = REMOTE_ACTIVATION_TIME, prop 6 = DAMAGE_RADIUS in feet).
		// Most bombs land at (3s, 20ft = 320uu); Shatter Bomb 5s/40ft; AVA
		// bomb 10s/125ft.  Earlier hardcode was (3s, 384uu), which was close
		// for EMP-class bombs only.
		float activationSecs = 0.f, radiusUU = 0.f;
		GetTimerBombParams(deployableId, &activationSecs, &radiusUU);

		Deployable->r_nTickingTime        = (int)activationSecs;
		Deployable->c_fStartTickingTime   = 0.0f;     // client repnotify sets this from WorldInfo.TimeSeconds
		Deployable->s_fActivationTime     = activationSecs;
		// Proximity radius — prop 8 (TGPID_PROXIMITY_DISTANCE) is the one field
		// ApplyProperty mirrors to engine storage; GetTimerBombParams already
		// baked the ×16 feet→uu scale so this is raw UU.
		Deployable->s_fProximityRadius       = radiusUU;
		Deployable->r_fClientProximityRadius = radiusUU;

		Deployable->bNetDirty       = 1;
		Deployable->bForceNetUpdate = 1;

		// SetTimer('StartFire', activationSecs, non-looping) — UC's StartFire
		// transitions to DeviceBuildup → DeviceFiring where
		// m_FireMode.ApplyEffectType(self, 263) applies the explosion to actors
		// in radius.  If UC ApplyEffectType runs on server (likely, since it's
		// marked ROLE_Authority), the explosion damages victims.  Route through
		// Actor__SetTimer (rather than the SDK wrapper) so the FName packing
		// matches what the native expects — SDK bitfield/FName ABI has been
		// unreliable in this binary.
		Actor__SetTimer::SetTimer(Deployable, activationSecs, false, FName("StartFire"), nullptr);

		Logger::Log("bomb",
			"[bomb armed] deployableId=%d  activationTime=%.2fs  radius=%.0fuu (DB-driven)\n"
			"             SetTimer('StartFire', %.2fs) scheduled\n",
			deployableId, activationSecs, radiusUU, activationSecs);
	}

	// Bomb diagnostic: after all setup is done, dump bomb-specific replicated
	// state (tick time, destroy flags, mesh, fire mode, life span).  If bombs
	// vanish visually, one of these is almost certainly uninitialised — the
	// tick system never starts, LifeSpan goes to 0 immediately, or the mesh
	// fails to load.
	Logger::Log("bomb",
		"[bomb final state] deployable=0x%p id=%d class=%s\n"
		"                   r_nTickingTime=%d  c_fStartTickingTime=%.2f\n"
		"                   r_bInitialIsEnemy=%d  r_nReplicateDestroyIt=%d  r_bDelayDeployed=%d\n"
		"                   m_bInDestroyedState=%d  m_bIsDeployed=%d  s_bIsActivated=%d\n"
		"                   m_FireMode=0x%p  c_Mesh=0x%p  m_EquipEffect=0x%p\n"
		"                   LifeSpan=%.2f  m_fLifeAfterDeathSecs=%.2f\n"
		"                   r_fClientProximityRadius=%.2f  s_fProximityRadius=%.2f\n",
		Deployable, Deployable->r_nDeployableId,
		Deployable->Class ? Deployable->Class->GetFullName() : "<null>",
		Deployable->r_nTickingTime, Deployable->c_fStartTickingTime,
		(int)Deployable->r_bInitialIsEnemy,
		Deployable->r_nReplicateDestroyIt,
		(int)Deployable->r_bDelayDeployed,
		(int)Deployable->m_bInDestroyedState,
		(int)Deployable->m_bIsDeployed,
		(int)Deployable->s_bIsActivated,
		(void*)Deployable->m_FireMode,
		(void*)Deployable->c_Mesh,
		(void*)Deployable->m_EquipEffect,
		Deployable->LifeSpan,
		Deployable->m_fLifeAfterDeathSecs,
		Deployable->r_fClientProximityRadius,
		Deployable->s_fProximityRadius);

	// Second readback — after ApplyDeployableSetup, eventApplyEquipEffects,
	// and BeaconFactory registration.  Any of those could silently re-invoke
	// SetTaskForce (zeroing r_InstigatorInfo) or reset the ownership bit.
	// Compare against the first readback to localise the mutation.
	if (Deployable->r_DRI) {
		ATgRepInfo_Deployable* dri = Deployable->r_DRI;
		Logger::Log("team_colors",
			"[team-fix readback END] deployable=0x%p (after all init steps)\n"
			"                    DRI=0x%p  r_bOwnedByTaskforce=%d  r_TaskforceInfo=0x%p\n"
			"                    r_InstigatorInfo=0x%p  r_DeployableOwner=0x%p\n"
			"                    DRI.Role=%d RemoteRole=%d bAlwaysRelevant=%d bNetInitial=%d bNetDirty=%d\n"
			"                    deployable.r_bInitialIsEnemy=%d  r_DRI(ptr)=0x%p\n",
			Deployable,
			dri, (int)dri->r_bOwnedByTaskforce, dri->r_TaskforceInfo,
			dri->r_InstigatorInfo, dri->r_DeployableOwner,
			(int)dri->Role, (int)dri->RemoteRole, (int)dri->bAlwaysRelevant,
			(int)dri->bNetInitial, (int)dri->bNetDirty,
			(int)Deployable->r_bInitialIsEnemy, Deployable->r_DRI);
	}

	return Deployable;
}

// Called by TgProj_Deployable::SpawnTheDeployable when a deployable projectile lands.
ATgDeployable* __fastcall TgProj_Deployable__SpawnDeployable::Call(
	ATgProj_Deployable* pThis, void* edx,
	FVector vLocation, AActor* TargetActor, FVector vNormal)
{
	// Bomb diagnostic: log every projectile->deployable hook entry. If this
	// never fires when a bomb lands, the projectile isn't reaching the native
	// SpawnDeployable path — either no projectile was server-spawned or it's
	// a different projectile class (TgProj_Grenade etc.) whose HitWall doesn't
	// call SpawnDeployable.
	Logger::Log("bomb",
		"[SpawnDeployable entry] pThis=0x%p  targetActor=0x%p  loc=(%.1f,%.1f,%.1f)  pThis.class=%s\n",
		pThis, TargetActor, vLocation.X, vLocation.Y, vLocation.Z,
		(pThis && pThis->Class) ? pThis->Class->GetFullName() : "<null>");

	if (!pThis) return nullptr;

	ATgPawn* pawn = (ATgPawn*)pThis->Instigator;
	if (!pawn) {
		Logger::Log("bomb", "[SpawnDeployable] null Instigator — bailing\n");
		Logger::Log(GetLogChannel(), "TgProj_Deployable::SpawnDeployable: null Instigator\n");
		return nullptr;
	}

	// Resolve the deployable_id. Two sources depending on how the projectile
	// was configured:
	//   (a) ATgProjectile::s_nSpawnDeployableId — set by InitializeProjectile
	//       from asm_data_set_projectiles.spawn_deployable_id. Used by bombs
	//       (TgProj_Deployable with spawn_deployable_id directly on the
	//       projectile row).
	//   (b) FireMode setup +0x28 — used by beacons/other deployables whose
	//       fire mode carries the deployable id.
	// Try (a) first since it's the projectile's own authoritative field; fall
	// back to (b) for cases where the projectile config didn't set it.
	int deployableId = pThis->s_nSpawnDeployableId;
	if (deployableId == 0) {
		UTgDeviceFire* fireMode = pThis->s_LastDefaultMode;
		if (fireMode && fireMode->m_pFireModeSetup.Dummy) {
			deployableId = *(int*)((char*)fireMode->m_pFireModeSetup.Dummy + 0x28);
			Logger::Log("bomb",
				"[SpawnDeployable] deployableId resolved via fire-mode setup: %d\n",
				deployableId);
		}
	} else {
		Logger::Log("bomb",
			"[SpawnDeployable] deployableId resolved via s_nSpawnDeployableId: %d\n",
			deployableId);
	}
	if (deployableId == 0) {
		Logger::Log("bomb", "[SpawnDeployable] no deployableId from either source — bailing\n");
		Logger::Log(GetLogChannel(), "TgProj_Deployable::SpawnDeployable: no deployableId\n");
		return nullptr;
	}

	Logger::Log("bomb",
		"[SpawnDeployable] resolved deployableId=%d for pawn=%s — calling SpawnDeployableActor\n",
		deployableId, pawn->GetFullName());
	Logger::Log(GetLogChannel(), "TgProj_Deployable::SpawnDeployable: pawn=%s deployableId=%d loc=(%.0f,%.0f,%.0f)\n",
		pawn->GetFullName(), deployableId, vLocation.X, vLocation.Y, vLocation.Z);

	ATgDeployable* result = SpawnDeployableActor(pawn, deployableId, vLocation, vNormal);

	Logger::Log("bomb",
		"[SpawnDeployable] SpawnDeployableActor returned 0x%p (class=%s)\n",
		result,
		(result && result->Class) ? result->Class->GetFullName() : "<null>");

	return result;
}
