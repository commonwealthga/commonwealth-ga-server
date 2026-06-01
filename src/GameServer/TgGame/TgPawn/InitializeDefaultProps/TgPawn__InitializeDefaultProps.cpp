#include "src/GameServer/TgGame/TgPawn/InitializeDefaultProps/TgPawn__InitializeDefaultProps.hpp"
#include "src/GameServer/Constants/GameTypes.h"
#include "src/GameServer/Core/TMap/Allocate/TMap__Allocate.hpp"
#include "src/GameServer/Constants/TgProperties.h"
#include "src/Database/Database.hpp"
#include "src/Config/Config.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include <cmath>

int TgPawn__InitializeDefaultProps::nPendingBotId = 0;
bool TgPawn__InitializeDefaultProps::bPendingEnemyScaling = false;
float TgPawn__InitializeDefaultProps::nPendingDifficultyScalarOverride = 0.0f;

namespace {
struct BotDefaults {
	float hitPoints;
	float speed;
	float powerPool;
	float rechargeRate;
	float accuracy;
	float sightRange;
	// Per-bot stat scale from asm_data_set_bots.bot_balance_multiplier.
	// Designer-applied multi-difficulty stat tag — most bots = 1.0, elite/boss
	// Colony tier = 1.1–1.7, two test sentinels = 10.0. BBM=0 is filtered out
	// upstream in EnsureSpawnTablesLoaded (= "don't spawn at all"), so a 0
	// reaching this struct only happens for non-factory spawns (deployables,
	// pets) where we fall back to no multiplier.
	float balanceMultiplier;
	// asm_data_set_bots.fixed_fov_degrees — total vision arc in degrees that
	// the AI controller uses for SeePlayer / target acquisition. Personal
	// Turret = 180, Auto Cannon = 90, Plasma Turret = 180, Flame/Objective
	// Turret = 360. 0 = "use UC default" (TgPawn.uc:10535 sets 0.5 → 120° arc).
	int fixedFovDegrees;
};
// Per-bot defaults cache. Populated lazily on the first InitializeDefaultProps
// call for a given bot_id; subsequent spawns of the same bot reuse the row
// without hitting SQLite. The DB rows are static reference data (loaded from
// assembly.dat at startup), so once cached they don't need invalidation.
std::unordered_map<int, BotDefaults> g_botDefaultsCache;
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

	float balanceMultiplier = 1.0f;
	int   fixedFovDegrees   = 0; // 0 = leave UC default (0.5 → 120° arc)

	if (nPendingBotId != 0) {
		auto it = g_botDefaultsCache.find(nPendingBotId);
		if (it == g_botDefaultsCache.end()) {
			// First spawn of this bot — query the DB row and cache it.
			BotDefaults d{ hitPoints, speed, powerPool, rechargeRate, accuracy, sightRange, balanceMultiplier, fixedFovDegrees };
			sqlite3* db = Database::GetConnection();
			sqlite3_stmt* stmt;
			if (sqlite3_prepare_v2(db,
				"SELECT hit_points, default_speed, default_power_pool, power_pool_regen_per_sec, accuracy_override, default_sensor_range, bot_balance_multiplier, fixed_fov_degrees "
				"FROM asm_data_set_bots WHERE bot_id = ? LIMIT 1",
				-1, &stmt, nullptr) == SQLITE_OK) {
				sqlite3_bind_int(stmt, 1, nPendingBotId);
				if (sqlite3_step(stmt) == SQLITE_ROW) {
					if (sqlite3_column_type(stmt, 0) != SQLITE_NULL && sqlite3_column_int(stmt, 0) > 0)
						d.hitPoints    = (float)sqlite3_column_int(stmt, 0);
					// if (sqlite3_column_type(stmt, 1) != SQLITE_NULL && sqlite3_column_double(stmt, 1) > 0.0)
						d.speed        = (float)sqlite3_column_double(stmt, 1) * 16.0f;
					// if (sqlite3_column_type(stmt, 2) != SQLITE_NULL && sqlite3_column_int(stmt, 2) > 0)
						d.powerPool    = (float)sqlite3_column_int(stmt, 2);
					// if (sqlite3_column_type(stmt, 3) != SQLITE_NULL && sqlite3_column_double(stmt, 3) > 0.0)
						d.rechargeRate = (float)sqlite3_column_double(stmt, 3);
					if (sqlite3_column_type(stmt, 4) != SQLITE_NULL && sqlite3_column_double(stmt, 4) > 0.0)
						d.accuracy     = (float)sqlite3_column_double(stmt, 4);
					if (sqlite3_column_type(stmt, 5) != SQLITE_NULL && sqlite3_column_double(stmt, 5) > 0.0)
						d.sightRange   = (float)sqlite3_column_double(stmt, 5);
					if (sqlite3_column_type(stmt, 6) != SQLITE_NULL)
						d.balanceMultiplier = (float)sqlite3_column_double(stmt, 6);
					if (sqlite3_column_type(stmt, 7) != SQLITE_NULL && sqlite3_column_int(stmt, 7) > 0)
						d.fixedFovDegrees = sqlite3_column_int(stmt, 7);
				}
				sqlite3_finalize(stmt);
			}
			it = g_botDefaultsCache.emplace(nPendingBotId, d).first;
		}
		const BotDefaults& d = it->second;
		hitPoints         = d.hitPoints;
		speed             = d.speed;
		powerPool         = d.powerPool;
		rechargeRate      = d.rechargeRate;
		accuracy          = d.accuracy;
		sightRange        = d.sightRange;
		balanceMultiplier = d.balanceMultiplier;
		fixedFovDegrees   = d.fixedFovDegrees;
		nBotId = nPendingBotId;
		nPendingBotId = 0;
	}

