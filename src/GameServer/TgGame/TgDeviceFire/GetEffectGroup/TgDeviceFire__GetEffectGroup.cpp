#include "src/GameServer/TgGame/TgDeviceFire/GetEffectGroup/TgDeviceFire__GetEffectGroup.hpp"
#include "src/GameServer/TgGame/BuffEffectRegistry/BuffEffectRegistry.hpp"
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
// Full-scan GObjObjects for a class by full name. Fallback when
// ObjectCache misses. Also tries prefix-match so we can diagnose naming
// skew if the stored full name doesn't exactly match "Class TgGame.Name".
static UClass* FindClassByFullName(const char* target) {
	TArray<UObject*>* objs = UObject::GObjObjects();
	if (!objs || !objs->Data) return nullptr;
	static bool dumpedTgEffect = false;
	for (int i = 0; i < objs->Count; i++) {
		UObject* obj = objs->Data[i];
		if (!obj) continue;
		char* fullName = obj->GetFullName();
		if (!fullName) continue;
		if (strcmp(fullName, target) == 0) {
			return (UClass*)obj;
		}
		// Debug: dump every object whose name mentions TgEffectBuff or
		// TgEffect classes — tells us whether the class object exists under
		// a different name format, or whether it's missing entirely.
		// if (!dumpedTgEffect && strstr(fullName, "TgEffect") != nullptr) {
		// 	Logger::Log("effects", "[CLASS-DUMP] i=%d obj=%p fullName='%s'\n",
		// 		i, (void*)obj, fullName);
		// }
	}
	dumpedTgEffect = true;
	return nullptr;
}

// -------------------------------------------------------------------------
// asm_data_set_effect_groups.posture_type_value_id is a FK into
// asm_data_set_valid_values (group 61), NOT the TG_POSTURE byte enum.
// UC's TgEffectGroup.ApplyPosture passes m_nPosture straight to
// TgPawn.SetPosture which writes `r_ePosture = byte(ePosture)` — so the
// raw FK gets truncated. e.g. 814 (=None) becomes byte(814)=46, which is
// out of the 0..32 enum range → BlendByPostureAnimNode finds no matching
// child and the pawn freezes (turret idle/fire animations missing).
// Translate FK → enum byte at load time. Unmapped/zero values map to
// TG_POSTURE_NONE (31) which makes ApplyPosture early-return.
static int MapPostureValueIdToEnum(int valueId) {
	switch (valueId) {
		case 334: return 0;  // Default
		case 336: return 1;  // Hibernate
		case 337: return 2;  // Enraged
		case 338: return 3;  // Scared
		case 339: return 4;  // Wounded
		case 404: return 5;  // Stunned
		case 405: return 6;  // ShortCircuit
		case 406: return 7;  // Confuse
		case 407: return 8;  // Slow
		case 408: return 9;  // Knockback
		case 409: return 10; // Sleep
		case 410: return 11; // CriticalFailure
		case 411: return 12; // Immobilize
		case 412: return 13; // VisionBlur
		case 414: return 14; // Weakness
		case 415: return 15; // Blocking
		case 446: return 16; // Job1
		case 447: return 17; // Job2
		case 496: return 18; // Patrol
		case 669: return 20; // AtConsole (no value_id for Relax=19 in DB)
		case 720: return 21; // GenericFire1
		case 721: return 22; // GenericFire2
		case 722: return 23; // GenericFire3
		case 742: return 24; // BlockStun
		case 1055: return 25; // Rest
		case 1150: return 26; // Startled
		case 1151: return 27; // Low Profile Panic
		case 1152: return 28; // Running Panic
		case 1153: return 29; // ScaredAndWounded
		case 1189: return 30; // Searching
		case 814: return 31; // None
		case 0:
		default:  return 31; // unset / unknown → TG_POSTURE_NONE (no posture change)
	}
}

