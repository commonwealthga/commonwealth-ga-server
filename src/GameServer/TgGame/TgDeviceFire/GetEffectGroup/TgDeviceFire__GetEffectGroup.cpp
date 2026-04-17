#include "src/GameServer/TgGame/TgDeviceFire/GetEffectGroup/TgDeviceFire__GetEffectGroup.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/Database/Database.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include "src/Utils/Macros.hpp"

// ConstructObject<UTgEffectGroup> (validates IsChildOf UTgEffectGroup::StaticClass())
typedef void*(__cdecl* ConstructEffectGroupFn)(void*, int, int, int, unsigned, unsigned, int, int, int*);
static ConstructEffectGroupFn ConstructEffectGroup = (ConstructEffectGroupFn)0x109a90b0;

// ConstructObject<UTgEffect> (validates IsChildOf UTgEffect::StaticClass(), works for all subclasses)
typedef void*(__cdecl* ConstructEffectFn)(void*, int, int, int, unsigned, unsigned, int, int, int*);
static ConstructEffectFn ConstructEffect = (ConstructEffectFn)0x10a73f30;

// -------------------------------------------------------------------------
// Map class_res_id from asm_data_set_effects to a UClass*
// IDs observed in DB: 80 (base), 89 (visibility), 157 (buff), 181 (damage), 244 (sensor), 692 (heal)
// -------------------------------------------------------------------------
static UClass* GetEffectClassById(int classResId) {
	switch (classResId) {
		case 181: return ClassPreloader::GetClass("Class TgGame.TgEffectDamage");
		case 692: return ClassPreloader::GetClass("Class TgGame.TgEffectHeal");
		case  89: return ClassPreloader::GetClass("Class TgGame.TgEffectVisibility");
		case 157: return ClassPreloader::GetClass("Class TgGame.TgEffectBuff");
		case 244: return ClassPreloader::GetClass("Class TgGame.TgEffectSensor");
		case  80:
		default:  return ClassPreloader::GetClass("Class TgGame.TgEffect");
	}
}