	// Gate enemy difficulty scaling on the bPendingEnemyScaling signal that
	// SpawnBotById (with non-null factory) or SpawnObjectiveBot raises right
	// before invoking the spawn. Players (SpawnPlayerCharacter sets
	// nPendingBotId too but never raises this flag), player-side pets/decoys/
	// turrets, deployables, and anything else that reaches this hook without
	// raising the flag stay at raw stats.
	const bool scaleAsEnemy = bPendingEnemyScaling;
	bPendingEnemyScaling = false;

	// Per-spawn scalar override (-spawnfriend/-spawnenemy chat command). 0 =
	// "use map default" (Config::GetDifficultyScalar). Consume + clear here
	// even if scaling is gated off, so a leftover from a chat spawn that
	// somehow skipped the gate can't leak into a later factory spawn.
	const float scalarOverride = nPendingDifficultyScalarOverride;
	nPendingDifficultyScalarOverride = 0.0f;
	const float difficultyScalar = scalarOverride > 0.0f
		? scalarOverride
		: Config::GetDifficultyScalar();

	// Combined stat scale = per-bot BBM × per-difficulty scalar.
	// BBM=0 is the "never spawn" sentinel — already filtered upstream from
	// the enemy bot-factory pipeline, so we only reach this branch with BBM=0
	// for non-factory spawns. BBM>0: multiply HP and seed outgoing damage
	// modifier (prop 65). Power pool is intentionally not scaled. At Ultra-Max
	// (scalar 3.0), a plain BBM=1.0 enemy ends up at 3× HP and +200% damage;
	// an elite BBM=1.7 Colony Soldier ends up at 5.1× HP and +410% damage.
	const float combinedMultiplier = (scaleAsEnemy && balanceMultiplier > 0.0f)
		? balanceMultiplier * difficultyScalar
		: 1.0f;
	if (combinedMultiplier != 1.0f) {
		hitPoints *= combinedMultiplier;
	}

