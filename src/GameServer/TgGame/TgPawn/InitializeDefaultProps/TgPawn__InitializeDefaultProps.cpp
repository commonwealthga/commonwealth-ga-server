#include "src/GameServer/TgGame/TgPawn/InitializeDefaultProps/TgPawn__InitializeDefaultProps.hpp"
#include "src/GameServer/Constants/GameTypes.h"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/TgGame/TgProperty/ConstructTgProperty/TgProperty__ConstructTgProperty.hpp"
#include "src/GameServer/Core/TMap/Allocate/TMap__Allocate.hpp"
#include "src/GameServer/Constants/TgProperties.h"
#include "src/Database/Database.hpp"
#include "src/Utils/Logger/Logger.hpp"

int TgPawn__InitializeDefaultProps::nPendingBotId = 0;

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
		sqlite3* db = Database::GetConnection();
		sqlite3_stmt* stmt;
		if (sqlite3_prepare_v2(db,
			"SELECT hit_points, default_speed, default_power_pool, power_pool_regen_per_sec, accuracy_override, default_sensor_range "
			"FROM asm_data_set_bots WHERE bot_id = ? LIMIT 1",
			-1, &stmt, nullptr) == SQLITE_OK) {
			sqlite3_bind_int(stmt, 1, nPendingBotId);
			if (sqlite3_step(stmt) == SQLITE_ROW) {
				if (sqlite3_column_type(stmt, 0) != SQLITE_NULL && sqlite3_column_int(stmt, 0) > 0)
					hitPoints    = (float)sqlite3_column_int(stmt, 0);
				if (sqlite3_column_type(stmt, 1) != SQLITE_NULL && sqlite3_column_double(stmt, 1) > 0.0)
					speed        = (float)sqlite3_column_double(stmt, 1) * 16.0f;
				if (sqlite3_column_type(stmt, 2) != SQLITE_NULL && sqlite3_column_int(stmt, 2) > 0)
					powerPool    = (float)sqlite3_column_int(stmt, 2);
				if (sqlite3_column_type(stmt, 3) != SQLITE_NULL && sqlite3_column_double(stmt, 3) > 0.0)
					rechargeRate = (float)sqlite3_column_double(stmt, 3);
				if (sqlite3_column_type(stmt, 4) != SQLITE_NULL && sqlite3_column_double(stmt, 4) > 0.0)
					accuracy     = (float)sqlite3_column_double(stmt, 4);
				if (sqlite3_column_type(stmt, 5) != SQLITE_NULL && sqlite3_column_double(stmt, 5) > 0.0)
					sightRange   = (float)sqlite3_column_double(stmt, 5);
			}
			sqlite3_finalize(stmt);
		}
		nBotId = nPendingBotId;
		nPendingBotId = 0;
	}

	InitializeProperty(Pawn, GA_PROPERTY::TGPID_HEALTH,                  hitPoints,    hitPoints,    0, hitPoints);
	InitializeProperty(Pawn, GA_PROPERTY::TGPID_HEALTH_MAX,              hitPoints,    hitPoints,    0, hitPoints);
	// InitializeProperty(Pawn, GA_PROPERTY::TGPID_GROUND_SPEED,            speed,        speed,        0, 10000);
	InitializeProperty(Pawn, GA_PROPERTY::TGPID_POWERPOOL,               powerPool,    powerPool,    0, powerPool);
	InitializeProperty(Pawn, GA_PROPERTY::TGPID_POWERPOOL_MAX,           powerPool,    powerPool,    0, powerPool);
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
	// Engine APawn default AirSpeed = 600.0f
	InitializeProperty(Pawn, GA_PROPERTY::TGPID_AIR_SPEED,               600.0f, 600.0f, 0, 600.0f);

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
	InitializeProperty(Pawn, GA_PROPERTY::TGPID_PROTECTION_PHYSICAL,     30.0f, 0, 0, 100);
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

	LogCallEnd();
}

