#include "src/GameServer/TgGame/TgEffectManager/RemoveAllEffects/TgEffectManager__RemoveAllEffects.hpp"
#include "src/GameServer/TgGame/TgEffectGroup/RemoveEffects/TgEffectGroup__RemoveEffects.hpp"
#include "src/GameServer/TgGame/TgEffectManager/ProcessReactiveSkillBasedEffectGroup/TgEffectManager__ProcessReactiveSkillBasedEffectGroup.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include <set>

// TgEffectManager::RemoveAllEffects — reimplements the stripped stub @ 0x10a6ef00.
// Walks every applied group, reverses property modifiers, tears down timers,
// frees the rep slot (via the intact ClearEffectRep), and pops from the array.
// Scheduled on death/respawn (TgPawn.uc:4456-4462) so a respawned pawn doesn't
// inherit stuns / debuffs / speed modifiers it was carrying at death.
//
// Clean-room rebuild: slot teardown delegated to the intact refcount-aware
// ClearEffectRep (0x10a6f030). Stateless routing — nothing to forget. The
// per-effect aoi gate and station-aura group skip are preserved — they are
// the correctness core.

typedef void(__fastcall* ClearEffectRepFn)(ATgEffectManager*, void*, int, int);
static const ClearEffectRepFn ClearEffectRepNative = (ClearEffectRepFn)0x10a6f030;