	// DIAG: per-respawn buff investigation. Log the values feeding
	// AddProperty and the post-init engine power-regen field. If the
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
	// Order matters: ApplyProperty fanout for TGPID_HEALTH (51) clamps the
	// value against r_nHealthMaximum at +0x43c, which is written by
	// TGPID_HEALTH_MAX (304)'s fanout. With 51 first, the clamp ceiling is 0
	// → m_fRaw is forced to 0, and any subsequent additive heal effect adds
	// to 0 instead of the spawn HP. Same trap below for POWERPOOL/_MAX.
	Pawn->AddProperty( GA_PROPERTY::TGPID_HEALTH_MAX,              hitPoints,    hitPoints,    0, hitPoints * 10.0f);
	Pawn->AddProperty( GA_PROPERTY::TGPID_HEALTH,                  hitPoints,    hitPoints,    0, hitPoints * 10.0f);
	// TGPID_GROUND_SPEED (49): the rest device applies PERC_DECREASE here.
	// Was previously disabled because UC's Remove math leaks per apply/remove
	// cycle (passes effBase instead of propBase*effBase), so repeated rest
	// toggles compounded ground speed toward 0. The global device-fire bracket
	// (see UObject__ProcessEvent.cpp:CaptureBaselineOnBeginState /
	// RestoreBaselineOnStopFire) now snapshots m_fRaw at BeginState and
	// overwrites UC's leaked value with the baseline on ServerStopFire, so
	// it's safe to re-register. Without this line, any effect modifying
	// prop 49 silently no-ops (SetProperty needs the prop in s_Properties).
	Pawn->AddProperty( GA_PROPERTY::TGPID_GROUND_SPEED,            speed,        speed,        0, 10000);
	// Same clamp trap as HP above — POWERPOOL_MAX (255) skill buffs like the
	// +40% Power Pool tree node were being computed into UTgProperty.m_fRaw
	// (verified 100 -> 140) but clipped back to 100 at fan-out because
	// m_fMaximum was set to `powerPool`. Raise the ceiling.
	// MAX before CURRENT (see HEALTH note above) — TGPID_POWERPOOL fanout
	// clamps to r_fMaxPowerPool which is written by TGPID_POWERPOOL_MAX.
	Pawn->AddProperty( GA_PROPERTY::TGPID_POWERPOOL_MAX,           powerPool,    powerPool,    0, powerPool * 10.0f);
	Pawn->AddProperty( GA_PROPERTY::TGPID_POWERPOOL,               powerPool,    powerPool,    0, powerPool * 10.0f);
	Pawn->AddProperty( GA_PROPERTY::TGPID_POWERPOOL_RECHARGE_RATE, rechargeRate, rechargeRate, 0, 1000);
	Pawn->AddProperty( GA_PROPERTY::TGPID_POWERPOOL_COST,          0, 0, 0, 0);
	Pawn->AddProperty( GA_PROPERTY::TGPID_POWERPOOL_MIN_COST,      0, 0, 0, 0);
	Pawn->AddProperty( GA_PROPERTY::TGPID_FLIGHT_ACCELERATION,     0, 0, 0, 1000);
	Pawn->AddProperty( GA_PROPERTY::TGPID_ACCURACY,                accuracy,     accuracy,     0, 1);

	// Outgoing-damage scale from combined BBM × difficulty.
	//
	// Going via AddProperty(TGPID_DAMAGE_MODIFIER / prop 65) does nothing —
	// ATgPawn::ApplyProperty has no case for 65 (default branch: no field
	// write), and no UC code reads s_Properties[65] during damage calc. The
	// real damage-multiplier consumer is TgEffectDamage.uc:120:
	//     fProratedAmount *= InstigatorPawn.s_fDamageAdjustment;
	// (also TgEffectHeal.uc:71 — heals scale too, which is what we want for
	// elite healer/medic bots). The float defaults to 1.0 in TgPawn.uc and is
	// never written by anything else, so a direct write here is the canonical
	// init-time knob. Skip when combined == 1.0 (no-op — players and
	// non-factory spawns land here so their output stays vanilla).
	if (combinedMultiplier != 1.0f) {
		Pawn->s_fDamageAdjustment = combinedMultiplier;
	}

	// Vision range — TgPawn.uc sets SightRadius from this on dedicated server (GetProperty(152))
	// if (nBotId != 680 && nBotId != 681 && nBotId != 679 && nBotId != 567) {
		Pawn->AddProperty( GA_PROPERTY::TGPID_CHARACTER_VISION_RANGE,  sightRange,   sightRange,   0, 10000);
	// } else {
	// 	// Pawn->AddProperty(GA_PROPERTY::TGPID_HEALTH,                  1000000,    1000000,    0, 1000000);
	// 	// Pawn->AddProperty(GA_PROPERTY::TGPID_HEALTH_MAX,              1000000,    1000000,    0, 1000000);
	// }

	// Air speed — used by PlayerJetting state for lateral movement
	// (TgPlayerController.uc:6344 — `newAccel = Normal(...) * Pawn.AirSpeed / 2`).
	//
	// Read whatever the engine/UC CDO has already populated and register that
	// as the base. Engine.Pawn default is 600 but TgPawn overrides to 480
	// (TgPawn.uc:10539), and several flying-bot subclasses override further
	// (TgPawn_Hover/FlyingBoss/AttackTransport/ColonyEye = 800, TgPawn_NewWasp
	// = 3200). Hardcoding any single value here clobbers all those subclass
	// defaults — same trap the JumpZ block below documents. Use the CDO value
	// when > 0, fall back to TgPawn's UC default 480 otherwise.
	//
	// Max raised to a large multiple of base so PERC stacking (stealth's
	// +300%, plus skill/device modifiers) isn't clamped at the base value
	// during ApplyProperty fanout — the engine field needs room to receive
	// the buffed value.
	float airSpeed = Pawn->AirSpeed > 0.0f ? Pawn->AirSpeed : 480.0f;
	Pawn->AddProperty( GA_PROPERTY::TGPID_AIR_SPEED, airSpeed, airSpeed, 0, airSpeed * 10.0f);

