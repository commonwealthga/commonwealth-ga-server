#include "src/GameServer/TgGame/TgDeviceFire/GetEffectGroup/TgDeviceFire__GetEffectGroup.hpp"
#include "src/GameServer/TgGame/BuffEffectRegistry/BuffEffectRegistry.hpp"
#include "src/GameServer/TgGame/BuffEffectRegistry/DeployableOriginRegistry.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/Database/Database.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <unordered_set>

// ConstructObject<UTgEffectGroup> (validates IsChildOf UTgEffectGroup::StaticClass())
typedef void*(__cdecl* ConstructEffectGroupFn)(void*, int, int, int, unsigned, unsigned, int, int, int*);
static ConstructEffectGroupFn ConstructEffectGroup = (ConstructEffectGroupFn)0x109a90b0;

// -------------------------------------------------------------------------
// Visual-noise blacklist: type-263 (DeviceFiring) effect groups whose
// target_fx_id is the "dome ring" asset 1275. On every refire pulse
// ApplyEffectType clones the group, SetEffectRep recreates the form on the
// client, and FX 1275's intrinsic asset duration is long enough that the
// recreated instances overlap into a huge screen-obscuring dome. The
// original game does not render this visual on these deployables (per
// gameplay testing on Techro Buff Station and EMP Field Generator I) —
// suspect FX 1275 is a dead/asset-dummy in the shipped client or there's a
// class-specific suppression we don't reproduce. Same-pattern groups with
// other FX assets (e.g. medical station egid 6025 with FX 227) have short
// asset durations and cycle-recreate as discrete pulses (intended), so we
// narrow the filter to exactly the FX-1275 cohort.
//
// 14 known egIds across Buff Stations (incl. zAvA Mk1–4), Medical Hub
// I/II/III + zAvA Medical Mk4, EMP Field Generator I, and four unnamed
// zAvA power-hub variants. Confirmed each egId is unique to one device via:
//   sqlite3 server.db "SELECT effect_group_id, COUNT(DISTINCT device_id)
//     FROM asm_data_set_device_mode_effect_groups
//     WHERE effect_group_id IN (SELECT effect_group_id
//       FROM asm_data_set_effect_groups
//       WHERE target_fx_id=1275 AND effect_group_type_value_id=263)
//     GROUP BY effect_group_id;"
// → all 1, so filtering doesn't bleed across devices.
//
// Returning null+nIndex=-1 from the consumer mimics "no more type-263
// groups" for UC; since each device's only type-263 group is the suppressed
// one, the ApplyEffectType iteration cleanly bails on the next index.
// -------------------------------------------------------------------------
static bool IsVisualNoiseEffectGroup(int egId) {
	static std::unordered_set<int> badEgIds;
	static bool initialized = false;
	if (!initialized) {
		initialized = true;
		sqlite3* db = Database::GetConnection();
		if (db) {
			sqlite3_stmt* stmt = nullptr;
			if (sqlite3_prepare_v2(db,
				"SELECT effect_group_id FROM asm_data_set_effect_groups "
				"WHERE target_fx_id = 1275 AND effect_group_type_value_id = 263",
				-1, &stmt, nullptr) == SQLITE_OK) {
				while (sqlite3_step(stmt) == SQLITE_ROW) {
					badEgIds.insert(sqlite3_column_int(stmt, 0));
				}
				sqlite3_finalize(stmt);
			}
			Logger::Log("effects",
				"[INIT] IsVisualNoiseEffectGroup loaded %d egIds (target_fx_id=1275, type=263)\n",
				(int)badEgIds.size());
		}
	}
	return badEgIds.count(egId) > 0;
}

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

	// Captured from the effect_groups SELECT below; the m_bHasVisual decision
	// is deferred to AFTER the stealth-lifetime promotion further down (cat 621
	// gets a synthetic 1s lifetime that we want the visual gate to honor).
	int iconId          = 0;
	int targetFxId      = 0;
	int fxDisplayGroup  = 0;

	// --- Fill UTgEffectGroup fields ---
	{
		sqlite3_stmt* stmt = nullptr;
		int rc = sqlite3_prepare_v2(db,
			"SELECT lifetime_sec, apply_interval_sec, application_value_id, "
			"application_value, application_chance, category_value_id, "
			"required_category_value_id, required_skill_id, situational_type_value_id, "
			"situational_value, stack_count_max, buff_value, contagion_flag, "
			"device_specific_flag, posture_type_value_id, health, icon_id, "
			"target_fx_id, fx_display_group_res_id "
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
				// m_nNumMaxStacks: engine (FUN_10a6f300) only writes the DB value
				// when > 0; otherwise leaves CDO default (20). 14627 effect groups
				// have stack_count_max=0 in DB — unconditional write was silently
				// capping their stacking at 1 (UC GetStackingEffectGroup:479
				// removes oldest when nCurrentStacks >= m_nNumMaxStacks; with 0
				// that fires on the second stack). Mirror the engine.
				{
					int dbStacks = sqlite3_column_int(stmt, 10);
					if (dbStacks > 0) g->m_nNumMaxStacks = dbStacks;
					// else: leave CDO default of 20 (TgEffectGroup.uc:913)
				}
				g->m_fBuffValue        = (float)sqlite3_column_double(stmt, 11);
				g->m_nPosture          = MapPostureValueIdToEnum(sqlite3_column_int(stmt, 14));
				g->m_nHealth           = sqlite3_column_int   (stmt, 15);
				// Engine FUN_10a6f300 sets m_nHealthMitigated = m_nHealth at
				// build time (lines 0x120/0x124 both written from same DB int).
				// UC ProcessEffect:243 overwrites with GetEffectHealth(Impact)
				// for groups where m_nHealth > 0, so the build-time value is
				// only read between build and first apply, but mirroring the
				// engine prevents any read of an uninitialized 0.
				g->m_nHealthMitigated  = g->m_nHealth;

				unsigned int& flags = *(unsigned int*)((char*)g + 0x74);
				if (sqlite3_column_int(stmt, 12)) flags |= 0x10; else flags &= ~0x10; // m_bContagious
				if (sqlite3_column_int(stmt, 13)) flags |= 0x20; else flags &= ~0x20; // m_bDeviceSpecificFlag

				// Stash visual columns for the m_bHasVisual gate below (post
				// stealth-lifetime promotion). Don't set bit 0x80 here.
				iconId         = sqlite3_column_int(stmt, 16);
				targetFxId     = sqlite3_column_int(stmt, 17);
				fxDisplayGroup = sqlite3_column_int(stmt, 18);
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

				g->m_Effects.Add(e);
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
	// if (g->m_nCategoryCode == 621 && g->m_fLifeTime == 0.0f) {
	// 	g->m_fLifeTime = 1.0f;
	// 	Logger::Log("effects",
	// 		"[BUILD] egId=%d promoted stealth lifetime 0 -> 1s (HUD icon)\n", egId);
	// }

	// m_bHasVisual (bit 0x80 @ +0x74) — necessary-but-not-sufficient gate for
	// `SetEffectRep` / `RefreshEffectRep` in `TgEffectManager.ProcessEffect`
	// (TgEffectManager.uc:280). `SetEffectRep` is the **client-notification
	// primitive for ANY effect visual** — it writes either:
	//   • `r_ManagedEffectList` (HUD-icon strip slot, Branch B in the native),
	//   • OR `r_EventQueue` (transient FX/sound ring buffer, Branch A — which
	//     our hook reaches manually for fTime==0).
	// The client decodes the egId and renders whatever the DB metadata
	// dictates: HUD icon (`icon_id`), target particle/audio asset
	// (`target_fx_id`), or grouped FX (`fx_display_group_res_id`).
	//
	// So "has visual" ≠ "has HUD icon". An earlier icon-only gate dropped the
	// transient FX/sound for ~1968 effect groups (Medical Station heal pulse
	// target_fx_id=432, Crescent jetpack thrust target_fx_id=1265, knockback
	// impact target_fx_id, etc.). The right gate is "any of the three visual
	// resource ids is non-zero": 3935 / 14649 effect groups, leaving 10714
	// (mostly rolled mods + cat-302 mechanical-only modifiers) blocked.
	//
	// The lingering-icon case (Boss Shrike Energy Cannon AoE on player: cat
	// 302, lifetime 0, icon_id 20, target_fx_id 122) is handled at apply time
	// in the `SetEffectRep` hook — we suppress the managed-list slot when
	// `fTime==0 && m_Target != m_Instigator`, while still keeping the queue
	// push so impact FX/sound plays.
	{
		unsigned int& gflags = *(unsigned int*)((char*)g + 0x74);
		// Bit 0x80 = m_bHasVisual — UC's TgEffectManager.ProcessEffect only
		// calls `SetEffectRep` for groups where this is set. SetEffectRep is
		// where our class-157 (TgEffectBuff) routing fires `ApplyBuffEffectFromHook`
		// on apply, so a group with zero visual assets AND a class-157 effect
		// (e.g. Focused Repair Arm's +40% damage buff egId 9136 — icon_id=0,
		// target_fx_id=0, fx_display_group=0, but prop 65 +40% via class-157)
		// would otherwise silently fail to register the buff on the target.
		// UC's base `TgEffect.ApplyEffect` still runs and sets `m_fCurrent`,
		// but its `ApplyToProperty → SetProperty` write silently no-ops because
		// the target pawn doesn't carry modifier prop 65 in `s_Properties`.
		// Force-set the visual bit when ANY effect in the group is flagged in
		// the buff registry — this guarantees SetEffectRep fires and our
		// buff-routing path lands the entry in `m_EffectBuffInfo`. Side effect
		// is benign: with icon_id=0 + target_fx_id=0 + fx_display_group=0 the
		// client has nothing to render, so SetEffectRep's queue/slot writes
		// produce no visible artifact.
		bool hasBuffEffect = false;
		for (int i = 0; i < g->m_Effects.Count; ++i) {
			UTgEffect* e = g->m_Effects.Data[i];
			if (e && BuffEffectRegistry::IsBuff(e)) { hasBuffEffect = true; break; }
		}
		if (iconId > 0 || targetFxId > 0 || fxDisplayGroup > 0 || hasBuffEffect) {
			gflags |= 0x80;
		} else {
			gflags &= ~0x80u;
		}
	}

	// Set m_bIsManaged (bit 0x01 at offset 0x74). Criterion is a direct port
	// of the original engine's native `InstantiateDeviceFire` @ 0x10a26780,
	// which after building each device-bound effect group does:
	//
	//   if (eg->m_fLifeTime <= epsilon)
	//       eg->flags |= 0x01;     // m_bIsManaged = true
	//   else
	//       eg->flags &= ~0x01;    // m_bIsManaged = false
	//
	// Semantic: m_bIsManaged means "this group needs explicit apply/remove
	// tracking because no LifeDone timer will reap it." Three UC consumers:
	//   • `TgEffectManager.ProcessEffect:231` apply gate
	//       (lifetime > 0 || m_bIsManaged) — lifetime=0 hold-to-sustain groups
	//       need m_bIsManaged to enter the apply chain at all.
	//   • `TgEffectManager.ProcessEffect:262` add-clause
	//       (bEventApplied || bTimerStarted || m_bIsManaged) — final fallback
	//       to add into s_AppliedEffectGroups.
	//   • `TgDeviceFire.RemoveEffectType:1156/1195` non-forced explicit-remove
	//       filter — only managed groups respond to RemoveAimEffects /
	//       RemoveRestEffects. Both call sites pass nEffectGroupType ∈ {266,283}
	//       which the DB shows are 100% lifetime=0.
	//
	// History: we previously force-set m_bIsManaged=1 on every built group.
	// That broke the engine's intended semantic by force-promoting lifetime>0
	// effect groups whose Apply was gated out by category protection into
	// `s_AppliedEffectGroups` as "phantom clones" — and those phantoms then
	// leaked side effects (notably SendCombatMessage for cat=921 EMP Burn on
	// biological targets, producing the `*EMP Burn*` popup that should have
	// been suppressed by HUMAN BASE ATTRIBUTES' prop 328 protection). Matching
	// the binary's criterion restores the original behavior.
	{
		unsigned int& gflags = *(unsigned int*)((char*)g + 0x74);
		if (g->m_fLifeTime <= 0.0f) {
			gflags |= 0x01; // m_bIsManaged = true
		} else {
			gflags &= ~0x01u; // m_bIsManaged = false (lifetime expires via LifeDone)
		}
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

			for (int** pp = begin; pp != end; pp++) {
				int egId   = (*pp)[0];
				int egType = (*pp)[1];
				UTgEffectGroup* g = BuildEffectGroup(egId, egType);
				if (g) {
					g->m_nDamageType = fmDamageType;
					g->m_eAttackType = eAttackType;
					pThis->s_EffectGroupList.Add(g);
					// Associate this template with the fire-mode that owns
					// it. CloneEffectGroup uses this to recover the firing
					// entity (deployable) when UC's `InitInstance` leaves
					// `m_nSourceDeviceInstId == 0` — happens for
					// deployable-fired effects whose Impact has no
					// DeviceModeReference. Set-once per template (lazy
					// init), so no race between simultaneous deployers.
					DeployableOriginRegistry::NoteTemplateOwner(g, pThis);
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
			// Skip groups in the FX-1275 visual-noise cohort. See big
			// comment block on IsVisualNoiseEffectGroup near top of file.
			if (IsVisualNoiseEffectGroup(g->m_nEffectGroupId)) {
				if (Logger::IsChannelEnabled("effects")) {
					Logger::Log("effects",
						"[GET] fireMode=%s nType=%d skipping egId=%d (FX-1275 visual-noise cohort)\n",
						pThis->GetFullName(), nType, g->m_nEffectGroupId);
				}
				continue;
			}

			if (nIndex) *nIndex = i;

			if (Logger::IsChannelEnabled("effects")) {
				Logger::Log("effects",
					"[GET] fireMode=%s nType=%d -> egId=%d (startIdx=%d resultIdx=%d)\n",
					pThis->GetFullName(), nType, g->m_nEffectGroupId, startIdx, i);
			}
			return g;
		}
	}

	if (nIndex) *nIndex = -1;

	if (Logger::IsChannelEnabled("effects")) {
		Logger::Log("effects",
			"[GET] fireMode=%s nType=%d -> NONE (searched from %d in list of %d)\n",
			pThis->GetFullName(), nType, startIdx, list.Count);
	}

	return nullptr;
}

