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

		// 0. Reverse property modifiers — UNLESS this is a regen-ticker that
		//    re-emits each tick (reversing would undo a one-shot resource gift,
		//    then the next tick re-applies it: net 0, or "set to base" when the
		//    pool is low — the power-station +10/sec "set to 10" bug). Two
		//    regen-ticker patterns:
		//      (a) HoT: any effect with apply_on_interval (bit 0x01 @ +0x48) —
		//          the manager owns a per-interval timer; each tick persists.
		//      (b) Station aura: type=264 + lifetime=0 + interval=0 — no manager
		//          timer; the source deployable's fire rate re-emits.
		//    NOT a regen-ticker (so DO reverse): jetpack-pulse (type=263,
		//    lifetime=0, interval=1, aoi=0) — Newest-Wins displaces each pulse;
		//    without reversal +AirSpeed compounds.
		bool isRegenTicker = false;
		for (int e = 0; e < applied->m_Effects.Count; ++e) {
			UTgEffect* eff = applied->m_Effects.Data[e];
			if (!eff || reinterpret_cast<uintptr_t>(eff) < 0x10000u) continue;
			if (*(unsigned int*)((char*)eff + 0x48) & 0x01) { isRegenTicker = true; break; }
		}
		if (!isRegenTicker
			&& applied->m_nType == 264
			&& applied->m_fLifeTime == 0.0f
			&& applied->m_fApplyInterval == 0.0f) {
			isRegenTicker = true;
		}

		AActor* target = applied->m_Target ? applied->m_Target : pThis->r_Owner;
		if (target && !isRegenTicker) {
			TgEffectGroup__RemoveEffects::Call(applied, nullptr, target, 0);
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
