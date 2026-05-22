#include "src/GameServer/TgGame/TgProj_Deployable/SpawnDeployable/TgProj_Deployable__SpawnDeployable.hpp"
#include "src/GameServer/TgGame/TgProj_Deployable/SpawnDeployable/ApplyPlayerModsToDeployable.hpp"
#include "src/GameServer/TgGame/TgTeamBeaconManager/BeaconSdkSafe/BeaconSdkSafe.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/GameServer/Engine/Actor/SetTimer/Actor__SetTimer.hpp"
#include "src/Database/Database.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace {
// Binary entry point for `AActor::SetCollisionFromCollisionType`.  Reads
// `this->CollisionType` (offset 0x93) and rewrites the collision flag bits
// on `this->CollisionComponent` to match the engine's per-mode pattern,
// then runs ConditionalDetach + ConditionalUpdateComponents to re-register
// the primitives in the spatial query system with the new flags.  See UE3
// source UnActor.cpp:2094 for the canonical implementation — the binary
// matches it byte-for-byte at this address.
//
// We need this to undo the side-effect of `ApplyDeployableSetup`: that
// native invokes `UpdateTargetCylinder`, which sets
// `CollisionType = COLLIDE_BlockAllButWeapons` (clearing
// `CollisionComponent->BlockZeroExtent`) — making the cylinder invisible
// to hitscan weapon traces.  After ApplyDeployableSetup we set
// `CollisionType` back to `COLLIDE_CustomDefault` (0) and re-run this
// function to restore the CDO-default flag pattern (BlockZeroExtent=true).
using SetCollisionFromCollisionType_Fn = void(__thiscall*)(AActor*);
inline void SetCollisionFromCollisionType(AActor* a) {
	reinterpret_cast<SetCollisionFromCollisionType_Fn>(0x10c65910)(a);
}
} // namespace

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

