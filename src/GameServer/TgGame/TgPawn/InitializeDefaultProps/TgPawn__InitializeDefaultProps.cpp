#include "src/GameServer/TgGame/TgPawn/InitializeDefaultProps/TgPawn__InitializeDefaultProps.hpp"
#include "src/GameServer/Constants/GameTypes.h"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/TgGame/TgProperty/ConstructTgProperty/TgProperty__ConstructTgProperty.hpp"
#include "src/GameServer/Core/TMap/Allocate/TMap__Allocate.hpp"
#include "src/GameServer/Constants/TgProperties.h"
#include "src/Database/Database.hpp"
#include "src/Utils/Logger/Logger.hpp"

int TgPawn__InitializeDefaultProps::nPendingBotId = 0;

namespace {
struct BotDefaults {
	float hitPoints;
	float speed;
	float powerPool;
	float rechargeRate;
	float accuracy;
	float sightRange;
};
// Per-bot defaults cache. Populated lazily on the first InitializeDefaultProps
// call for a given bot_id; subsequent spawns of the same bot reuse the row
// without hitting SQLite. The DB rows are static reference data (loaded from
// assembly.dat at startup), so once cached they don't need invalidation.
std::unordered_map<int, BotDefaults> g_botDefaultsCache;
}

UTgProperty* TgPawn__InitializeDefaultProps::InitializeProperty(ATgPawn* Pawn, int nPropertyId, float fBase, float fRaw, float fMinimum, float fMaximum) {
	UTgProperty* Property = (UTgProperty*)TgProperty__ConstructTgProperty::CallOriginal(
		ClassPreloader::GetTgPropertyClass(), -1, 0, 0,0,0,0,0,0);

	Property->m_nPropertyId = nPropertyId;
	Property->m_fBase = fBase;
	Property->m_fRaw = fRaw;
	Property->m_fMinimum = fMinimum;
	Property->m_fMaximum = fMaximum;

	UTgProperty*** PropertiesDataPtrPtr = (UTgProperty***)((char*)Pawn + 0x3F4);
	int* PropertiesCountPtr = (int*)((char*)Pawn + 0x3F8);
	int* PropertiesMaxPtr   = (int*)((char*)Pawn + 0x3FC);

	// Initialize if not allocated yet
	if (*PropertiesDataPtrPtr == nullptr)
	{
		*PropertiesDataPtrPtr = (UTgProperty**)malloc(sizeof(UTgProperty*) * 512);
		memset(*PropertiesDataPtrPtr, 0, sizeof(UTgProperty*) * 512);
		*PropertiesCountPtr = 0;
		*PropertiesMaxPtr = 512;
	}

	// Add the property
	int Count = *PropertiesCountPtr;
	(*PropertiesDataPtrPtr)[Count] = Property;
	*PropertiesCountPtr = Count + 1;

	Pawn->SetProperty(nPropertyId, fRaw);

	return Property;
}

