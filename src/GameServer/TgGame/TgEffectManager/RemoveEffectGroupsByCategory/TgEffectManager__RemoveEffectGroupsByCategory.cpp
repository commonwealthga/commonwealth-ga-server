#include "src/GameServer/TgGame/TgEffectManager/RemoveEffectGroupsByCategory/TgEffectManager__RemoveEffectGroupsByCategory.hpp"
#include "src/GameServer/TgGame/TgEffectGroup/RemoveEffects/TgEffectGroup__RemoveEffects.hpp"
#include "src/GameServer/TgGame/_effect_core/HitSituationalMitigation.hpp"
#include "src/GameServer/TgGame/_effect_core/CleanseTracking.hpp"
#include "src/GameServer/TgGame/TgEffectManager/ProcessReactiveSkillBasedEffectGroup/TgEffectManager__ProcessReactiveSkillBasedEffectGroup.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/Utils/Logger/Logger.hpp"

// TgEffectManager::RemoveEffectGroupsByCategory — reimplements the stripped stub
// @ 0x10a6ef20. UC: `native function bool RemoveEffectGroupsByCategory(int
// nCategoryCode, int nQuantity);` Called from TgEffect::ApplyEffect when
// m_nPropertyId == 140 (the cure/cleanse type): purge up to nQuantity groups of
// a category (e.g. poison) so a healing grenade's cure stops the DoT.
//
// Clean-room rebuild: slot teardown delegated to the intact refcount-aware
// ClearEffectRep (0x10a6f030) instead of refcount-blind manual zeroing.

typedef void(__fastcall* ClearEffectRepFn)(ATgEffectManager*, void*, int, int);
static const ClearEffectRepFn ClearEffectRepNative = (ClearEffectRepFn)0x10a6f030;