bool TgProj_Deployable__SpawnDeployable::IsDomeShieldDeployableId(int nDeployableId) {
	static std::unordered_map<int, bool> s_cache;
	auto it = s_cache.find(nDeployableId);
	if (it != s_cache.end()) return it->second;

	bool isDome = false;
	sqlite3* db = Database::GetConnection();
	if (db) {
		sqlite3_stmt* stmt = nullptr;
		// Mesh-name prefix is the cleanest discriminator: `DEV_ForceField_Dome_*`
		// is reserved for the dome shield meshes (957 Large, 956 Medium, 955
		// Small) and never appears on wall meshes (`DEV_ForceField_Wall_*`).
		// Attack type and target type don't separate dome from wall — they both
		// use 342/214/0.
		int rc = sqlite3_prepare_v2(db,
			"SELECT 1 FROM asm_data_set_deployables d "
			"JOIN asm_data_set_assembly_meshes am ON am.asm_id = d.asm_id "
			"WHERE d.deployable_id = ? AND am.name LIKE 'DEV_ForceField_Dome%' "
			"LIMIT 1",
			-1, &stmt, nullptr);
		if (rc == SQLITE_OK) {
			sqlite3_bind_int(stmt, 1, nDeployableId);
			isDome = (sqlite3_step(stmt) == SQLITE_ROW);
			sqlite3_finalize(stmt);
		}
	}

	s_cache[nDeployableId] = isDome;
	return isDome;
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

float TgProj_Deployable__SpawnDeployable::GetDeployTimeSecs(int nDeployableId) {
	// Stations/turrets: explicit prop 279 (5–15s). Bombs/mines/force fields:
	// fall back to 0.1s so UC's Deploy.BeginState completes effectively at
	// once — matches AVA Bomb's explicit 0.1s in the DB. Without an explicit
	// override, UC reads m_fDefaultDeployAnimLength which is similar across
	// all deployables and drowns out the per-type tuning.
	constexpr float kDefaultDeployTimeSecs = 0.1f;

	static std::unordered_map<int, float> s_cache;
	auto it = s_cache.find(nDeployableId);
	if (it != s_cache.end()) return it->second;

	float secs = kDefaultDeployTimeSecs;
	sqlite3* db = Database::GetConnection();
	if (db) {
		// Prop 279 lives on the *thrower* device-mode (the player's deploy
		// action), looked up by deployable_id via the m2m table. MIN() collapses
		// duplicates that come from a thrower device with multiple modes
		// targeting the same deployable.
		sqlite3_stmt* stmt = nullptr;
		if (sqlite3_prepare_v2(db,
			"SELECT MIN(p.base_value) "
			"FROM asm_data_set_devices_data_set_device_modes m "
			"JOIN asm_data_set_device_mode_properties p "
			"  ON p.device_id = m.device_id AND p.device_mode_id = m.device_mode_id "
			"WHERE m.deployable_id = ? AND p.prop_id = 279;",
			-1, &stmt, nullptr) == SQLITE_OK) {
			sqlite3_bind_int(stmt, 1, nDeployableId);
			if (sqlite3_step(stmt) == SQLITE_ROW &&
				sqlite3_column_type(stmt, 0) != SQLITE_NULL) {
				float v = (float)sqlite3_column_double(stmt, 0);
				if (v > 0.0f) secs = v;
			}
			sqlite3_finalize(stmt);
		}
	}

	s_cache[nDeployableId] = secs;
	return secs;
}

float TgProj_Deployable__SpawnDeployable::GetPetDeployTimeSecs(int nBotId) {
	// Personal turret = 1s; bigger emplaced turrets 25–60s. Default 2s when
	// missing — matches the prior SpawnPet hardcode used as a "no anim length
	// available" floor. Cache per bot_id.
	constexpr float kDefaultPetDeployTimeSecs = 2.0f;

	static std::unordered_map<int, float> s_cache;
	auto it = s_cache.find(nBotId);
	if (it != s_cache.end()) return it->second;

	float secs = kDefaultPetDeployTimeSecs;
	sqlite3* db = Database::GetConnection();
	if (db) {
		sqlite3_stmt* stmt = nullptr;
		if (sqlite3_prepare_v2(db,
			"SELECT MIN(p.base_value) "
			"FROM asm_data_set_devices_data_set_device_modes m "
			"JOIN asm_data_set_device_mode_properties p "
			"  ON p.device_id = m.device_id AND p.device_mode_id = m.device_mode_id "
			"WHERE m.bot_id = ? AND p.prop_id = 279;",
			-1, &stmt, nullptr) == SQLITE_OK) {
			sqlite3_bind_int(stmt, 1, nBotId);
			if (sqlite3_step(stmt) == SQLITE_ROW &&
				sqlite3_column_type(stmt, 0) != SQLITE_NULL) {
				float v = (float)sqlite3_column_double(stmt, 0);
				if (v > 0.0f) secs = v;
			}
			sqlite3_finalize(stmt);
		}
	}

	s_cache[nBotId] = secs;
	return secs;
}

float TgProj_Deployable__SpawnDeployable::ApplyDeployTimeBuff(ATgPawn* pawn, float baseSecs, int deviceInstId) {
	if (!pawn || baseSecs <= 0.0f) return baseSecs;

	// GetBuffedProperty (TgPawn vtable[0x55C], FUN_109d7ff0). Direct call rather
	// than SDK wrapper for the same reason TgPawn__ApplyBuff calls GetBuffIndex
	// directly: avoid the wrapper's parm-struct shenanigans. Signature mirrors
	// the one used in CloneEffectGroup's [DAMAGE-BUFF] / [EFFECT-BUFF] blocks.
	typedef void(__fastcall* GetBuffedPropertyFn)(
		ATgPawn*, void*, unsigned char,
		int, int, int, int, int, float, float*, void*);
	static const GetBuffedPropertyFn GetBuffedPropertyNative =
		(GetBuffedPropertyFn)0x109d7ff0;

	float buffedSecs = baseSecs;
	GetBuffedPropertyNative(
		pawn, /*edx=*/nullptr,
		/*eRequestContext=*/3,        // BUFF_DEVICE — fire-mode prop reads
		/*nPropId=*/279,              // Time To Deploy (secs)
		/*nReqCategoryCode=*/0,
		/*nReqSkillId=*/0,
		/*nReqDeviceInstId=*/deviceInstId,
		/*bUsePotencyModifier=*/0,
		/*fBaseValue=*/baseSecs,
		/*fBuffedValue=*/&buffedSecs,
		/*Effect=*/nullptr);

	if (buffedSecs != baseSecs) {
		Logger::Log("inventory",
			"[DEPLOY-TIME] pawn=%p prop279  base=%.3fs -> buffed=%.3fs (×%.3f)\n",
			(void*)pawn, baseSecs, buffedSecs,
			(baseSecs > 0.0f) ? buffedSecs / baseSecs : 1.0f);
	}
	return buffedSecs;
}

bool TgProj_Deployable__SpawnDeployable::IsDestructibleDeployableId(int nDeployableId) {
	// Cache: 0 = HP=0 (indestructible), 1 = HP>0 (destructible).
	static std::unordered_map<int, int> s_cache;
	auto it = s_cache.find(nDeployableId);
	if (it != s_cache.end()) return it->second == 1;

	int destructible = 1;  // assume HP>0 if DB lookup misses
	sqlite3* db = Database::GetConnection();
	if (db) {
		sqlite3_stmt* stmt = nullptr;
		if (sqlite3_prepare_v2(db,
			"SELECT health FROM asm_data_set_deployables WHERE deployable_id = ? LIMIT 1;",
			-1, &stmt, nullptr) == SQLITE_OK) {
			sqlite3_bind_int(stmt, 1, nDeployableId);
			if (sqlite3_step(stmt) == SQLITE_ROW) {
				const int hp = sqlite3_column_int(stmt, 0);
				destructible = (hp > 0) ? 1 : 0;
			}
			sqlite3_finalize(stmt);
		}
	}
	s_cache[nDeployableId] = destructible;
	return destructible == 1;
}

int TgProj_Deployable__SpawnDeployable::GetMaxDeployablesOut(int device_mode_id) {
	// -1 sentinel = uninit; 0 = no limit (DB row absent); positive = configured limit.
	static std::unordered_map<int, int> s_cache;
	auto it = s_cache.find(device_mode_id);
	if (it != s_cache.end()) return it->second;

	int limit = 0;
	sqlite3* db = Database::GetConnection();
	if (db) {
		sqlite3_stmt* stmt = nullptr;
		if (sqlite3_prepare_v2(db,
			"SELECT base_value FROM asm_data_set_device_mode_properties "
			"WHERE device_mode_id = ? AND prop_id = 154 LIMIT 1;",
			-1, &stmt, nullptr) == SQLITE_OK) {
			sqlite3_bind_int(stmt, 1, device_mode_id);
			if (sqlite3_step(stmt) == SQLITE_ROW &&
				sqlite3_column_type(stmt, 0) != SQLITE_NULL) {
				const float v = (float)sqlite3_column_double(stmt, 0);
				if (v > 0.0f) limit = (int)v;
			}
			sqlite3_finalize(stmt);
		}
	}
	s_cache[device_mode_id] = limit;
	return limit;
}

void TgProj_Deployable__SpawnDeployable::CompactDeployableList(ATgPawn* pawn) {
	if (!pawn) return;
	auto& arr = pawn->s_SelfDeployableList;
	int write = 0;
	for (int read = 0; read < arr.Count; ++read) {
		ATgDeployable* dep = arr.Data[read];
		// Drop null entries (allocator slack) and already-destroyed deployables
		// (the actor still exists until LifeSpan expires, but it shouldn't
		// count toward the live-deployable limit).
		if (!dep) continue;
		if (dep->m_bInDestroyedState) continue;
		if (dep->bDeleteMe) continue;
		arr.Data[write++] = dep;
	}
	// Zero out the freed tail so stale pointers don't survive between compacts
	// in case anything reads past Count by accident.
	for (int i = write; i < arr.Count; ++i) arr.Data[i] = nullptr;
	arr.Count = write;
}

void TgProj_Deployable__SpawnDeployable::EnforceDeployableLimit(
	ATgPawn* pawn, ATgDevice* sourceDevice, UTgDeviceFire* sourceFireMode)
{
	if (!pawn || !sourceDevice || !sourceFireMode) return;

	const int device_mode_id = sourceFireMode->m_nId;
	const int limit = GetMaxDeployablesOut(device_mode_id);
	if (limit <= 0) return;  // no DB row → no limit

	CompactDeployableList(pawn);

	auto& arr = pawn->s_SelfDeployableList;

	// Count live deployables sharing the same source device. "Same device"
	// matches the user-observable rule: deploying a second medical station
	// destroys the first, but a medical station + power station coexist
	// (different devices, separate counts). Match by pointer identity since
	// each player has at most one device of each kind equipped.
	auto matches = [&](ATgDeployable* d) -> bool {
		return d && d->r_Owner == sourceDevice;
	};

	int liveCount = 0;
	for (int i = 0; i < arr.Count; ++i) if (matches(arr.Data[i])) ++liveCount;

	// Already at or above limit → destroy the oldest (lowest-index) matching
	// entries until adding the new one will leave us at exactly `limit`.
	int toKill = (liveCount + 1) - limit;
	if (toKill <= 0) return;

	Logger::Log(GetLogChannel(),
		"[DeployableLimit] pawn=0x%p device=0x%p mode_id=%d limit=%d live=%d killing oldest %d\n",
		pawn, sourceDevice, device_mode_id, limit, liveCount, toKill);

	for (int i = 0; i < arr.Count && toKill > 0; ++i) {
		ATgDeployable* dep = arr.Data[i];
		if (!matches(dep)) continue;

		Logger::Log(GetLogChannel(),
			"[DeployableLimit]   destroying [%d] 0x%p deployableId=%d\n",
			i, dep, dep->r_nDeployableId);

		// `eventDestroyIt(false)` runs UC's full destroy event: stops fire,
		// sets LifeSpan, swaps to destroyed mesh, flips m_bInDestroyedState,
		// and bumps r_nReplicateDestroyIt so the client repnotify fires.
		// Match KillDeployables' approach so behavior is consistent across
		// owner-death cleanup and limit-enforcement cleanup.
		dep->eventDestroyIt(0 /* bSkipFx */);
		--toKill;
	}
}

void TgProj_Deployable__SpawnDeployable::RegisterDeployableInGRI(ATgDeployable* dep) {
	if (!dep) return;
	AWorldInfo* worldInfo = dep->WorldInfo;
	if (!worldInfo || !worldInfo->GRI) return;
	ATgRepInfo_Game* gri = (ATgRepInfo_Game*)worldInfo->GRI;

	// Compact in place: drop nulls and already-destroyed entries before the
	// append, so the global list reflects only live deployables. TArray::Add
	// now routes through GAllocator::Realloc (matches the engine's allocator),
	// so we can grow naturally instead of relying on a fixed preallocated cap
	// — the previous libc-malloc + 255-slot-cap pattern was a workaround for
	// the cross-allocator bug that's since been fixed at the SDK level.
	int liveCount = 0;
	for (int read = 0; read < gri->m_Deployables.Count; ++read) {
		ATgDeployable* d = gri->m_Deployables.Data[read];
		if (!d) continue;
		if (d->m_bInDestroyedState) continue;
		if (d->bDeleteMe) continue;
		if (d == dep) continue;  // dedupe — defensive against double-register
		gri->m_Deployables.Data[liveCount++] = d;
	}
	gri->m_Deployables.Count = liveCount;

	gri->m_Deployables.Add(dep);

	// Deliberately NOT setting gri->bNetDirty / bForceNetUpdate here. The
	// optimized rep list walks the array by content-equality each tick, so
	// changes propagate naturally.

	Logger::Log(GetLogChannel(),
		"[GRI.m_Deployables] registered 0x%p deployableId=%d  list count=%d\n",
		dep, dep->r_nDeployableId, gri->m_Deployables.Count);
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
	ATgPawn* pawn, int deployableId, FVector vLocation, FVector vNormal,
	ATgDevice* sourceDevice, UTgDeviceFire* sourceFireMode)
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
	//
	// We also keep the values around (`cylRadius`, `cylHalfHeight`) so we can
	// resize the spawned actor's CollisionComponent after Spawn — the UC CDO
	// default is 12r×10h, far smaller than every real station's mesh, so
	// weapon traces and walking collision miss the visible mesh entirely
	// (no damage/heal/repair-arm trace ever reaches the deployable as a
	// target).
	float cylRadius = 0.f, cylHalfHeight = 0.f;
	GetDeployableCollisionCylinder(deployableId, &cylRadius, &cylHalfHeight);

	// Dome shield bubbles spawn CENTERED on the pawn — `vLocation` from the
	// caller already IS `pawn->Location` (cylinder center, roughly waist-
	// height). Lifting by halfHeight would push the sphere's center way above
	// the pawn's head, making the bubble float overhead instead of engulfing
	// them. The lift exists for trace-based placements where `vLocation` is a
	// ground contact point and the actor's cylinder center needs to clear the
	// terrain.
	const bool isDomeShield = IsDomeShieldDeployableId(deployableId);
	if (!isDomeShield) {
		vLocation.Z += cylHalfHeight + 5.0f;
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

	// r_nDeployableId MUST be set BEFORE eventInitReplicationInfo() runs
	// further down — UC's TgDeployable.InitReplicationInfo copies
	// `r_nDeployableId` into the freshly-spawned DRI's `r_nDeployableId`,
	// and the HUD's device-bar healthbar lookup (FindSpawnedPet @
	// 0x114de150) matches by `DRI->r_nDeployableId == fireMode.deployable_id`.
	// Previous order set this AFTER eventInitReplicationInfo, so the DRI
	// inherited 0 → match always failed → no healthbar appeared.
	// (The later assignment further down is now redundant but harmless;
	// kept to preserve the "set before ApplyDeployableSetup" invariant.)
	Deployable->r_nDeployableId = deployableId;

	// Cylinder resize.
	//
	// The UC CDO sets CollisionRadius=12 / CollisionHeight=10, which is tiny
	// relative to the visible mesh (per `asm_data_set_assembly_meshes`:
	// medstation 28×10, power station 18×12). Resize to the per-deployable
	// dimensions so the cylinder matches the visible footprint — the cfg
	// values were already pulled into `cylRadius`/`cylHalfHeight` above.
	//
	// `AActor::SetCollisionSize` is the canonical entry point: it updates
	// CollisionRadius + CollisionHeight on the primary CollisionComponent and
	// schedules a deferred reattach. Mirror to `m_TargetComponent` (the AI
	// aim cylinder) so the two stay in sync — the actor wrapper only
	// resizes CollisionComponent.
	if (cylRadius > 0.0f && cylHalfHeight > 0.0f) {
		Deployable->SetCollisionSize(cylRadius, cylHalfHeight);
		if (Deployable->m_TargetComponent) {
			Deployable->m_TargetComponent->SetCylinderSize(cylRadius, cylHalfHeight);
		}
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

	// Source-device attribution. UC SpawnDeployable native sets these via
	// TgDeployable::DeployedBy / DeployedByFireMode; without them, downstream
	// code reading deployable->r_Owner gets null:
	//   - per-type deployable limits can't find the existing instances to
	//     destroy (a player could deploy unlimited turrets/stations);
	//   - TgEffect::TrackStats source-trace can't tell that the bomb's
	//     explosion came from a morale device, so morale credits the firer
	//     for their own bomb's damage (infinite-loop ultimate refill);
	//   - block/counter-effect lookups that walk back to the firing device
	//     return null and silently no-op.
	// Both fields are CPF_Net so the client also sees the attribution.
	if (sourceDevice)   Deployable->r_Owner             = sourceDevice;
	if (sourceFireMode) Deployable->s_SpawnerDeviceMode = sourceFireMode;

	// (Identity stamping moved to end-of-function — m_FireMode isn't
	// initialized at this point; needs to run after all deploy setup.)

	Logger::Log("team_colors",
		"[Instigator-fix] deployable=0x%p  Spawn returned Instigator=%p Owner=%p  → patched to Instigator=%p Owner=%p  (pawn=%p)\n",
		Deployable, spawnedInstigator, spawnedOwner,
		Deployable->Instigator, Deployable->Owner, pawn);

	ATgRepInfo_Player* pawnrep = (ATgRepInfo_Player*)pawn->PlayerReplicationInfo;

	Deployable->eventInitReplicationInfo();
	if (pawnrep && pawnrep->r_TaskForce) {
		Deployable->SetTaskForceNumber(pawnrep->r_TaskForce->r_nTaskForce);
	}

	// Register in the global per-game deployable list. UC consumers
	// (TgBeaconFactory) and the HUD device-bar healthbar pipeline likely walk
	// this list to enumerate live deployables. The native that originally
	// populated it is stripped — see RegisterDeployableInGRI.
	RegisterDeployableInGRI(Deployable);

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

		// HUD healthbar diagnostic: dump (a) DRI's r_InstigatorInfo (should ==
		// pawnrep), and (b) the post-deploy state of pawnrep->m_DRIArray —
		// if Count incremented, server-side AddMinion (called from UC
		// InitReplicationInfo) ran successfully and the bookkeeping side is
		// healthy; if Count did NOT increment, the AddMinion path didn't fire
		// (likely because Instigator was null at InitReplicationInfo time, or
		// the cast inside AddMinion missed). Pair this with the client-side
		// `replicated_event` channel: if the server count is correct but no
		// `RE: var=r_InstigatorInfo` line appears on the client, the bug is
		// in DRI rep / RepNotify dispatch, not in our spawn flow.
		struct ArrPtr { void* Data; int Count; int Max; };
		ArrPtr* dris = reinterpret_cast<ArrPtr*>((char*)pawnrep + 0x2AC);
		Logger::Log("hud_healthbar",
			"[deploy] dep=0x%p deployableId=%d  dri=0x%p  r_InstigatorInfo=0x%p (expect %p)  "
			"pawnrep=0x%p pawnrep.m_DRIArray.Count=%d\n",
			Deployable, Deployable->r_nDeployableId, dri, dri->r_InstigatorInfo, pawnrep,
			pawnrep, dris->Count);
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
	if (Logger::IsChannelEnabled("team_colors")) {
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

	// Per-device deploy limit (TGPID_MAX_DEPLOYABLES_OUT, prop 154 in DB).
	// Walks pawn->s_SelfDeployableList, drops dead entries, and destroys the
	// oldest matching deployables when adding a new one would exceed the
	// configured limit. No-op when the firing fire mode has no prop-154 row.
	EnforceDeployableLimit(pawn, sourceDevice, sourceFireMode);

	pawn->s_SelfDeployableList.Add(Deployable);

	// r_nDeployableId MUST be set before ApplyDeployableSetup/InitializeDefaultProps —
	// the native ApplyDeployableSetup uses it to look up the cfg; our
	// InitializeDefaultProps uses it to query asm_data_set_deployables for HP.
	Deployable->r_nDeployableId    = deployableId;
	Deployable->m_bInDestroyedState = 0;
	// HP=0 deployables (Shatter Bomb, EMP Bomb, mines, etc) are not meant to
	// take damage in the seconds before they trigger — enemies should ignore
	// them entirely. Drive r_bTakeDamage from the DB's `health` column so the
	// asm-data-driven contract holds: HP>0 → destructible station/turret,
	// HP=0 → indestructible bomb/mine. Pairs with the collision gate further
	// down (we keep the cylinder in COLLIDE_BlockAllButWeapons so hitscan
	// passes through but movement still blocks).
	const bool destructible = IsDestructibleDeployableId(deployableId);
	Deployable->r_bTakeDamage      = destructible ? 1 : 0;
	Deployable->s_bIsActivated     = 1;
	Deployable->m_bIsDeployed      = 0;
	Deployable->r_nPhysicalType    = 861; // TgPawn.TG_PHYSICALITY_MECHANICAL
	Deployable->bOnlyDirtyReplication = 0;
	Deployable->Role               = 3;
	Deployable->RemoteRole         = 1;
	Deployable->bNetInitial        = 1;
	Deployable->bNetDirty          = 1;
	Deployable->bForceNetUpdate    = 1;
	Deployable->bAlwaysRelevant    = 1;

	// TgDeployable does NOT auto-spawn its r_EffectManager (unlike TgPawn,
	// which has explicit code in its spawn path:
	//   if (Role == ROLE_Authority && r_EffectManager == none) {
	//     r_EffectManager = Spawn(Class'TgGame.TgEffectManager', self,, vect(0,0,0));
	//     r_EffectManager.r_Owner = self;
	//     r_EffectManager.Instigator = self;
	//   }
	// — see TgPawn.uc).  TgDeployable.uc declares only `var TgEffectManager
	// r_EffectManager;` with no defaultsubobject and no PostBeginPlay spawn,
	// so it stays null on every plain TgDeployable instance.
	//
	// Without this, UC `TgDeployable.ProcessEffect` calls
	// `r_EffectManager.ProcessEffect(...)` which "Accessed None"-warns and
	// no-ops.  That's the entire reason hitscan damage and repair-arm beams
	// never apply to stations: trace lands, IsValidTarget=true, ApplyHit
	// runs, SubmitHitEffects looks up effect groups, SubmitEffect dispatches
	// to TgDeployable.ProcessEffect — and the chain dies right at the null
	// EffectManager dereference.  Turrets work because they're TgPawn.
	//
	// For HP=0 deployables (timer bombs, mines) we *deliberately* skip the
	// bootstrap so r_EffectManager stays null. UC's ProcessEffect gate on
	// these deployables checks `(Effect.m_nType != 264) || r_bTakeDamage`;
	// type-263 instants (e.g. Venom Bomb's −10 Health instant in cat=302
	// <Local>) pass that gate regardless of r_bTakeDamage and would land
	// damage. Letting the manager dereference null short-circuits ALL
	// incoming effects on bombs (the data design treats HP=0 as "no health,
	// no effects"), so a player's Venom Bomb explosion no longer destroys
	// the Shatter Bomb sitting next to it. HP>0 deployables — including the
	// Assassin's Timed Explosive — still get a manager and remain reachable
	// by Shatter Bomb's protection-stripping cat=653 effect group as
	// designed (the Assassin bomb's +100 Protection-AOE/Melee/Ranged from
	// equip-effect 13992 absorbs ordinary damage; only Shatter Bomb's
	// SUBTRACT 100 Protection effects punch through).
	if (!Deployable->r_EffectManager && destructible) {
		UClass* emClass = ClassPreloader::GetClass("Class TgGame.TgEffectManager");
		if (emClass) {
			FVector emLoc = {0.0f, 0.0f, 0.0f};
			Deployable->r_EffectManager = (ATgEffectManager*)Deployable->Spawn(
				emClass,
				Deployable,         // SpawnOwner
				FName(),
				emLoc,
				Deployable->Rotation,
				nullptr,            // ActorTemplate
				1                   // bNoCollisionFail
			);
		}
		Logger::Log(GetLogChannel(),
			"SpawnDeployableActor: r_EffectManager bootstrap — was null; %s%s\n",
			emClass ? "" : "EM CLASS NOT PRELOADED  ",
			Deployable->r_EffectManager ? "spawned OK" : "spawn FAILED");
	}

	// Wire the effect-manager owner fields so applied effects know who to
	// report to / reverse against.
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

	// Restore CollisionType to COLLIDE_CustomDefault and re-derive the
	// CollisionComponent flags from the class CDO.
	//
	// `ApplyDeployableSetup` (vtable slot 250 → InitializeFromDeployableDat @
	// 0x10a243e0) calls `UpdateTargetCylinder` (vtable slot 235), which writes
	// `CollisionType = COLLIDE_BlockAllButWeapons` (= 6) and runs
	// `SetCollisionFromCollisionType`.  That mode clears
	// `CollisionComponent->BlockZeroExtent` (per UE3's
	// UnActor.cpp:2147 — only BlockAll/BlockWeapons/BlockWeaponsKickable get
	// BlockZeroExtent=true).  In the shipped game this was balanced by a
	// custom hitscan path (TgDeviceFire's GetTraceImpact native) and/or by
	// per-cfg c_Mesh trace flags — neither of which we have on this
	// reverse-engineered server.  Net effect on us: hitscan weapon traces
	// (zero-extent line traces in UE3's octree, gated on
	// `TestPrimitive->BlockZeroExtent` per UnOctree.cpp:951) skip the
	// cylinder, and `c_Mesh`'s cfg-driven flags are all 0 too (verified via
	// `asm_data_set_assembly_meshes` — medstation/power station have
	// `block_zero_extent_flag=0`), so NO component on the deployable is a
	// valid hitscan target.  Result: bullets, melee, and repair-arm pulses
	// pass straight through.
	//
	// Reverting to COLLIDE_CustomDefault makes the engine copy the flag
	// pattern from the CDO's CollisionComponent (CylinderComponent CDO has
	// `BlockZeroExtent=true, BlockNonZeroExtent=true` — see
	// CylinderComponent.uc:24-25), which gives the cylinder
	// `BlockZeroExtent=true` and makes it a valid hitscan target.  Other
	// flags stay correct: bCollideActors=true, bBlockActors=true (player
	// movement still blocked), BlockNonZeroExtent=true (projectile sweeps
	// still hit), BlockRigidBody=false (was true under BlockAllButWeapons —
	// reverted, but this only affects rigid-body push interactions which the
	// stations don't use).
	//
	// We write the `CollisionType` field directly (offset 0x93 — see
	// Engine SDK header) instead of going through the binary's
	// `SetCollisionType` native (0x10c6cd20): that native ALSO updates
	// `ReplicatedCollisionType` and `bNetDirty`, which is exactly what we
	// want for the client side too — the value will replicate, and the
	// client's `PostNetReceive` will run the same `SetCollisionFromCollisionType`
	// to apply the same CDO flags locally.  But to keep this transparent
	// and self-contained, we set both fields explicitly and call
	// `SetCollisionFromCollisionType` directly.
	// Indestructible (HP=0) deployables — bombs, mines, etc — should NOT have
	// hitscan-targetable collision. Leave them in the COLLIDE_BlockAllButWeapons
	// state that ApplyDeployableSetup just installed (BlockZeroExtent=false on
	// the cylinder), so enemy bullets and AI line-traces pass straight through.
	// Movement still blocks (BlockNonZeroExtent stays true), and the bomb's own
	// proximity/explosion logic continues working since that's a UC-driven
	// timer, not a hit response. Stations/turrets need the revert below for
	// repair-arm beams and weapon damage to land.
	if (destructible) {
		*(unsigned char*)((char*)Deployable + 0x93) = 0; // CollisionType = COLLIDE_CustomDefault
		*(unsigned char*)((char*)Deployable + 0x94) = 0; // ReplicatedCollisionType
		*(uint32_t*)((char*)Deployable + 0xAC) |= 0x100000; // bNetDirty
		SetCollisionFromCollisionType(Deployable);
	} else {
		Logger::Log("deploy_phase",
			"[invuln-deployable] deployableId=%d  HP=0 in DB — leaving CollisionType=COLLIDE_BlockAllButWeapons "
			"(hitscan passes through, movement still blocks); r_bTakeDamage=0\n",
			deployableId);
	}

	// Diagnostic: confirm the post-revert flag state.  Should show
	// cylFlags with bit 7 (BlockZeroExtent) set.
	{
		uint32_t cylFlags = Deployable->CollisionComponent
			? *(uint32_t*)((char*)Deployable->CollisionComponent + 0x128) : 0;
		uint32_t actorFlags = *(uint32_t*)((char*)Deployable + 0xB0);
		Logger::Log("deploy_phase",
			"[cylinder-resize] deployable=0x%p id=%d  radius=%.1f halfHeight=%.1f\n"
			"                  collisionComp=%p targetComp=%p  CollisionType=%d (post-revert)\n"
			"                  actorFlags@+0xB0=0x%08x  (bCollide=%d bBlock=%d bProj=%d bDmg=%d bWorld=%d)\n"
			"                  cylFlags@+0x128=0x%08x  (CollideActors=%d BlockActors=%d BlockZero=%d BlockNonZero=%d BlockRigidBody=%d)\n",
			Deployable, deployableId, cylRadius, cylHalfHeight,
			(void*)Deployable->CollisionComponent,
			(void*)Deployable->m_TargetComponent,
			(int)*(unsigned char*)((char*)Deployable + 0x93),
			actorFlags,
			(int)((actorFlags >> 27) & 1),
			(int)((actorFlags >> 30) & 1),
			(int)((actorFlags >> 31) & 1),
			(int)((actorFlags >> 19) & 1),
			(int)((actorFlags >> 28) & 1),
			cylFlags,
			(int)((cylFlags >> 4) & 1),
			(int)((cylFlags >> 6) & 1),
			(int)((cylFlags >> 7) & 1),
			(int)((cylFlags >> 8) & 1),
			(int)((cylFlags >> 10) & 1));
	}

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

	if (bIsBeacon) {
		// Player-deploy path. The factory-spawn path (TgBeaconFactory::SpawnObject)
		// handles its own DRI wiring + immediate-DEPLOYED state; this branch is
		// only for beacons thrown by a player via TgDeviceFire.Deploy.
		//
		// Native RegisterBeacon @ 0x109F1ED0 does the heavy lifting:
		//   * mgr->r_Beacon       = beacon
		//   * mgr->r_BeaconInfo   = beacon->r_DRI (cast to TgRepInfo_Beacon)
		//   * r_BeaconInfo->r_vLoc        = beacon->Location  (HUD compass)
		//   * r_BeaconInfo->r_bDeployed   = bDeployed param   (HUD marker)
		//   * r_BeaconInfo->r_TaskforceInfo = mgr->r_TaskForce
		//   * Calls CheckBeacon(true) internally, which (when beacon is good and
		//     r_DRI->r_InstigatorInfo is non-null) sets mgr->r_BeaconHolder =
		//     r_InstigatorInfo — that's what drives the HUD's deployer-name
		//     display for deployed beacons.
		//
		// So we MUST set r_DRI->r_InstigatorInfo (and team fields) BEFORE
		// RegisterBeacon — otherwise CheckBeacon fires with a null instigator
		// and r_BeaconHolder stays empty. We pass bDeployed=false so the
		// DEPLOYING state holds the entrance gate closed until UC's deploy
		// timer completes; UC's Deploy.EndState then re-registers with
		// bDeployed=true.
		ATgTeamBeaconManager* beaconMgr = pawnrep ? pawnrep->r_TaskForce->r_BeaconManager : nullptr;
		if (beaconMgr) {
			// Kill the previously-active world beacon for this team (if any).
			// UC's Destroyed → UnRegisterBeacon will clear mgr->r_Beacon, so
			// our RegisterBeacon below installs cleanly.
			if (beaconMgr->r_Beacon && beaconMgr->r_Beacon != (ATgDeploy_Beacon*)Deployable) {
				beaconMgr->r_Beacon->eventDestroyIt(0);
			}

			// Intentionally leave s_DeployFactory == null for player-deployed
			// beacons. CheckBeacon's good-beacon branch picks r_BeaconStatus
			// from factory->s_bAutoSpawn — all map factories have AutoSpawn=1
			// (TgActorFactory defaultprops), which forces status=5 (AT_SPAWN)
			// — a status the HUD switch (TgUIPrimaryHUD_BeaconStatus
			// ::TickPrimaryHUDElement @ 0x114d8580) has no case for, so
			// neither location arrow nor deployer name renders.
			//
			// Null s_DeployFactory trips CheckBeacon's "bad-beacon" branch
			// (s_DeployFactory==null is one of its bail conditions). With
			// LoadInventoryBeacon==0 (player consumed the deploy device) and
			// r_Beacon set (we just registered) → r_BeaconStatus=2 (DEPLOYING)
			// → HUD case 2 fires both direction (r_vLoc valid + r_bDeployed=0)
			// AND name (r_BeaconHolder set). The bottom of CheckBeacon still
			// runs `r_BeaconHolder = r_DRI->r_InstigatorInfo` in this branch.
			//
			// ABFS handles the null-s_DeployFactory case explicitly (treats
			// the beacon as forward-deployed and destroys on tier change).

			// Wire DRI ownership BEFORE RegisterBeacon — see comment block
			// above for why. SetTaskForce native zeros r_InstigatorInfo (see
			// reference_deployable_team_colors.md) so we write directly.
			if (Deployable->r_DRI && beaconMgr->r_TaskForce) {
				Deployable->r_DRI->r_bOwnedByTaskforce = 1;
				Deployable->r_DRI->r_TaskforceInfo     = beaconMgr->r_TaskForce;
				Deployable->r_DRI->r_InstigatorInfo    = pawnrep;
			}
			Deployable->r_bInitialIsEnemy = 0;

			// RegisterBeacon writes r_vLoc / r_bDeployed / r_TaskforceInfo on
			// the DRI and fires CheckBeacon to populate r_BeaconHolder.
			// bDeployed=false so r_bDeployed stays 0 (DEPLOYING) — UC's
			// DeployComplete will flip m_bIsDeployed=1 later and re-register
			// with bDeployed=true via state machinery. Without false here the
			// entrance HasExit gate (which checks r_Beacon->m_bIsDeployed)
			// would unlock immediately on throw instead of after the deploy
			// timer completes.
			BeaconSdk::RegisterBeacon(beaconMgr, (ATgDeploy_Beacon*)Deployable, false);

			// RegisterBeacon's writes don't auto-set bNetDirty on the DRI,
			// so the rep tick would skip r_vLoc updates without this.
			if (Deployable->r_DRI) {
				Deployable->r_DRI->bNetDirty       = 1;
				Deployable->r_DRI->bForceNetUpdate = 1;
			}
			beaconMgr->bNetDirty        = 1;
			beaconMgr->bForceNetUpdate  = 1;
			Deployable->bNetDirty       = 1;
			Deployable->bForceNetUpdate = 1;

			Logger::Log("beacon",
				"SpawnDeployableActor[beacon]: dep=0x%p s_DeployFactory=0x%p mgr=0x%p "
				"mgr->r_Beacon=0x%p mgr->r_BeaconInfo=0x%p mgr->r_BeaconHolder=0x%p deployer=0x%p\n",
				Deployable, Deployable->s_DeployFactory, beaconMgr,
				beaconMgr->r_Beacon, beaconMgr->r_BeaconInfo, beaconMgr->r_BeaconHolder, pawnrep);

			// Tear down the carrier visual on the deployer. The carry-beacon
			// device (id 1918) lives at slot 11 of the pawn; SpawnBotById
			// applied m_EquipEffect to spawn the floating-rings form. Now that
			// the beacon is in the world, we no longer carry it — symmetric
			// RemoveEquipEffects clears the SetEffectRep slot so the form
			// disappears on clients. UC's eventual inventory clear (via the
			// CheckBeacon → bPersistsInInventory chain) is async/asymmetric
			// and doesn't fire NonPersistRemoveDevice, so we own the cleanup.
			if (pawn) {
				ATgDevice* carryDev = pawn->m_EquippedDevices[11];
				if (carryDev && carryDev->r_nDeviceId == 1918) {
					uint32_t devFlags = *(uint32_t*)((char*)carryDev + 0x22C);
					int bEquipEffectsApplied = (devFlags & 0x4) != 0;
					Logger::Log("beacon",
						"SpawnDeployableActor[beacon]: tearing down carrier visual on pawn=0x%p slot=11 device=0x%p m_bEquipEffectsApplied=%d\n",
						pawn, carryDev, bEquipEffectsApplied);
					carryDev->RemoveEquipEffects();
				}
			}

		} else {
			Logger::Log("beacon",
				"SpawnDeployableActor[beacon]: WARNING no BeaconManager for pawn 0x%p — skipping registration\n",
				pawn);
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
	// Per-deployable deploy time. UC's Deploy.BeginState reads
	// r_fTimeToDeploySecs first; only when 0 does it fall back to the deploy
	// anim length (which is similar across all assets and was making every
	// deployable feel uniform). Stations/turrets get explicit 5–15s from
	// prop 279; bombs/mines/forcefields get 0.1s so they "deploy" instantly.
	{
		float deploySecs = ApplyDeployTimeBuff(pawn, GetDeployTimeSecs(deployableId),
		                                       sourceDevice ? sourceDevice->r_nDeviceInstanceId : 0);
		Deployable->r_fTimeToDeploySecs = deploySecs;
		Deployable->bNetDirty           = 1;
		Deployable->bForceNetUpdate     = 1;
		Logger::Log("bomb",
			"[deploy time] deployableId=%d  r_fTimeToDeploySecs=%.3fs (DB+buff)\n",
			deployableId, deploySecs);
	}

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

	// Phase 5: apply the placing pawn's rolled-mod buffs to the deployable's
	// s_Properties AND the timer-bomb engine mirrors (s_fProximityRadius,
	// r_fClientProximityRadius). Runs AFTER the timer-bomb block above so we
	// can scale the just-set proximity radius (which the client warning HUD
	// reads) in addition to the s_Properties[6] value (which the server-side
	// damage AoE reads via GetDamageRadius). Handles both the visual-warning
	// path AND the actual-damage path in one shot. See ApplyPlayerModsToDeployable.cpp.
	// `sourceDevice->r_nDeviceInstanceId` filters the pawn's buff registry to
	// THIS deploying device's mods + wildcards (skills). Without it, every
	// other equipped weapon's mods would also fold into the deployable's
	// props (Output Mod on every device alone would inflate radius/damage
	// by ~7×). Pass 0 if no source device — only wildcard entries fold in.
	ApplyPlayerModsToDeployable::Apply(pawn, Deployable,
	                                   sourceDevice ? sourceDevice->r_nDeviceInstanceId : 0);

	// Stamp deployable + internal fire-mode owner with the deploy device's
	// identity so UC's `TgEffectGroup.InitInstance` (TgEffectGroup.uc:121-141)
	// can populate cloned heal/buff effects' `m_nSourceDeviceInstId` /
	// `m_nSourceDeviceSkillId` from the player's deploy device instead of
	// from the (zeroed) deployable-internal device.
	//
	// UC reads `Impact.DeviceModeReference.m_Owner` and switches:
	//   - if TgDevice → reads `Dev.r_nDeviceInstanceId` / `Dev.m_nSkillId`
	//   - else if TgDeployable → reads `dep.GetSpawnerDeviceInstanceId()` /
	//                                `dep.GetFireDeviceSkillId()`
	//
	// Both natively return 0 in our env unless we wire the underlying fields.
	// Diagnostic confirmed: cloneSrcDevInst=0 at heal-fire time → query
	// passes devInst=0 / skillId=0 → only pure-wildcard buff entries match.
	//
	// Done at end-of-function: m_FireMode is initialized AFTER Spawn (e.g.,
	// by ApplyDeployableSetup further up); stamping at the early r_Owner-set
	// site found m_FireMode=null and silently skipped.
	if (sourceDevice) {
		// Re-assert s_SpawnerDeviceMode HERE: ApplyDeployableSetup (run after the
		// early :692 set) re-inits the deployable's fire config and clears it, so
		// GetSpawnerDeviceInstanceId — which walks s_SpawnerDeviceMode->m_Owner->
		// r_nDeviceInstanceId — was resolving 0 at heal-fire time (station heal
		// scaled by wildcard skills only, not the device's Output Mod). Proven by
		// the [deploy-origin] log: spawnerMode=NULL while sourceDevice invId=74.
		// m_FireSkillId (0x22C) covers UC's dep-branch GetFireDeviceSkillId.
		if (sourceFireMode) Deployable->s_SpawnerDeviceMode = sourceFireMode;
		Deployable->m_FireSkillId = sourceDevice->m_nSkillId;

		// Diagnostic for the station-heal devInst=0 bug: shows whether the
		// deploy device carries a real invId/skill and whether the
		// s_SpawnerDeviceMode->m_Owner chain (what GetSpawnerDeviceInstanceId
		// walks) actually points back at it.
		Logger::Log("effects",
			"[deploy-origin] deployable=%p sourceDevice=%p invId=%d skill=%d  spawnerMode=%p mode->m_Owner=%p (==sourceDevice? %d)\n",
			(void*)Deployable, (void*)sourceDevice,
			sourceDevice->r_nDeviceInstanceId, sourceDevice->m_nSkillId,
			(void*)Deployable->s_SpawnerDeviceMode,
			(void*)(Deployable->s_SpawnerDeviceMode ? Deployable->s_SpawnerDeviceMode->m_Owner : nullptr),
			(int)(Deployable->s_SpawnerDeviceMode &&
			      Deployable->s_SpawnerDeviceMode->m_Owner == (AActor*)sourceDevice));

		// Internal-device case: if m_FireMode's owner is a TgDevice (the
		// deployable's stub heal/damage device, not the deployable itself),
		// stamp that device's identity so UC's Dev branch reads the
		// player's deploy-device values.
		if (Deployable->m_FireMode && Deployable->m_FireMode->m_Owner) {
			AActor* fireOwner = Deployable->m_FireMode->m_Owner;
			const char* foClass = fireOwner->Class ? fireOwner->Class->GetFullName() : nullptr;
			if (foClass && strstr(foClass, "TgDeployable") == nullptr
			            && strstr(foClass, "TgDeploy_")    == nullptr) {
				ATgDevice* internalDev = (ATgDevice*)fireOwner;
				internalDev->r_nDeviceInstanceId = sourceDevice->r_nDeviceInstanceId;
				internalDev->m_nSkillId          = sourceDevice->m_nSkillId;
				Logger::Log("effects",
					"[fire-owner-stamp] deployable=0x%p  internalDev=0x%p (class=%s)  "
					"r_nDeviceInstanceId=%d m_nSkillId=%d (from sourceDevice 0x%p)\n",
					Deployable, internalDev, foClass,
					sourceDevice->r_nDeviceInstanceId, sourceDevice->m_nSkillId,
					sourceDevice);
			} else {
				Logger::Log("effects",
					"[fire-owner-stamp] deployable=0x%p  fire owner is %s — skipping device stamp "
					"(dep branch handled via m_FireSkillId + s_SpawnerDeviceMode)\n",
					Deployable, foClass ? foClass : "<null>");
			}
		} else {
			Logger::Log("effects",
				"[fire-owner-stamp] deployable=0x%p  m_FireMode=%p m_Owner=%p — no fire-mode owner to stamp\n",
				Deployable, (void*)Deployable->m_FireMode,
				(void*)(Deployable->m_FireMode ? Deployable->m_FireMode->m_Owner : nullptr));
		}
	}

	// (Spawning-device origin side-map removed: OriginResolver recovers the
	// deploy device's instId/skillId at apply time by walking the deployable's
	// s_SpawnerDeviceMode -> device, so no registration is needed here. The
	// deployable's s_SpawnerDeviceMode is wired during fire-mode setup above.)

	// Bomb diagnostic: after all setup is done, dump bomb-specific replicated
	// state (tick time, destroy flags, mesh, fire mode, life span).  If bombs
	// vanish visually, one of these is almost certainly uninitialised — the
	// tick system never starts, LifeSpan goes to 0 immediately, or the mesh
	// fails to load.
	if (Logger::IsChannelEnabled("bomb")) {
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
	}

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
	if (Logger::IsChannelEnabled("bomb")) {
		Logger::Log("bomb",
			"[SpawnDeployable entry] pThis=0x%p  targetActor=0x%p  loc=(%.1f,%.1f,%.1f)  pThis.class=%s\n",
			pThis, TargetActor, vLocation.X, vLocation.Y, vLocation.Z,
			(pThis && pThis->Class) ? pThis->Class->GetFullName() : "<null>");
	}

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

	if (Logger::IsChannelEnabled("bomb")) {
		Logger::Log("bomb",
			"[SpawnDeployable] resolved deployableId=%d for pawn=%s — calling SpawnDeployableActor\n",
			deployableId, pawn->GetFullName());
		Logger::Log(GetLogChannel(), "TgProj_Deployable::SpawnDeployable: pawn=%s deployableId=%d loc=(%.0f,%.0f,%.0f)\n",
			pawn->GetFullName(), deployableId, vLocation.X, vLocation.Y, vLocation.Z);
	}

	// Forward the projectile's source device + fire mode to the deployable so
	// it carries r_Owner / s_SpawnerDeviceMode. ATgProjectile.r_Owner is the
	// device that fired the projectile (set by TgDeviceFire::InitializeProjectile).
	ATgDevice*     srcDevice   = (ATgDevice*)pThis->r_Owner;
	UTgDeviceFire* srcFireMode = pThis->s_OwnerFireMode;

	ATgDeployable* result = SpawnDeployableActor(pawn, deployableId, vLocation, vNormal,
		srcDevice, srcFireMode);

	if (Logger::IsChannelEnabled("bomb")) {
		Logger::Log("bomb",
			"[SpawnDeployable] SpawnDeployableActor returned 0x%p (class=%s)\n",
			result,
			(result && result->Class) ? result->Class->GetFullName() : "<null>");
	}

	return result;
}
