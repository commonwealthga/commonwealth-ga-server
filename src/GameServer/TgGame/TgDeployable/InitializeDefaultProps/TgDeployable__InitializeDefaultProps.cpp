#include "src/GameServer/TgGame/TgDeployable/InitializeDefaultProps/TgDeployable__InitializeDefaultProps.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/TgGame/TgProperty/ConstructTgProperty/TgProperty__ConstructTgProperty.hpp"
#include "src/GameServer/Constants/TgProperties.h"
#include "src/Database/Database.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace {
struct DeployableDefaults {
	int health;
};
// Per-deployable_id cache. Populated on first call; reused across spawns.
// asm_data_set_deployables is static reference data loaded from assembly.dat.
std::unordered_map<int, DeployableDefaults> g_deployableDefaultsCache;
}

UTgProperty* TgDeployable__InitializeDefaultProps::InitializeProperty(
	ATgDeployable* Deployable, int nPropertyId,
	float fBase, float fRaw, float fMinimum, float fMaximum)
{
	if (!Deployable) return nullptr;

	// Allocate s_Properties storage on first use. 128 slots is comfortably over
	// what the deploy path populates (HP, HP_MAX, DeployRate, + any fire-mode
	// property descriptors pushed by ApplyDeployableSetup).
	if (Deployable->s_Properties.Data == nullptr) {
		Deployable->s_Properties.Data = (UTgProperty**)malloc(sizeof(UTgProperty*) * 128);
		memset(Deployable->s_Properties.Data, 0, sizeof(UTgProperty*) * 128);
		Deployable->s_Properties.Count = 0;
		Deployable->s_Properties.Max   = 128;
	}

	// Idempotent upsert — if the prop is already in s_Properties, update its
	// fields in place instead of adding a duplicate slot.  Lets callers seed
	// defaults and then override from DB rows without creating double entries
	// that SetProperty/GetProperty's linear scan would read inconsistently.
	UTgProperty* Property = nullptr;
	for (int i = 0; i < Deployable->s_Properties.Count; ++i) {
		UTgProperty* p = Deployable->s_Properties.Data[i];
		if (p && p->m_nPropertyId == nPropertyId) {
			Property = p;
			break;
		}
	}
	if (!Property) {
		Property = (UTgProperty*)TgProperty__ConstructTgProperty::CallOriginal(
			ClassPreloader::GetTgPropertyClass(), -1, 0, 0, 0, 0, 0, 0, 0);
		int idx = Deployable->s_Properties.Count;
		Deployable->s_Properties.Data[idx] = Property;
		Deployable->s_Properties.Count     = idx + 1;
	}

	Property->m_nPropertyId = nPropertyId;
	Property->m_fBase       = fBase;
	Property->m_fRaw        = fRaw;
	Property->m_fMinimum    = fMinimum;
	Property->m_fMaximum    = fMaximum;

	// SetProperty is a real native at 0x10a1c940 — scans s_Properties for the
	// id, writes the new value, then dispatches ApplyProperty (vtable slot 251)
	// which pushes the value into any engine-side mirror (r_nHealth for HP).
	Deployable->SetProperty(nPropertyId, fRaw);

	return Property;
}

// Load the deployable's device-mode properties from the DB and push each row
// into s_Properties via InitializeProperty.  Matches what the game's asm.dat
// loader (FUN_109a7d20 in the client binary) does for device modes: one
// TgProperty slot per row with {prop_id, base, base-as-raw, min, max}.
//
// Called from Call() AFTER the identity-default block so DB values overwrite
// the zeroed protections / identity deploy-rate / proximity defaults — the
// upsert in InitializeProperty guarantees we mutate existing slots rather
// than duplicating them.
//
// Covers deployables the user deploys (medstation pulse rate, sweep-sensor
// range, persist-time, etc.) — everything previously "missing" from our
// server-side property table.
// Per-deployable_id cache of device-mode property rows.  Populated lazily on
// first deploy of a given id; every subsequent spawn reuses the vector without
// touching SQLite.  asm.dat is static reference data loaded once at game start,
// so there's nothing to invalidate at runtime.  An empty vector (deployable
// with zero DB rows — beacons, pickups, etc.) is also cached as a negative
// result so we don't re-query for nothing.
namespace {
struct CachedProp { int propId; float base; float minV; float maxV; };
std::unordered_map<int, std::vector<CachedProp>> g_deployablePropCache;
}

