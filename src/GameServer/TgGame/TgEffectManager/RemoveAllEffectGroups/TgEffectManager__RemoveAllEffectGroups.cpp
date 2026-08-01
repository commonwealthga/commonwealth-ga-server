#include "src/GameServer/TgGame/TgEffectManager/RemoveAllEffectGroups/TgEffectManager__RemoveAllEffectGroups.hpp"
#include "src/GameServer/TgGame/TgEffectGroup/RemoveEffects/TgEffectGroup__RemoveEffects.hpp"
#include "src/Utils/Logger/Logger.hpp"

// TgEffectManager::RemoveAllEffectGroups — reimplements the stripped stub @
// 0x10a6ef30. Removes all applied groups sharing the displacement bucket of the
// given group. Called from GetNewEffectGroupByApp before applying a replacement
// (Newest/SameInstigator/Strongest), so any modifiers those groups installed
// MUST be reversed here or the subsequent ApplyEffects compounds m_fRaw.
//
// Bucket scope mirrors the cat-302 ("Local") special case used by every
// displacement path (TgEffectManager.uc GetStackingEffectGroup:464 /
// GetRefreshedEffectGroup:504): cat==302 → match by m_nEffectGroupId, else by
// m_nCategoryCode. Without it, a cat-302 displacement wipes every other cat-302
// effect — including the jetpack per-pulse +AirSpeed/+FlightAccel (egs
// 10450/52/54/56); dropping FlightAccel triggers client SetPhysics(2) and the
// player falls mid-flight.
//
// Clean-room rebuild: slot teardown delegated to the intact refcount-aware
// ClearEffectRep (0x10a6f030). The EffectDisplacementMarker is GONE — SetEffectRep
// is the intact native now and handles re-emission/same-frame realloc via its own
// queue/managed reconciliation (canonical Q4), so there is nothing to mark.

typedef void(__fastcall* ClearEffectRepFn)(ATgEffectManager*, void*, int, int);
static const ClearEffectRepFn ClearEffectRepNative = (ClearEffectRepFn)0x10a6f030;

void __fastcall TgEffectManager__RemoveAllEffectGroups::Call(ATgEffectManager* pThis, void* /*edx*/, UTgEffectGroup* EffectGroup) {
	LogCallBegin();
	if (EffectGroup == nullptr) { LogCallEnd(); return; }

	const int nCategoryCode  = EffectGroup->m_nCategoryCode;
	const int nEffectGroupId = EffectGroup->m_nEffectGroupId;
	const bool isLocal = (nCategoryCode == 302);

	for (int i = pThis->s_AppliedEffectGroups.Count - 1; i >= 0; i--) {
		UTgEffectGroup* applied = pThis->s_AppliedEffectGroups.Data[i];
		// Null OR small-int corruption (see TgEffectGroup__RemoveEffects.cpp).
		if (applied == nullptr || reinterpret_cast<uintptr_t>(applied) < 0x10000u) {
			if (applied) {
				Logger::Log("effects",
					"[REMOVE-ALL-GROUPS] mgr=%p s_AppliedEffectGroups[%d]=%p — "
					"small-int value, skipping\n",
					(void*)pThis, i, (void*)applied);
			}
			continue;
		}

		const bool matches = isLocal
			? (applied->m_nEffectGroupId == nEffectGroupId)
			: (applied->m_nCategoryCode == nCategoryCode);
		if (!matches) continue;

		// 0. Reverse property modifiers — PER-EFFECT, mirroring RemoveAllEffects.
		//    A mixed-aoi group (Corrupted Tribesman backstab EG 27742: aoi=1
		//    poison tick + aoi=0 -25% Effect Heal Modifier; REST EG 2654; any
		//    cat-303 poison) must reverse its aoi=0 modifiers on displacement.
		//    The old coarse "skip whole group if any aoi=1" gate leaked the
		//    aoi=0 sibling into m_EffectBuffInfo forever on every Strongest-Wins
		//    re-application — stacking heal reduction that survived death.
		//    aoi=1 tick-gifts stay committed (reversing one would undo the
		//    power-station resource gift — the "+10/sec set to 10" bug).
		//    Station aura (type=264 + lifetime=0 + interval=0, pure aoi=0):
		//    group-level skip stays — the source deployable's re-fire owns it.
		const bool isStationAuraPattern =
			(applied->m_nType == 264
			&& applied->m_fLifeTime == 0.0f
			&& applied->m_fApplyInterval == 0.0f);

		AActor* target = applied->m_Target ? applied->m_Target : pThis->r_Owner;
		if (target && !isStationAuraPattern) {
			for (int e = 0; e < applied->m_Effects.Count; ++e) {
				UTgEffect* eff = applied->m_Effects.Data[e];
				// Null OR small-int corruption (see TgEffectGroup__RemoveEffects.cpp).
				if (!eff || reinterpret_cast<uintptr_t>(eff) < 0x10000u) {
					if (eff) {
						Logger::Log("effects",
							"[REMOVE-ALL-GROUPS] mgr=%p applied egId=%d effect[%d]=%p — "
							"small-int value, skipping\n",
							(void*)pThis, applied->m_nEffectGroupId, e, (void*)eff);
					}
					continue;
				}
				const unsigned int eflags = *(unsigned int*)((char*)eff + 0x48);
				if (eflags & 0x01) continue;        // aoi=1: tick-gift, skip reversal
				if (!EffectPhantomGuardExempt(eff) && eff->m_fCurrent == 0.0f) continue;  // phantom clone: Apply never ran
				DispatchEffectRemove(eff, target, 0);  // own-class Remove (buffs need the override)
			}
		}

		// 1. Cancel timers armed on this group (else DoT ticks keep firing).
		unsigned char* actor = (unsigned char*)pThis;
		unsigned int timerCount = *(unsigned int*)(actor + 0xA4);
		unsigned char* timerData = *(unsigned char**)(actor + 0xA0);
		for (unsigned int t = 0; t < timerCount; t++) {
			unsigned char* td = timerData + t * 0x1C;
			if (*(void**)(td + 0x14) == (void*)applied) *(unsigned int*)(td + 0x0C) = 0;
		}

		// 2. Invulnerability refcount (category 862).
		if (nCategoryCode == 862 && pThis->r_nInvulnerableCount > 0) {
			pThis->r_nInvulnerableCount--;
		}

		// 3. Release the rep slot via the intact refcount-aware native.
		ClearEffectRepNative(pThis, nullptr, applied->m_nEffectGroupId, applied->s_ManagedEffectListIndex);

		// 4. Swap-remove from s_AppliedEffectGroups.
		pThis->s_AppliedEffectGroups.Data[i] = pThis->s_AppliedEffectGroups.Data[pThis->s_AppliedEffectGroups.Count - 1];
		pThis->s_AppliedEffectGroups.Count--;
	}

	pThis->bNetDirty = 1;
	LogCallEnd();
}