	// JumpZ (prop 50): must be in s_Properties so SetProperty(50) can resolve
	// through our GetProperty hook and propagate to the APawn::JumpZ engine
	// field via ApplyProperty — otherwise stealth/buff effects that modify
	// JumpZ silently no-op. BUT: the pawn's UC class defaults (TgPawn_Character
	// and subclasses) already set JumpZ, and hardcoding a value here overwrites
	// that default and makes ambient jumps feel wrong. So read whatever the
	// engine/UC has already populated and register that as the base — which
	// makes the init a no-op for APawn::JumpZ (the SetProperty call inside
	// AddProperty writes the same value back) and gives stealth a valid
	// baseline to add to / subtract from.
	float jumpZ = Pawn->JumpZ > 0.0f ? Pawn->JumpZ : 420.0f;  // APawn default fallback
	Pawn->AddProperty( GA_PROPERTY::TGPID_JUMPZ, jumpZ, jumpZ, 0, 5000);

	// Gravity Z modifier — replicated at ATgPawn+0x117C. Default 1.0 = normal gravity.
	// Applied in TgPawn physics; 0 would mean no gravity.
	Pawn->AddProperty( GA_PROPERTY::TGPID_GRAVITYZ_MODIFIER,       1.0f, 1.0f, 0, 100.0f);

	// Deploy rate (prop 278) — `r_fDeployRate` lives at ATgPawn+0x1034 and is
	// what `TgPawn_Turret.uc:TickDeploy` multiplies DeltaSeconds by each tick:
	// `r_fCurrentDeployTime += DeltaSeconds * r_fDeployRate`. Repair-arm
	// effect groups carry a prop-278 effect (class TgEffect, calc 67 ADD,
	// base +4.5, lifetime 1s, eg-type 264) that fires `SetProperty(target, 278,
	// raw+4.5)` on hit; without seeding the slot here, the linear scan in
	// SetProperty finds nothing and the write silently no-ops, leaving turret
	// deploy rate locked at 1.0 even while a teammate is repairing it.
	// Identity 1.0 matches APawn's r_fDeployRate UC default.
	Pawn->AddProperty( GA_PROPERTY::TGPID_DEPLOY_RATE_MODIFIER,    1.0f, 1.0f, 0, 100.0f);

	// Falling damage modifier — server-only at ATgPawn+0x1180.
	// TakeFallingDamage (TgPawn.uc:2784) multiplies fall damage by this value.
	// Returns early if <= 0.0, so it MUST be initialized to a positive value.
	// 1.0 = normal fall damage; 0 = disabled; >1 = amplified.
	Pawn->AddProperty( GA_PROPERTY::TGPID_FALLING_DAMAGE_MODIFIER, 1.0f, 1.0f, 0, 100.0f);

	// TGPID_STEALTH_TYPE_CODE (341): UScript class default is r_nStealthTypeCode=1037 (TG_STEALTH_GENERAL).
	// SetProperty(341, 0) overrides that → r_nStealthTypeCode = 0 = no stealth active.
	// Stealth effects will set it to a non-zero value ID (e.g. 1037) when they activate.
	Pawn->AddProperty( GA_PROPERTY::TGPID_STEALTH_TYPE_CODE, 0, 0, 0, 10000);

	// Stealth transition time modifier — identity default = 1.0
	Pawn->AddProperty( GA_PROPERTY::TGPID_STEALTH_TRANSTIME_MODIFIER, 1.0f, 1.0f, 0, 1000);

	// MakeVisible Fade Rate — UC's stealth-end logic reads this to decide how
	// fast the player fades back to visible after firing/taking damage.
	// Skill `Stealth Restealth` (581) does +50% via class-80 TgEffect calc=68;
	// without the prop in s_Properties the direct-apply silently no-ops.
	// Identity default 1.0 so the +% scaling lands as a real delta.
	Pawn->AddProperty( GA_PROPERTY::TGPID_MAKEVISIBLE_FADE_RATE, 1.0f, 1.0f, 0, 100.0f);