static void SeedPropertiesFromDeviceModeDb(ATgDeployable* Deployable, int deployableId) {
	auto it = g_deployablePropCache.find(deployableId);
	if (it == g_deployablePropCache.end()) {
		// First time we see this deployable_id — read the DB once, cache forever.
		std::vector<CachedProp> rows;
		sqlite3* db = Database::GetConnection();
		if (!db) {
			Logger::Log("deployable_props",
				"[SeedPropsFromDb] no DB connection — deployableId=%d\n", deployableId);
			return;
		}
		sqlite3_stmt* stmt = nullptr;
		// Join deployables → device_mode_properties.  A deployable with multiple
		// modes contributes each mode's props; InitializeProperty upserts by
		// prop_id so multi-mode duplicates collapse automatically onto a single
		// slot (last row wins — benign since same-prop repeats in asm.dat share
		// the same base/min/max across modes in practice).
		int rc = sqlite3_prepare_v2(db,
			"SELECT p.prop_id, p.base_value, p.min_value, p.max_value "
			"FROM asm_data_set_deployables d "
			"JOIN asm_data_set_device_mode_properties p ON d.device_id = p.device_id "
			"WHERE d.deployable_id = ? "
			"ORDER BY p.prop_id;",
			-1, &stmt, nullptr);
		if (rc != SQLITE_OK) {
			Logger::Log("deployable_props",
				"[SeedPropsFromDb] prepare failed (rc=%d) deployableId=%d: %s\n",
				rc, deployableId, sqlite3_errmsg(db));
			// Don't cache on prepare error — a transient DB issue shouldn't
			// permanently blacklist this id; next deploy can retry.
			return;
		}
		sqlite3_bind_int(stmt, 1, deployableId);
		while (sqlite3_step(stmt) == SQLITE_ROW) {
			rows.push_back({
				sqlite3_column_int(stmt, 0),
				(float)sqlite3_column_double(stmt, 1),
				(float)sqlite3_column_double(stmt, 2),
				(float)sqlite3_column_double(stmt, 3),
			});
		}
		sqlite3_finalize(stmt);
		it = g_deployablePropCache.emplace(deployableId, std::move(rows)).first;

		Logger::Log("deployable_props",
			"[SeedPropsFromDb] deployableId=%d cached %zu rows (first deploy — DB query fired)\n",
			deployableId, it->second.size());
	}

	const std::vector<CachedProp>& rows = it->second;

	std::string seedDetails;
	for (const CachedProp& r : rows) {
		// Native FUN_109a7d20 copies base → raw at +0x44, so match that.
		TgDeployable__InitializeDefaultProps::InitializeProperty(
			Deployable, r.propId, r.base, /*raw=*/r.base, r.minV, r.maxV);

		// Mirror known SDK fields that ATgDeployable::ApplyProperty DOESN'T
		// handle (only props 8 + 278 fan out natively — everything else below
		// would otherwise stay at zero even after the s_Properties slot is
		// populated).  Without this the state machine stalls: UC's
		// Active.BeginState gates the StartFire timer on `s_fActivationTime > 0`
		// (see TgDeployable.uc:1999).  A medstation with prop 7 = 1.0 would
		// otherwise sit in Active forever because s_fActivationTime stays 0.
		//
		// prop 7   → s_fActivationTime    : post-deploy delay before first fire
		//                                   (bombs: fuse length; stations: 1s warmup)
		// prop 150 → s_fPersistTime       : total uptime before auto-expiry
		//                                   (medstation: 10000; beacons: 0 = forever)
		//
		// Bombs also write these explicitly in SpawnDeployableActor's
		// timer-bomb block with different derivation (dedicated DB lookup on
		// prop 6 for radius + prop 7 for activation) — the write here is
		// idempotent with that path; bombs just get the value set twice.
		switch (r.propId) {
			case 7:   Deployable->s_fActivationTime = r.base; break;
			case 150: Deployable->s_fPersistTime    = r.base; break;
		}

		char line[96];
		snprintf(line, sizeof(line), "                prop %3d = %.3f  [min %.3f max %.3f]\n",
			r.propId, r.base, r.minV, r.maxV);
		seedDetails += line;
	}

	Logger::Log("deployable_props",
		"[SeedPropsFromDb] 0x%p deployableId=%d  seeded=%zu props (from cache)\n%s"
		"                s_fActivationTime=%.2f  s_fPersistTime=%.2f  s_fProximityRadius=%.2f\n",
		Deployable, deployableId, rows.size(), seedDetails.c_str(),
		Deployable->s_fActivationTime, Deployable->s_fPersistTime,
		Deployable->s_fProximityRadius);
}