bool __fastcall TgEffectManager__RemoveEffectGroupsByCategory::Call(ATgEffectManager* Manager, void* /*edx*/, int nCategoryCode, int nQuantity) {
	// Closing bracket for the type-505 self-shot guard. TgEffectDamage.uc:206
	// calls this with exactly (431, 99) on the victim's manager, after
	// ProtectionModifier and the health cap and before TakeDamage — the one
	// unconditional beat in the damage path where mitigation is finished but
	// the hit has not landed yet. Runs before the arg checks so a window
	// always closes. See HitSituationalMitigation.hpp.
	if (nCategoryCode == 431 && nQuantity == 99) {
		HitSituationalMitigation::EndImpactMitigation(Manager);
	}

	if (!Manager || nQuantity <= 0) return false;

	int removed = 0;
	// Any health>0 (shield) group torn down here publishes a stale yellow
	// r_nShieldHealthMax/Remaining bar unless we zero it (see end of function).
	bool removedShield = false;

	// Reverse iteration so swap-deletes don't disturb earlier indices.
	for (int i = Manager->s_AppliedEffectGroups.Count - 1; i >= 0 && removed < nQuantity; i--) {
		UTgEffectGroup* group = Manager->s_AppliedEffectGroups.Data[i];
		// Null OR small-int corruption (see TgEffectGroup__RemoveEffects.cpp).
		if (!group || reinterpret_cast<uintptr_t>(group) < 0x10000u) {
			if (group) {
				Logger::Log("effects",
					"[REMOVE-BY-CAT] mgr=%p s_AppliedEffectGroups[%d]=%p — "
					"small-int value, skipping\n",
					(void*)Manager, i, (void*)group);
			}
			continue;
		}
		if (group->m_nCategoryCode != nCategoryCode) continue;

		// 0. Reverse modifiers the group installed (else they stay clamped forever).
		AActor* target = group->m_Target ? group->m_Target : Manager->r_Owner;
		if (target) {
			TgEffectGroup__RemoveEffects::Call(group, nullptr, target, 0);
		}

		// 1. Cancel timers armed on this group (else the DoT tick keeps firing).
		//    AActor+0xA0 Timers.Data / +0xA4 Count; 0x1C-byte entries, +0x14
		//    bound object, +0x0C flags (zero to cancel).
		unsigned char* actor = (unsigned char*)Manager;
		unsigned int timerCount = *(unsigned int*)(actor + 0xA4);
		unsigned char* timerData = *(unsigned char**)(actor + 0xA0);
		for (unsigned int t = 0; t < timerCount; t++) {
			unsigned char* td = timerData + t * 0x1C;
			if (*(void**)(td + 0x14) == (void*)group) *(unsigned int*)(td + 0x0C) = 0;
		}

		// 2. Invulnerability refcount (category 862).
		if (nCategoryCode == 862 && Manager->r_nInvulnerableCount > 0) {
			Manager->r_nInvulnerableCount--;
		}

		// 3. Release the rep slot via the intact refcount-aware native.
		ClearEffectRepNative(Manager, nullptr, group->m_nEffectGroupId, group->s_ManagedEffectListIndex);

		// Record shield-ness BEFORE the swap-remove (post-swap the pointer at
		// this slot belongs to a different group).
		if (group->m_nHealth > 0) removedShield = true;

		// 4. Swap-remove from s_AppliedEffectGroups.
		const int last = Manager->s_AppliedEffectGroups.Count - 1;
		Manager->s_AppliedEffectGroups.Data[i] = Manager->s_AppliedEffectGroups.Data[last];
		Manager->s_AppliedEffectGroups.Count--;

		removed++;
	}

	if (removed > 0) Manager->bNetDirty = 1;

	// 5. Reactive-skill OFF dispatch. This is the dispel/cleanse teardown path
	//    (prop 140), and 16 shipped effect groups cleanse category 770
	//    (Personal Shield): Neutralize Wave I-III (devices 3642/4655/4656/4657),
	//    Neutralize Blast I-IV, Neutralize Grenade I-IV, AOE_HEX_EMPField.
	//    Without the dispatch, Aegis Armament (skill 913 / EG 26696,
	//    m_nReqCategory=770, +25 Protection-Physical) stays committed to
	//    s_Properties[155].m_fRaw after its host shield is stripped — and it
	//    can never self-correct, because RemoveEffectGroup only fires OFF for
	//    the LAST group of a category (none left) and RemoveAllEffects on death
	//    only fires OFF for categories it actually tore down (770 already gone).
	//    Symptom: permanent +25% physical resistance with no shield up.
	//    Mirrors RemoveEffectGroup step 5 / RemoveAllEffects' batched pass.
	if (removed > 0 && nCategoryCode > 0) {
		bool stillHasCategory = false;
		for (int i = 0; i < Manager->s_AppliedEffectGroups.Count; i++) {
			UTgEffectGroup* g = Manager->s_AppliedEffectGroups.Data[i];
			if (g && reinterpret_cast<uintptr_t>(g) >= 0x10000u &&
			    g->m_nCategoryCode == nCategoryCode) { stillHasCategory = true; break; }
		}
		if (!stillHasCategory) {
			TgEffectManager__ProcessReactiveSkillBasedEffectGroup::Call(
				Manager, nullptr, nCategoryCode, 1u);
		}
	}

	// 6. Shield-bar clear — same cleanup RemoveEffectGroup and RemoveAllEffects
	//    already do. A neutralized shield otherwise leaves the yellow
	//    r_nShieldHealthMax/Remaining bar on the pawn until the next shield
	//    overwrites it (TgEffectManager.uc:241-245).
	if (removedShield && Manager->r_Owner != nullptr &&
	    reinterpret_cast<uintptr_t>(Manager->r_Owner) >= 0x10000u) {
		const std::string& cn = ObjectClassCache::GetClassName(Manager->r_Owner->Class);
		if (cn.compare(0, 19, "Class TgGame.TgPawn") == 0) {
			ATgPawn* pawn = (ATgPawn*)Manager->r_Owner;
			pawn->r_nShieldHealthMax = 0;
			pawn->r_nShieldHealthRemaining = 0;
			pawn->bNetDirty = 1;
			pawn->bForceNetUpdate = 1;
		}
	}

	// Cleanse stats: attribute the strip count to the player + device whose
	// property-140 effect triggered this call. Internal callers (the 431/99
	// mitigation bracket, invuln refcounting) are filtered inside.
	CleanseTracking::OnRemoved(Manager, nCategoryCode, nQuantity, removed);

	return removed > 0;
}