void __fastcall TgEffectManager__RemoveAllEffects::Call(ATgEffectManager* pThis, void* /*edx*/, void* ExcludeCategoryCodes) {
	LogCallBegin();
	if (pThis == nullptr) { LogCallEnd(); return; }

	// Optional exclude-by-category list (TArray<int>: Data, Count, Max). UC's
	// RemoveEffectsTimer passes {303, 719} so DeathFade cosmetics survive.
	struct ArrInt { int* Data; int Count; int Max; };
	const ArrInt* excl = reinterpret_cast<const ArrInt*>(ExcludeCategoryCodes);
	auto isExcluded = [&](int catCode) -> bool {
		if (!excl || excl->Count <= 0 || excl->Data == nullptr) return false;
		for (int i = 0; i < excl->Count; ++i) if (excl->Data[i] == catCode) return true;
		return false;
	};

	Logger::Log("effects", "[REMOVE-ALL-EFFECTS] mgr=%p applied=%d excludes=%d\n",
		(void*)pThis, pThis->s_AppliedEffectGroups.Count, excl ? excl->Count : 0);

	// Track each category whose group(s) we tear down so we can fire the
	// reactive-skill OFF dispatch once per category at the end. Without this,
	// reactive skills like Aegis Armament (skill 913 / EG 26696, m_nReqCategory=770
	// = PERSONAL_SHIELD, +25 Protection-Physical while a shield is active) keep
	// their stat-mod committed past death because nothing reverses the m_fRaw
	// delta applied at shield-on. Symptom: voluntary death with a Range/AoE
	// Shield active → respawn → permanent +25% physical resist until the player
	// disconnects. RemoveEffectGroup handles this correctly in its own path
	// (last-of-category check + ProcessReactiveSkillBasedEffectGroup(cat, true));
	// we mirror that here for the bulk death-cleanup.
	//
	// Also tracks whether any health>0 (shield) group was torn down so we can
	// zero the replicated shield-bar fields (see end-of-function block).
	std::set<int> removedCategories;
	bool removedShield = false;

	// Reverse iteration so swap-removes don't skip entries.
	for (int i = pThis->s_AppliedEffectGroups.Count - 1; i >= 0; --i) {
		UTgEffectGroup* applied = pThis->s_AppliedEffectGroups.Data[i];
		// See "small-int" comment in TgEffectGroup__RemoveEffects.cpp — bare
		// null check misses corruption like 0x35d (=861 TG_PHYSICALITY_MECHANICAL)
		// that has been showing up in m_Effects/s_AppliedEffectGroups slots.
		if (applied == nullptr || reinterpret_cast<uintptr_t>(applied) < 0x10000u) {
			if (applied) {
				Logger::Log("effects",
					"[REMOVE-ALL-EFFECTS] mgr=%p s_AppliedEffectGroups[%d]=%p — "
					"small-int value, skipping (likely corrupted entry)\n",
					(void*)pThis, i, (void*)applied);
			}
			continue;
		}
		if (isExcluded(applied->m_nCategoryCode)) continue;

		AActor* target = applied->m_Target ? applied->m_Target : pThis->r_Owner;

		// Station-aura group skip: pure aoi=0 but lifetime=0 + interval=0 +
		// type=264 means the source deployable's per-tick re-fire owns the
		// lifecycle — the resource gift sticks; don't reverse it.
		const bool isStationAuraPattern =
			(applied->m_nType == 264 && applied->m_fLifeTime == 0.0f && applied->m_fApplyInterval == 0.0f);

		if (target && !isStationAuraPattern) {
			// PER-EFFECT reversal (not per-group): a mixed-aoi group (e.g. REST
			// device EG 2654 — one aoi=1 HoT tick + seven aoi=0 penalties; or a
			// Poison DoT — aoi=1 damage + aoi=0 slow) must reverse the aoi=0
			// modifiers while leaving the aoi=1 tick-gifts committed. A coarse
			// group gate left the aoi=0 penalties stuck on s_Properties forever.
			for (int e = 0; e < applied->m_Effects.Count; ++e) {
				UTgEffect* eff = applied->m_Effects.Data[e];
				// Filter out null AND small-int corruption (see RemoveEffects comment).
				if (!eff || reinterpret_cast<uintptr_t>(eff) < 0x10000u) {
					if (eff) {
						Logger::Log("effects",
							"[REMOVE-ALL-EFFECTS] mgr=%p applied egId=%d effect[%d]=%p — "
							"small-int value, skipping\n",
							(void*)pThis, applied->m_nEffectGroupId, e, (void*)eff);
					}
					continue;
				}
				const unsigned int eflags = *(unsigned int*)((char*)eff + 0x48);
				if (eflags & 0x01) continue;        // aoi=1: tick-gift, skip reversal
				// Sensor/Visibility exemption (see RemoveEffects.cpp): without it,
				// dying mid Visual Scanner / Vulture Vision left r_ScannerSettings
				// populated forever on the revived pawn (permanent reveal).
				if (!EffectPhantomGuardExempt(eff) && eff->m_fCurrent == 0.0f) continue;  // phantom clone: Apply never ran
				DispatchEffectRemove(eff, target, 0);  // own-class Remove (buffs need the override)
			}
		}

		// Cancel timers armed on this group (AActor+0xA0 Timers.Data / +0xA4
		// Count; 0x1C-byte entries, +0x14 bound object, +0x0C rate).
		unsigned char* actor = (unsigned char*)pThis;
		unsigned int  timerCount = *(unsigned int*) (actor + 0xA4);
		unsigned char* timerData = *(unsigned char**)(actor + 0xA0);
		for (unsigned int t = 0; t < timerCount; ++t) {
			unsigned char* td = timerData + t * 0x1C;
			if (*(void**)(td + 0x14) == (void*)applied) *(unsigned int*)(td + 0x0C) = 0;
		}

		// Invulnerability refcount (category 862).
		if (applied->m_nCategoryCode == 862 && pThis->r_nInvulnerableCount > 0) {
			pThis->r_nInvulnerableCount--;
		}

		// Release the rep slot via the intact refcount-aware native.
		ClearEffectRepNative(pThis, nullptr, applied->m_nEffectGroupId, applied->s_ManagedEffectListIndex);

		// Record category + shield-ness BEFORE the swap-remove (post-swap the
		// pointer at this slot belongs to a different group).
		if (applied->m_nCategoryCode > 0) removedCategories.insert(applied->m_nCategoryCode);
		if (applied->m_nHealth > 0) removedShield = true;

		// Swap-remove from s_AppliedEffectGroups.
		pThis->s_AppliedEffectGroups.Data[i] =
			pThis->s_AppliedEffectGroups.Data[pThis->s_AppliedEffectGroups.Count - 1];
		pThis->s_AppliedEffectGroups.Count--;
	}

	// Fire reactive-OFF dispatch for every category that's now empty. Mirrors
	// RemoveEffectGroup's last-of-category check, batched. The recompute after
	// the swap-remove loop above is necessary because mid-loop the array is in
	// flux; an excluded group (303 DeathFade, 719) of the same category would
	// otherwise be missed as a survivor.
	for (int cat : removedCategories) {
		bool stillHasCategory = false;
		for (int i = 0; i < pThis->s_AppliedEffectGroups.Count; i++) {
			UTgEffectGroup* g = pThis->s_AppliedEffectGroups.Data[i];
			if (g && reinterpret_cast<uintptr_t>(g) >= 0x10000u &&
			    g->m_nCategoryCode == cat) { stillHasCategory = true; break; }
		}
		if (!stillHasCategory) {
			TgEffectManager__ProcessReactiveSkillBasedEffectGroup::Call(
				pThis, nullptr, cat, 1u);
		}
	}

	// Zero the published shield-bar so enemies stop seeing the healthbar on a
	// dead/respawning pawn. UC's apply path (TgEffectManager.uc:241-245)
	// unconditionally overwrites these on the next shield, so this cleanup is
	// purely the "last shield removed" cleanup the UC code never wrote.
	// Skip for non-pawn owners (TgDeployable has no such fields).
	if (removedShield && pThis->r_Owner != nullptr &&
	    reinterpret_cast<uintptr_t>(pThis->r_Owner) >= 0x10000u) {
		const std::string& cn = ObjectClassCache::GetClassName(pThis->r_Owner->Class);
		if (cn.compare(0, 19, "Class TgGame.TgPawn") == 0) {
			ATgPawn* pawn = (ATgPawn*)pThis->r_Owner;
			pawn->r_nShieldHealthMax = 0;
			pawn->r_nShieldHealthRemaining = 0;
			pawn->bNetDirty = 1;
			pawn->bForceNetUpdate = 1;
		}
	}

	pThis->bNetDirty = 1;
	LogCallEnd();
}