static UClass* GetEffectClassById(int classResId) {
	const char* name = nullptr;
	switch (classResId) {
		case 181: name = "Class TgGame.TgEffectDamage";     break;
		case 692: name = "Class TgGame.TgEffectHeal";       break;
		case  89: name = "Class TgGame.TgEffectVisibility"; break;
		// case 157: name = "Class TgGame.TgEffectBuff";       break;
		case 157: name = "Class TgGame.TgEffect";       break;
		case 244: name = "Class TgGame.TgEffectSensor";     break;
		case  80:
		default:  name = "Class TgGame.TgEffect";           break;
	}
	UClass* c = ClassPreloader::GetClass(name);
	// if (!c) c = FindClassByFullName(name);

	// TgEffectBuff is not registered in this build's GObjObjects (verified by
	// a full-scan miss while base TgEffect, TgEffectDamage, TgEffectHeal,
	// TgEffectSensor, TgEffectVisibility all exist). The class is referenced
	// only by its properties/functions — no UClass instance to construct
	// from. Since TgEffectBuff only adds `m_BuffSourceType` (one byte) on
	// top of UTgEffect and our apply path doesn't use it, fall back to the
	// base TgEffect class so the effect still gets created and its property-
	// modifier math runs.
	// if (!c && classResId == 157) {
	// 	c = ClassPreloader::GetClass("Class TgGame.TgEffect");
	// 	if (!c) c = FindClassByFullName("Class TgGame.TgEffect");
	// 	Logger::Log("effects",
	// 		"[BUILD] class_res_id=157 TgEffectBuff not found — falling back to TgEffect\n");
	// }
	return c;
}