// -------------------------------------------------------------------------
// Build a UTgEffectGroup (with its UTgEffect children) from the database
// -------------------------------------------------------------------------
static UTgEffectGroup* BuildEffectGroup(int egId, int egType) {
	UClass* egClass = ClassPreloader::GetClass("Class TgGame.TgEffectGroup");
	if (!egClass) {
		Logger::Log("effects", "[BUILD] TgEffectGroup class not found\n");
		return nullptr;
	}

	UTgEffectGroup* g = (UTgEffectGroup*)ConstructEffectGroup(egClass, -1, 0, 0, 0, 0, 0, 0, nullptr);
	if (!g) {
		Logger::Log("effects", "[BUILD] ConstructObject failed for egId=%d\n", egId);
		return nullptr;
	}

	sqlite3* db = Database::GetConnection();

	// --- Fill UTgEffectGroup fields ---
	{
		sqlite3_stmt* stmt = nullptr;
		int rc = sqlite3_prepare_v2(db,
			"SELECT lifetime_sec, apply_interval_sec, application_value_id, "
			"application_value, application_chance, category_value_id, "
			"required_category_value_id, required_skill_id, situational_type_value_id, "
			"situational_value, stack_count_max, buff_value, contagion_flag, "
			"device_specific_flag, posture_type_value_id, health "
			"FROM asm_data_set_effect_groups WHERE effect_group_id = ? LIMIT 1",
			-1, &stmt, nullptr);
		if (rc == SQLITE_OK) {
			sqlite3_bind_int(stmt, 1, egId);
			if (sqlite3_step(stmt) == SQLITE_ROW) {
				g->m_nEffectGroupId    = egId;
				g->m_nType             = egType;
				g->m_fLifeTime         = (float)sqlite3_column_double(stmt,  0);
				g->m_fApplyInterval    = (float)sqlite3_column_double(stmt,  1);
				g->m_nApplicationType  = sqlite3_column_int   (stmt,  2);
				g->m_fApplicationValue = (float)sqlite3_column_double(stmt,  3);
				g->m_fApplicationChance= (float)sqlite3_column_double(stmt,  4);
				g->m_nCategoryCode     = sqlite3_column_int   (stmt,  5);
				g->m_nReqCategoryCode  = sqlite3_column_int   (stmt,  6);
				g->m_nRequiredSkillId  = sqlite3_column_int   (stmt,  7);
				g->m_nSituationalType  = sqlite3_column_int   (stmt,  8);
				g->m_fSituationalValue = (float)sqlite3_column_double(stmt,  9);
				g->m_nNumMaxStacks     = sqlite3_column_int   (stmt, 10);
				g->m_fBuffValue        = (float)sqlite3_column_double(stmt, 11);
				g->m_nPosture          = sqlite3_column_int   (stmt, 14);
				g->m_nHealth           = sqlite3_column_int   (stmt, 15);

				unsigned int& flags = *(unsigned int*)((char*)g + 0x74);
				if (sqlite3_column_int(stmt, 12)) flags |= 0x10; else flags &= ~0x10; // m_bContagious
				if (sqlite3_column_int(stmt, 13)) flags |= 0x20; else flags &= ~0x20; // m_bDeviceSpecificFlag
			}
			sqlite3_finalize(stmt);
		}
	}

	// --- Build UTgEffect children ---
	{
		sqlite3_stmt* stmt = nullptr;
		int rc = sqlite3_prepare_v2(db,
			"SELECT class_res_id, prop_id, base_value, min_value, max_value, "
			"calc_method_value_id, property_value_id, apply_on_interval_flag "
			"FROM asm_data_set_effects WHERE effect_group_id = ?",
			-1, &stmt, nullptr);
		if (rc == SQLITE_OK) {
			sqlite3_bind_int(stmt, 1, egId);

			TARRAY_INIT(g, effects, UTgEffect*, 0x60, 64)

			while (sqlite3_step(stmt) == SQLITE_ROW) {
				UClass* effectClass = GetEffectClassById(sqlite3_column_int(stmt, 0));
				if (!effectClass) continue;

				UTgEffect* e = (UTgEffect*)ConstructEffect(effectClass, -1, 0, 0, 0, 0, 0, 0, nullptr);
				if (!e) continue;

				e->m_EffectGroup      = g;
				e->m_nPropertyId      = sqlite3_column_int   (stmt, 1);
				e->m_fBase            = (float)sqlite3_column_double(stmt, 2);
				e->m_fMinimum         = (float)sqlite3_column_double(stmt, 3);
				e->m_fMaximum         = (float)sqlite3_column_double(stmt, 4);
				e->m_nCalcMethodCode  = sqlite3_column_int   (stmt, 5);
				e->m_nPropertyValueId = sqlite3_column_int   (stmt, 6);

				// Flags at +0x48: bit 0x01 = m_bUseOnInterval, bit 0x02 = m_bRemovable.
				// CDO defaults m_bRemovable=true for UTgEffect (TgEffect.uc:662) so
				// apply/remove pairs go through the symmetric `ApplyEventBasedEffects`
				// → `TgEffect.Remove` → `ApplyToProperty(bRemove=true)` path. We
				// previously cleared this for lifetime==0 groups on the assumption
				// that "lifetime=0 means instant damage", but many lifetime=0
				// groups are *hold-to-sustain* (scope/aim nType=266, stealth,
				// rest): they must remain removable so scope-out actually reverses
				// the ground-speed debuff via UC's Remove path. Leaving the CDO
				// default in place is correct.
				unsigned int& eflags = *(unsigned int*)((char*)e + 0x48);
				if (sqlite3_column_int(stmt, 7)) eflags |= 0x01; else eflags &= ~0x01;

				TARRAY_ADD(effects, e)
			}
			sqlite3_finalize(stmt);
		}
	}

	// Stealth-category (621) effect groups in the DB have lifetime=0
	// (e.g. egId 9315 for Spring Stealth, slot 981 / device 3023). Promote
	// to a SHORT 1s lifetime to make the group register a HUD buff icon via
	// SetEffectRep. The generic hold-to-sustain path (scope, rest) works
	// fine with lifetime=0 — they don't need a HUD icon — but stealth wants
	// a visible countdown. The UC effect manager's re-submit logic for
	// application_value=156 ("Newest Wins") ignores duplicate submissions
	// instead of refreshing the timer, so we refresh ourselves from
	// `DeviceFiring.RefireCheckTimer` in UObject__ProcessEvent.cpp — each
	// refire resets the LifeDone timer's elapsed counter and re-replicates
	// the full time remaining. On fire-release, ServerStopFire explicitly
	// clears category=621 for immediate HUD teardown.
	if (g->m_nCategoryCode == 621 && g->m_fLifeTime == 0.0f) {
		g->m_fLifeTime = 1.0f;
		Logger::Log("effects",
			"[BUILD] egId=%d promoted stealth lifetime 0 -> 1s (HUD icon)\n", egId);
	}

	// Set m_bIsManaged (bit 0x01 at offset 0x74). UC's *apply* gate at
	// `TgEffectManager.ProcessEffect:231` is `(lifetime > 0 OR m_bIsManaged)`
	// — without either, `ApplyEventBasedEffects` never runs and removable
	// effects don't apply. UC's *remove* gate at `TgDeviceFire.RemoveEffectType`
	// is `theEffectGroup.m_bIsManaged || bForceRemove` — `RemoveAimEffects`,
	// `RemoveRestEffects`, and all non-forced remove paths pass bForceRemove=false,
	// so the remove is silently skipped unless the TEMPLATE has m_bIsManaged=true.
	//
	// Every built effect group represents a tracked apply/remove lifecycle
	// (application_value ∈ {155 Stacking, 156 Newest Wins, 157 Strongest,
	// 836 Refresh, 874 SameInstigator} per the DB) — there are no pure
	// "fire-and-forget instant" groups in asm_data_set_effect_groups. So
	// mark every built group as managed. Apply becomes reliable, Remove
	// becomes reliable, compounding goes away.
	{
		unsigned int& gflags = *(unsigned int*)((char*)g + 0x74);
		gflags |= 0x01; // m_bIsManaged
	}

	{
		unsigned int gflags = *(unsigned int*)((char*)g + 0x74);
		Logger::Log("effects",
			"[BUILD] egId=%d egType=%d effects=%d lifetime=%.2f category=%d app=%d bIsManaged=%d\n",
			egId, egType, g->m_Effects.Count, g->m_fLifeTime, g->m_nCategoryCode,
			g->m_nApplicationType, (gflags & 0x01) ? 1 : 0);
		for (int i = 0; i < g->m_Effects.Count; i++) {
			UTgEffect* e = g->m_Effects.Data[i];
			if (!e) continue;
			unsigned int eflags = *(unsigned int*)((char*)e + 0x48);
			Logger::Log("effects",
				"[BUILD]   effect[%d]: propId=%d calc=%d base=%.3f min=%.3f max=%.3f bRemovable=%d bUseOnInterval=%d\n",
				i, e->m_nPropertyId, e->m_nCalcMethodCode, e->m_fBase,
				e->m_fMinimum, e->m_fMaximum,
				(eflags & 0x02) ? 1 : 0, (eflags & 0x01) ? 1 : 0);
		}
	}

	return g;
}