	// Protection properties — CalcProtection in TgEffectGroup calls GetProperty(nProtectionType).
	// If the property is absent, SetProperty from damage resistance effects silently does nothing.
	// Initialize all to 0 (no resistance); effects buff them above zero.
	// The "default 30% physical" everyone expects comes from device 864 ("HUMAN BASE
	// ATTRIBUTES", slot 14/Rest) via permanent equip-effect group 3575 — applied by
	// Inventory::ApplyDeviceEquipEffects after CreateEquipDevice, since the asm.dat
	// loader that sets device->m_EquipEffect is one of the stripped natives.
	Pawn->AddProperty( GA_PROPERTY::TGPID_PROTECTION_PHYSICAL,     0, 0, 0, 1000.0f);
	Pawn->AddProperty( GA_PROPERTY::TGPID_PROTECTION_ENERGY,       0, 0, 0, 1000.0f);
	Pawn->AddProperty( GA_PROPERTY::TGPID_PROTECTION_THERMAL,      0, 0, 0, 1000.0f);
	Pawn->AddProperty( GA_PROPERTY::TGPID_PROTECTION_SLOW,         0, 0, 0, 1000.0f);
	Pawn->AddProperty( GA_PROPERTY::TGPID_PROTECTION_POISON,       0, 0, 0, 1000.0f);
	Pawn->AddProperty( GA_PROPERTY::TGPID_PROTECTION_DISEASE,      0, 0, 0, 1000.0f);
	Pawn->AddProperty( GA_PROPERTY::TGPID_PROTECTION_STUN,         0, 0, 0, 1000.0f);
	Pawn->AddProperty( GA_PROPERTY::TGPID_PROTECTION_SLEEP,        0, 0, 0, 1000.0f);
	Pawn->AddProperty( GA_PROPERTY::TGPID_PROTECTION_KNOCKBACK,    0, 0, 0, 1000.0f);
	Pawn->AddProperty( GA_PROPERTY::TGPID_PROTECTION_EMP_STUN,     0, 0, 0, 1000.0f);
	Pawn->AddProperty( GA_PROPERTY::TGPID_PROTECTION_IGNITE,       0, 0, 0, 1000.0f);
	Pawn->AddProperty( GA_PROPERTY::TGPID_PROTECTION_BIO,          0, 0, 0, 1000.0f);
	Pawn->AddProperty( GA_PROPERTY::TGPID_PROTECTION_EMP_BURN,     0, 0, 0, 1000.0f);
	Pawn->AddProperty( GA_PROPERTY::TGPID_PROTECTION_BLEED,        0, 0, 0, 1000.0f);
	Pawn->AddProperty( GA_PROPERTY::TGPID_PROTECTION_MELEE,        0, 0, 0, 1000.0f);
	Pawn->AddProperty( GA_PROPERTY::TGPID_PROTECTION_RANGED,       0, 0, 0, 1000.0f);
	Pawn->AddProperty( GA_PROPERTY::TGPID_PROTECTION_AOE,          0, 0, 0, 1000.0f);

	// Vision arc — APawn::PeripheralVision (offset 0x0228) is the cosine of
	// the half-arc the AI uses for SeePlayer / target acquisition. UC default
	// from TgPawn.uc:10535 is 0.5 (= 120° total arc) which is too narrow for
	// turrets that should see ±90° (Personal/Plasma, fixed_fov_degrees=180)
	// and totally wrong for 360° turrets (Flame/Objective/Base).
	//
	// Formula matches UC's ApplyScannerSettingsToPawn (TgPawn.uc:7290):
	//     PeripheralVision = cos(fov_degrees * PI / 360)
	// so 360 → cos(180°) = -1.0 (see in all directions), 180 → 0.0,
	// 90 → ≈0.707, 45 → ≈0.924.
	//
	// Only write when fixedFovDegrees > 0 (DB value present) — pawns without
	// a bot row (players, deployables) keep the UC default.
	if (fixedFovDegrees > 0) {
		Pawn->PeripheralVision = std::cos((float)fixedFovDegrees * 3.14159265f / 360.0f);
	}

	// DIAG: per-respawn buff investigation — read back the engine power-regen
	// field after all SetProperty calls. If higher than `rechargeRate`, the
	// pre/post values diverging on subsequent spawns tells us where the
	// stacking comes from.
	Logger::Log("debug",
	    "[InitDefaults] pawn=0x%p sProps.Count(post)=%d s_fPowerPoolRechargeRate(post)=%.2f r_fMaxPowerPool=%.2f fov=%d PeripheralVision=%.3f\n",
	    Pawn,
	    Pawn->s_Properties.Data ? Pawn->s_Properties.Num() : -1,
	    Pawn->s_fPowerPoolRechargeRate, Pawn->r_fMaxPowerPool,
	    fixedFovDegrees, Pawn->PeripheralVision);

	LogCallEnd();
}