// -------------------------------------------------------------------------
// Build a UTgEffectGroup (with its UTgEffect children) from the database
//
// Non-static: also consumed by ReapplyCharacterSkillTree for skill-tree
// allocations. Declared in BuildEffectGroup.hpp.
// -------------------------------------------------------------------------
UTgEffectGroup* BuildEffectGroup(int egId, int egType) {
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
				g->m_nPosture          = MapPostureValueIdToEnum(sqlite3_column_int(stmt, 14));
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
				int classResId = sqlite3_column_int(stmt, 0);
				UClass* effectClass = GetEffectClassById(classResId);
				if (!effectClass) {
					Logger::Log("effects",
						"[BUILD] egId=%d skipped effect: GetEffectClassById(%d) returned null\n",
						egId, classResId);
					continue;
				}

				UTgEffect* e = (UTgEffect*)ConstructEffect(effectClass, -1, 0, 0, 0, 0, 0, 0, nullptr);
				if (!e) {
					Logger::Log("effects",
						"[BUILD] egId=%d skipped effect: ConstructEffect failed (class_res_id=%d)\n",
						egId, classResId);
					continue;
				}

				float baseValue = (float)sqlite3_column_double(stmt, 2);
				int   calcCode  = sqlite3_column_int(stmt, 5);
				// Class-157 (TgEffectBuff) stores %-modifier amounts in
				// PERCENT (10.0 = 10%), while class-80 (TgEffect) stores them
				// in FRACTION (0.1 = 10%). Normalize to fraction at build
				// time so downstream code can treat base_value uniformly.
				// Only applies to calc codes 68 (+%) and 69 (-%); additive
				// codes 67/70 are absolute values regardless of class.
				if (classResId == 157 && (calcCode == 68 || calcCode == 69)) {
					baseValue /= 100.0f;
				}

				e->m_EffectGroup      = g;
				e->m_nPropertyId      = sqlite3_column_int   (stmt, 1);
				e->m_fBase            = baseValue;
				e->m_fMinimum         = (float)sqlite3_column_double(stmt, 3);
				e->m_fMaximum         = (float)sqlite3_column_double(stmt, 4);
				e->m_nCalcMethodCode  = calcCode;
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

				// Class-157 effects (TgEffectBuff) need to apply via Pawn.ApplyBuff
				// rather than TgEffect.ApplyToProperty — the latter writes to
				// `target->s_Properties[propId]->m_fRaw` which silently no-ops for
				// modifier props (113, 65, 352, …) the pawn doesn't carry there.
				// TgEffectBuff itself is stripped from this binary so we constructed
				// `e` as base TgEffect above; flag it in the side-set so
				// `UObject__ProcessEvent` recognises it at apply/remove time and
				// routes through `TgPawn__ApplyBuff::Call` instead. Mirrors
				// `unrealscript/TgGame/Classes/TgEffectBuff.uc:121-200`.
				if (classResId == 157) {
					BuffEffectRegistry::Mark(e);
				}

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
			"[BUILD] egId=%d egType=%d effects=%d lifetime=%.2f category=%d app=%d posture=%d bIsManaged=%d\n",
			egId, egType, g->m_Effects.Count, g->m_fLifeTime, g->m_nCategoryCode,
			g->m_nApplicationType, g->m_nPosture, (gflags & 0x01) ? 1 : 0);
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
			// m_bIsAOE bit 0x01 at +0x44 (UTgDeviceFire bitfield, SDK_HEADERS/TgGame_classes.h:7497).
			bool  fmIsAOE      = (*(unsigned char*)((char*)pThis + 0x44) & 0x01) != 0;

			// Map fire-mode attack signal to TgEffectGroup.AttackType enum
			// (TgEffectGroup.uc:40-48): None=0, Melee=1, Range=2, AOE=3.
			//
			// AOE takes precedence over melee/range — the only `valid_value_group=25`
			// entries are 83/85/87/170/177/...; none of them encode "AOE", so the
			// original game distinguishes splash damage via the m_bIsAOE bit on
			// the fire mode (UC TgDeviceFire.uc keys all of its AoE handling off
			// it: ApplyHit:339, splash distance scaling:448-465). Without this
			// override, grenade/bomb/aftershock/gammaburst splash damage gets
			// tagged as Range (2) and CalcAttackTypeProtection looks up prop 218
			// (Protection-Ranged) instead of prop 219 (Protection-AOE) — making
			// the AoE Shield (effect 5835: prop 219 +100, 10s/2000hp) silently
			// useless against the very damage it's meant to absorb.
			unsigned char eAttackType = 0;
			if (fmIsAOE)                                        eAttackType = 3; // TGAT_AOE
			else if (fmAttackType == 170)                       eAttackType = 1; // TGAT_Melee
			else if (fmAttackType == 85 || fmAttackType == 177) eAttackType = 2; // TGAT_Range

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
			// Surgical blacklist: Techro Buff Station (device 6144) egId=24419
			// is type 263 (DeviceFiring), effects=0 (visual-only), target=self,
			// target_fx_id=1275. Each refire pulse runs ApplyEffectType(self,
			// 263) which clones the group, adds it to s_AppliedEffectGroups,
			// triggers SetEffectRep → form attached to the station's
			// r_ManagedEffectList[1]. Each per-pulse zero+repopulate cycle
			// destroys and recreates the form on the client. FX 1275's intrinsic
			// asset duration is long enough that recreated FX instances overlap
			// into a "huge dome" visual on the station. The original game does
			// not render this visual on buff stations (per gameplay testing); we
			// suspect target_fx_id=1275 is either dead/asset-dummy in the
			// shipped client or there's a class-specific suppression we don't
			// reproduce. Same-pattern medical-station egid 6025 with FX 227 has
			// a short asset duration so cycle-recreation looks like discrete
			// pulses (intended). Mechanically identical effect groups whose
			// only differentiator is asset behavior — can't tell apart from
			// server data, so we hardcode the buff-station egid.
			//
			// Confirmed unique to device 6144 via:
			//   sqlite3 server.db "SELECT device_id FROM asm_data_set_device_mode_effect_groups WHERE effect_group_id = 24419;"
			// → 6144 (only). No other device uses 24419, so filtering here
			// doesn't bleed.
			//
			// Returning null+nIndex=-1 mimics "no more type-263 groups" for UC;
			// since 24419 is the buff station's only type-263 group, the
			// ApplyEffectType iteration cleanly bails on the next index.
			if (g->m_nEffectGroupId == 24419) {
				Logger::Log("effects",
					"[GET] fireMode=%s nType=%d skipping egId=24419 (Techro buff station visual-only — see code comment)\n",
					pThis->GetFullName(), nType);
				continue;
			}
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