// -------------------------------------------------------------------------
// Hook
// -------------------------------------------------------------------------
UTgEffectGroup* __fastcall TgDeviceFire__GetEffectGroup::Call(UTgDeviceFire* pThis, void* edx, int nType, int* nIndex) {
	if (!pThis) {
		if (nIndex) *nIndex = -1;
		return nullptr;
	}

	// Lazy-populate s_EffectGroupList from m_pFireModeSetup native data + DB
	if (pThis->s_EffectGroupList.Count == 0) {
		void* pFireMode = *(void**)((char*)pThis + 0x7C); // m_pFireModeSetup
		if (pFireMode) {
			int** begin = *(int***)((char*)pFireMode + 0x98);
			int** end   = *(int***)((char*)pFireMode + 0x9C);

			// Read fire mode's damage type (UTgDeviceFire+0xBC) and attack type (+0x88)
			// to propagate onto each effect group for CalcDamageTypeProtection / CalcAttackTypeProtection.
			int   fmDamageType = *(int*)((char*)pThis + 0xBC);
			int   fmAttackType = *(int*)((char*)pThis + 0x88);

			// Map fire-mode attack-type value ID to TG_EQUIP_ATTACK_TYPE enum:
			//   170 (0xAA) = melee → 1, 85 (0x55) / 177 (0xB1) = ranged → 2
			unsigned char eAttackType = 0;
			if (fmAttackType == 170)                       eAttackType = 1; // TGEAT_MELEE
			else if (fmAttackType == 85 || fmAttackType == 177) eAttackType = 2; // TGEAT_RANGE

			TARRAY_INIT(pThis, egList, UTgEffectGroup*, 0x48, 8)

			for (int** pp = begin; pp != end; pp++) {
				int egId   = (*pp)[0];
				int egType = (*pp)[1];
				UTgEffectGroup* g = BuildEffectGroup(egId, egType);
				if (g) {
					g->m_nDamageType = fmDamageType;
					g->m_eAttackType = eAttackType;
					TARRAY_ADD(egList, g)
				}
			}
		}
	}

	// Search for a group matching nType
	int startIdx = (nIndex && *nIndex >= 0) ? *nIndex : 0;
	TArray<UTgEffectGroup*>& list = pThis->s_EffectGroupList;
	for (int i = startIdx; i < list.Count; i++) {
		UTgEffectGroup* g = list.Data[i];
		if (g && g->m_nType == nType) {
			if (nIndex) *nIndex = i;
			Logger::Log("effects",
				"[GET] fireMode=%s nType=%d -> egId=%d (startIdx=%d resultIdx=%d)\n",
				pThis->GetFullName(), nType, g->m_nEffectGroupId, startIdx, i);
			return g;
		}
	}

	if (nIndex) *nIndex = -1;
	Logger::Log("effects",
		"[GET] fireMode=%s nType=%d -> NONE (searched from %d in list of %d)\n",
		pThis->GetFullName(), nType, startIdx, list.Count);
	return nullptr;
}