void __fastcall* TgPawn__InitializeDefaultProps::Call(ATgPawn* Pawn, void* edx) {
	LogCallBegin();

	if ((char*)Pawn + 0x400 == nullptr) {
		TMap__Allocate::CallOriginal((void*)((char*)Pawn + 0x400));
	}

	int nBotId = 0;

	// Defaults used when botId is unknown or DB row is missing/unpopulated.
	float hitPoints    = 1000.0f;
	float speed        = 300.0f;
	float powerPool    = 100.0f;
	float rechargeRate = 20.0f;
	float accuracy     = 1.0f;
	float sightRange   = 800.0f;

	if (nPendingBotId != 0) {
		auto it = g_botDefaultsCache.find(nPendingBotId);
		if (it == g_botDefaultsCache.end()) {
			// First spawn of this bot — query the DB row and cache it.
			BotDefaults d{ hitPoints, speed, powerPool, rechargeRate, accuracy, sightRange };
			sqlite3* db = Database::GetConnection();
			sqlite3_stmt* stmt;
			if (sqlite3_prepare_v2(db,
				"SELECT hit_points, default_speed, default_power_pool, power_pool_regen_per_sec, accuracy_override, default_sensor_range "
				"FROM asm_data_set_bots WHERE bot_id = ? LIMIT 1",
				-1, &stmt, nullptr) == SQLITE_OK) {
				sqlite3_bind_int(stmt, 1, nPendingBotId);
				if (sqlite3_step(stmt) == SQLITE_ROW) {
					if (sqlite3_column_type(stmt, 0) != SQLITE_NULL && sqlite3_column_int(stmt, 0) > 0)
						d.hitPoints    = (float)sqlite3_column_int(stmt, 0);
					if (sqlite3_column_type(stmt, 1) != SQLITE_NULL && sqlite3_column_double(stmt, 1) > 0.0)
						d.speed        = (float)sqlite3_column_double(stmt, 1) * 16.0f;
					if (sqlite3_column_type(stmt, 2) != SQLITE_NULL && sqlite3_column_int(stmt, 2) > 0)
						d.powerPool    = (float)sqlite3_column_int(stmt, 2);
					if (sqlite3_column_type(stmt, 3) != SQLITE_NULL && sqlite3_column_double(stmt, 3) > 0.0)
						d.rechargeRate = (float)sqlite3_column_double(stmt, 3);
					if (sqlite3_column_type(stmt, 4) != SQLITE_NULL && sqlite3_column_double(stmt, 4) > 0.0)
						d.accuracy     = (float)sqlite3_column_double(stmt, 4);
					if (sqlite3_column_type(stmt, 5) != SQLITE_NULL && sqlite3_column_double(stmt, 5) > 0.0)
						d.sightRange   = (float)sqlite3_column_double(stmt, 5);
				}
				sqlite3_finalize(stmt);
			}
			it = g_botDefaultsCache.emplace(nPendingBotId, d).first;
		}
		const BotDefaults& d = it->second;
		hitPoints    = d.hitPoints;
		speed        = d.speed;
		powerPool    = d.powerPool;
		rechargeRate = d.rechargeRate;
		accuracy     = d.accuracy;
		sightRange   = d.sightRange;
		nBotId = nPendingBotId;
		nPendingBotId = 0;
	}

	// DIAG: per-respawn buff investigation. Log the values feeding
	// InitializeProperty and the post-init engine power-regen field. If the
	// engine field reads back higher than `rechargeRate` we know a property
	// already existed (duplicate) or something wrote it after our SetProperty.
	Logger::Log("debug",
	    "[InitDefaults] pawn=0x%p botId=%d hp=%.1f speed=%.1f power=%.1f recharge=%.2f sProps.Count(pre)=%d s_fPowerPoolRechargeRate(pre)=%.2f\n",
	    Pawn, nBotId, hitPoints, speed, powerPool, rechargeRate,
	    Pawn->s_Properties.Data ? Pawn->s_Properties.Num() : -1,
	    Pawn->s_fPowerPoolRechargeRate);

	// Per-prop m_fMaximum is a hard CLAMP inside TgPawn::ApplyProperty
	// (FUN_1095bd40 = clamp(raw, min, max)). If skills push HP_MAX above the
	// base, the fanned-out replicated r_nHealthMaximum gets clipped back to
	// the base and the player never sees the buff. Raise the ceiling so
	// skill/item/effect modifiers have room to land. 10x base handles any
	// realistic stack; cap at 1e6 just in case.
	InitializeProperty(Pawn, GA_PROPERTY::TGPID_HEALTH,                  hitPoints,    hitPoints,    0, hitPoints * 10.0f);
	InitializeProperty(Pawn, GA_PROPERTY::TGPID_HEALTH_MAX,              hitPoints,    hitPoints,    0, hitPoints * 10.0f);
	// TGPID_GROUND_SPEED (49): the rest device applies PERC_DECREASE here.
	// Was previously disabled because UC's Remove math leaks per apply/remove
	// cycle (passes effBase instead of propBase*effBase), so repeated rest
	// toggles compounded ground speed toward 0. The global device-fire bracket
	// (see UObject__ProcessEvent.cpp:CaptureBaselineOnBeginState /
	// RestoreBaselineOnStopFire) now snapshots m_fRaw at BeginState and
	// overwrites UC's leaked value with the baseline on ServerStopFire, so
	// it's safe to re-register. Without this line, any effect modifying
	// prop 49 silently no-ops (SetProperty needs the prop in s_Properties).
	InitializeProperty(Pawn, GA_PROPERTY::TGPID_GROUND_SPEED,            speed,        speed,        0, 10000);
	// Same clamp trap as HP above — POWERPOOL_MAX (255) skill buffs like the
	// +40% Power Pool tree node were being computed into UTgProperty.m_fRaw
	// (verified 100 -> 140) but clipped back to 100 at fan-out because
	// m_fMaximum was set to `powerPool`. Raise the ceiling.
	InitializeProperty(Pawn, GA_PROPERTY::TGPID_POWERPOOL,               powerPool,    powerPool,    0, powerPool * 10.0f);
	InitializeProperty(Pawn, GA_PROPERTY::TGPID_POWERPOOL_MAX,           powerPool,    powerPool,    0, powerPool * 10.0f);
	InitializeProperty(Pawn, GA_PROPERTY::TGPID_POWERPOOL_RECHARGE_RATE, rechargeRate, rechargeRate, 0, 1000);
	InitializeProperty(Pawn, GA_PROPERTY::TGPID_POWERPOOL_COST,          0, 0, 0, 0);
	InitializeProperty(Pawn, GA_PROPERTY::TGPID_POWERPOOL_MIN_COST,      0, 0, 0, 0);
	InitializeProperty(Pawn, GA_PROPERTY::TGPID_FLIGHT_ACCELERATION,     0, 0, 0, 1000);
	InitializeProperty(Pawn, GA_PROPERTY::TGPID_ACCURACY,                accuracy,     accuracy,     0, 1);

	// Vision range — TgPawn.uc sets SightRadius from this on dedicated server (GetProperty(152))
	if (nBotId != 680 && nBotId != 681 && nBotId != 679 && nBotId != 567) {
		InitializeProperty(Pawn, GA_PROPERTY::TGPID_CHARACTER_VISION_RANGE,  sightRange,   sightRange,   0, 10000);
	} else {
		// InitializeProperty(Pawn, GA_PROPERTY::TGPID_HEALTH,                  1000000,    1000000,    0, 1000000);
		// InitializeProperty(Pawn, GA_PROPERTY::TGPID_HEALTH_MAX,              1000000,    1000000,    0, 1000000);
	}

	// Air speed — used by PlayerJetting state for lateral movement (TgPlayerController.uc)
	// Engine APawn default AirSpeed = 600.0f. Max raised to 5000 so PERC
	// stacking (stealth's +300%, plus skill/device modifiers) isn't clamped
	// back to 600 at ApplyProperty time — the engine field needs room to
	// actually receive the buffed value.
	InitializeProperty(Pawn, GA_PROPERTY::TGPID_AIR_SPEED,               600.0f, 600.0f, 0, 5000.0f);

	// JumpZ (prop 50): must be in s_Properties so SetProperty(50) can resolve
	// through our GetProperty hook and propagate to the APawn::JumpZ engine
	// field via ApplyProperty — otherwise stealth/buff effects that modify
	// JumpZ silently no-op. BUT: the pawn's UC class defaults (TgPawn_Character
	// and subclasses) already set JumpZ, and hardcoding a value here overwrites
	// that default and makes ambient jumps feel wrong. So read whatever the
	// engine/UC has already populated and register that as the base — which
	// makes the init a no-op for APawn::JumpZ (the SetProperty call inside
	// InitializeProperty writes the same value back) and gives stealth a valid
	// baseline to add to / subtract from.
	float jumpZ = Pawn->JumpZ > 0.0f ? Pawn->JumpZ : 420.0f;  // APawn default fallback
	InitializeProperty(Pawn, GA_PROPERTY::TGPID_JUMPZ, jumpZ, jumpZ, 0, 5000);

	// Gravity Z modifier — replicated at ATgPawn+0x117C. Default 1.0 = normal gravity.
	// Applied in TgPawn physics; 0 would mean no gravity.
	InitializeProperty(Pawn, GA_PROPERTY::TGPID_GRAVITYZ_MODIFIER,       1.0f, 1.0f, 0, 100.0f);

	// Falling damage modifier — server-only at ATgPawn+0x1180.
	// TakeFallingDamage (TgPawn.uc:2784) multiplies fall damage by this value.
	// Returns early if <= 0.0, so it MUST be initialized to a positive value.
	// 1.0 = normal fall damage; 0 = disabled; >1 = amplified.
	InitializeProperty(Pawn, GA_PROPERTY::TGPID_FALLING_DAMAGE_MODIFIER, 1.0f, 1.0f, 0, 100.0f);

	// TGPID_STEALTH_TYPE_CODE (341): UScript class default is r_nStealthTypeCode=1037 (TG_STEALTH_GENERAL).
	// SetProperty(341, 0) overrides that → r_nStealthTypeCode = 0 = no stealth active.
	// Stealth effects will set it to a non-zero value ID (e.g. 1037) when they activate.
	InitializeProperty(Pawn, GA_PROPERTY::TGPID_STEALTH_TYPE_CODE, 0, 0, 0, 10000);

	// Stealth transition time modifier — identity default = 1.0
	InitializeProperty(Pawn, GA_PROPERTY::TGPID_STEALTH_TRANSTIME_MODIFIER, 1.0f, 1.0f, 0, 1000);

	// Protection properties — CalcProtection in TgEffectGroup calls GetProperty(nProtectionType).
	// If the property is absent, SetProperty from damage resistance effects silently does nothing.
	// Initialize all to 0 (no resistance); effects buff them above zero.
	// The "default 30% physical" everyone expects comes from device 864 ("HUMAN BASE
	// ATTRIBUTES", slot 14/Rest) via permanent equip-effect group 3575 — applied by
	// Inventory::ApplyDeviceEquipEffects after CreateEquipDevice, since the asm.dat
	// loader that sets device->m_EquipEffect is one of the stripped natives.
	InitializeProperty(Pawn, GA_PROPERTY::TGPID_PROTECTION_PHYSICAL,     0, 0, 0, 100);
	InitializeProperty(Pawn, GA_PROPERTY::TGPID_PROTECTION_ENERGY,       0, 0, 0, 100);
	InitializeProperty(Pawn, GA_PROPERTY::TGPID_PROTECTION_THERMAL,      0, 0, 0, 100);
	InitializeProperty(Pawn, GA_PROPERTY::TGPID_PROTECTION_SLOW,         0, 0, 0, 100);
	InitializeProperty(Pawn, GA_PROPERTY::TGPID_PROTECTION_POISON,       0, 0, 0, 100);
	InitializeProperty(Pawn, GA_PROPERTY::TGPID_PROTECTION_DISEASE,      0, 0, 0, 100);
	InitializeProperty(Pawn, GA_PROPERTY::TGPID_PROTECTION_STUN,         0, 0, 0, 100);
	InitializeProperty(Pawn, GA_PROPERTY::TGPID_PROTECTION_SLEEP,        0, 0, 0, 100);
	InitializeProperty(Pawn, GA_PROPERTY::TGPID_PROTECTION_KNOCKBACK,    0, 0, 0, 100);
	InitializeProperty(Pawn, GA_PROPERTY::TGPID_PROTECTION_EMP_STUN,     0, 0, 0, 100);
	InitializeProperty(Pawn, GA_PROPERTY::TGPID_PROTECTION_IGNITE,       0, 0, 0, 100);
	InitializeProperty(Pawn, GA_PROPERTY::TGPID_PROTECTION_BIO,          0, 0, 0, 100);
	InitializeProperty(Pawn, GA_PROPERTY::TGPID_PROTECTION_EMP_BURN,     0, 0, 0, 100);
	InitializeProperty(Pawn, GA_PROPERTY::TGPID_PROTECTION_BLEED,        0, 0, 0, 100);
	InitializeProperty(Pawn, GA_PROPERTY::TGPID_PROTECTION_MELEE,        0, 0, 0, 100);
	InitializeProperty(Pawn, GA_PROPERTY::TGPID_PROTECTION_RANGED,       0, 0, 0, 100);
	InitializeProperty(Pawn, GA_PROPERTY::TGPID_PROTECTION_AOE,          0, 0, 0, 100);

	// DIAG: per-respawn buff investigation — read back the engine power-regen
	// field after all SetProperty calls. If higher than `rechargeRate`, the
	// pre/post values diverging on subsequent spawns tells us where the
	// stacking comes from.
	Logger::Log("debug",
	    "[InitDefaults] pawn=0x%p sProps.Count(post)=%d s_fPowerPoolRechargeRate(post)=%.2f r_fMaxPowerPool=%.2f\n",
	    Pawn,
	    Pawn->s_Properties.Data ? Pawn->s_Properties.Num() : -1,
	    Pawn->s_fPowerPoolRechargeRate, Pawn->r_fMaxPowerPool);

	LogCallEnd();
}