void __fastcall TgDeployable__InitializeDefaultProps::Call(ATgDeployable* Deployable, void* edx) {
	if (!Deployable) return;

	// UC PostBeginPlay calls InitializeDefaultProps during Spawn(), *before*
	// our spawn code has set r_nDeployableId. Bail out; SpawnDeployableActor
	// invokes us again explicitly once the id is set.
	if (Deployable->r_nDeployableId == 0) {
		return;
	}

	// Don't double-populate. If an earlier explicit call already ran, skip.
	if (Deployable->s_Properties.Data != nullptr && Deployable->s_Properties.Count > 0) {
		return;
	}

	int deployableId = Deployable->r_nDeployableId;

	// Defaults used when DB row is missing or health unpopulated.
	float health = 100.0f;

	auto it = g_deployableDefaultsCache.find(deployableId);
	if (it == g_deployableDefaultsCache.end()) {
		DeployableDefaults d{ (int)health };
		sqlite3* db = Database::GetConnection();
		sqlite3_stmt* stmt;
		if (sqlite3_prepare_v2(db,
			"SELECT health FROM asm_data_set_deployables WHERE deployable_id = ? LIMIT 1",
			-1, &stmt, nullptr) == SQLITE_OK) {
			sqlite3_bind_int(stmt, 1, deployableId);
			if (sqlite3_step(stmt) == SQLITE_ROW) {
				if (sqlite3_column_type(stmt, 0) != SQLITE_NULL) {
					int h = sqlite3_column_int(stmt, 0);
					if (h > 0) d.health = h;
				}
			}
			sqlite3_finalize(stmt);
		}
		it = g_deployableDefaultsCache.emplace(deployableId, d).first;
	}
	health = (float)it->second.health;

	Logger::Log("deployable_props",
		"[InitDefaultProps] 0x%p deployableId=%d health=%.1f\n",
		Deployable, deployableId, health);

	// Seed identity defaults first so every slot effects might buff is present
	// (protection, proximity, deploy-rate).  DB-driven values then overwrite
	// via the upsert below — DB wins over identity for any prop the asm.dat
	// loader captures.
	// TGPID_HEALTH (51), TGPID_HEALTH_MAX (304) — neither is in the
	// ATgDeployable::ApplyProperty switch (only props 8 + 278 are), so
	// fRaw doesn't land in an engine mirror field. But r_nHealth/DRI are
	// already populated below, and any GetProperty-based damage calc can
	// read the slot from s_Properties.  Taken from asm_data_set_deployables.
	// health (a top-level column, not in device_mode_properties).
	InitializeProperty(Deployable, GA_PROPERTY::TGPID_HEALTH,     health, health, 0, health);
	InitializeProperty(Deployable, GA_PROPERTY::TGPID_HEALTH_MAX, health, health, 0, health);

	// TGPID_DEPLOY_RATE_MODIFIER (278) — identity 1.0. This one IS in the
	// native ApplyProperty switch (maps to r_fDeployRate +0x27C). Repair
	// arms add +4.5 for 1s via effect-system Hit on prop 278; UC TickDeploy
	// multiplies DeltaSeconds by r_fDeployRate each tick.
	InitializeProperty(Deployable, GA_PROPERTY::TGPID_DEPLOY_RATE_MODIFIER, 1.0f, 1.0f, 0, 100.0f);

	// TGPID_PROXIMITY_DISTANCE (8) — the other prop ATgDeployable::ApplyProperty
	// handles (maps to s_fProximityRadius / r_fClientProximityRadius with ×16
	// conversion). Identity 0.0 means no proximity; sensors/mines override via
	// their equip-effect or device-mode properties.
	InitializeProperty(Deployable, GA_PROPERTY::TGPID_PROXIMITY_DISTANCE, 0.0f, 0.0f, 0, 1000.0f);

	// Protection properties — must exist in s_Properties so that damage
	// calculation (TgEffectManager::ApplyDamage or similar) can resolve
	// GetProperty(N) and read the current value. Equip effects on stations
	// (Protection-Bio +1000 etc.) write into these slots via SetProperty;
	// without the slot pre-registered, the SetProperty no-ops (linear scan
	// finds nothing) and the buff is lost.
	//
	// ATgDeployable::ApplyProperty does NOT engine-mirror these prop IDs —
	// that's OK because damage calc reads via GetProperty, not a cached
	// field. Identity default 0 so they only matter when buffed.
	InitializeProperty(Deployable, GA_PROPERTY::TGPID_PROTECTION_PHYSICAL,  0, 0, 0, 10000);
	InitializeProperty(Deployable, GA_PROPERTY::TGPID_PROTECTION_ENERGY,    0, 0, 0, 10000);
	InitializeProperty(Deployable, GA_PROPERTY::TGPID_PROTECTION_THERMAL,   0, 0, 0, 10000);
	InitializeProperty(Deployable, GA_PROPERTY::TGPID_PROTECTION_SLOW,      0, 0, 0, 10000);
	InitializeProperty(Deployable, GA_PROPERTY::TGPID_PROTECTION_POISON,    0, 0, 0, 10000);
	InitializeProperty(Deployable, GA_PROPERTY::TGPID_PROTECTION_DISEASE,   0, 0, 0, 10000);
	InitializeProperty(Deployable, GA_PROPERTY::TGPID_PROTECTION_STUN,      0, 0, 0, 10000);
	InitializeProperty(Deployable, GA_PROPERTY::TGPID_PROTECTION_SLEEP,     0, 0, 0, 10000);
	InitializeProperty(Deployable, GA_PROPERTY::TGPID_PROTECTION_KNOCKBACK, 0, 0, 0, 10000);
	InitializeProperty(Deployable, GA_PROPERTY::TGPID_PROTECTION_EMP_STUN,  0, 0, 0, 10000);
	InitializeProperty(Deployable, GA_PROPERTY::TGPID_PROTECTION_IGNITE,    0, 0, 0, 10000);
	InitializeProperty(Deployable, GA_PROPERTY::TGPID_PROTECTION_BIO,       0, 0, 0, 10000);
	InitializeProperty(Deployable, GA_PROPERTY::TGPID_PROTECTION_EMP_BURN,  0, 0, 0, 10000);
	InitializeProperty(Deployable, GA_PROPERTY::TGPID_PROTECTION_BLEED,     0, 0, 0, 10000);
	InitializeProperty(Deployable, GA_PROPERTY::TGPID_PROTECTION_MELEE,     0, 0, 0, 10000);
	InitializeProperty(Deployable, GA_PROPERTY::TGPID_PROTECTION_RANGED,    0, 0, 0, 10000);
	InitializeProperty(Deployable, GA_PROPERTY::TGPID_PROTECTION_AOE,       0, 0, 0, 10000);

	// DB-driven device-mode props (range, fire-time, persist-time, damage-
	// radius, specific protection values, sweep-sensor geometry, etc.) — the
	// asm.dat loader's equivalent of pushing TgProperty slots onto the fire
	// mode.  Runs AFTER the identity defaults so DB values overwrite the
	// zeroed protections / identity deploy-rate / proximity where a row
	// exists.  Upsert in InitializeProperty guarantees no duplicate slots.
	SeedPropertiesFromDeviceModeDb(Deployable, deployableId);

	// DRI carries the replicated HP fields separately from ATgDeployable.r_nHealth.
	// Without this sync the client's health bar reads stale maximum.
	if (Deployable->r_DRI) {
		Deployable->r_DRI->r_nHealthMaximum  = (int)health;
		Deployable->r_DRI->r_nHealthCurrent  = (int)health;
		Deployable->r_DRI->r_fDeployMaxHealthPCT = 1.0f;
		Deployable->r_DRI->bNetDirty        = 1;
		Deployable->r_DRI->bForceNetUpdate  = 1;
	}

	Deployable->bNetDirty       = 1;
	Deployable->bForceNetUpdate = 1;
}
