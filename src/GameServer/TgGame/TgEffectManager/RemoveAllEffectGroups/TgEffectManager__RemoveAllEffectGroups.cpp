#include "src/GameServer/TgGame/TgEffectManager/RemoveAllEffectGroups/TgEffectManager__RemoveAllEffectGroups.hpp"
#include "src/GameServer/TgGame/TgEffectGroup/RemoveEffects/TgEffectGroup__RemoveEffects.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Removes all applied effect groups that share the same m_nCategoryCode as
// the given effect group. Called from GetNewEffectGroupByApp before applying
// a replacement effect (Newest Wins / SameInstigator / Strongest cases) —
// which means any property mods those groups installed MUST be reversed
// here, otherwise the caller's subsequent ApplyEffects will stack on top of
// the un-reversed modifiers and compound m_fRaw on every re-apply.
//
// Previously this function only cleared the HUD slot + s_AppliedEffectGroups
// entry; it did NOT run RemoveEffects, so movement-speed debuffs and similar
// PERC/ADD effects leaked across every Newest-Wins re-apply.
void __fastcall TgEffectManager__RemoveAllEffectGroups::Call(ATgEffectManager* pThis, void* edx, UTgEffectGroup* EffectGroup) {
	LogCallBegin();

	if (EffectGroup == nullptr) {
		LogCallEnd();
		return;
	}

	int nCategoryCode = EffectGroup->m_nCategoryCode;

	// Iterate s_AppliedEffectGroups, remove matching category entries
	for (int i = pThis->s_AppliedEffectGroups.Count - 1; i >= 0; i--) {
		UTgEffectGroup* applied = pThis->s_AppliedEffectGroups.Data[i];
		if (applied == nullptr) continue;

		if (applied->m_nCategoryCode == nCategoryCode) {
			// 0. Reverse property modifiers installed by the applied group's
			//    effects. Runs the symmetric inverse math via UC's
			//    TgEffect.Remove. Prefer the clone's own m_Target; fall back
			//    to the manager's owner if it was never InitInstance'd.
			AActor* target = applied->m_Target ? applied->m_Target : pThis->r_Owner;
			if (target) {
				TgEffectGroup__RemoveEffects::Call(applied, nullptr, target, 0);
			}

			// 1. Cancel any timers (ApplyInterval, LifeDone) this group armed
			//    on the effect manager. Otherwise DoT ticks keep firing after
			//    the group is gone.
			unsigned char* actor = (unsigned char*)pThis;
			unsigned int timerCount = *(unsigned int*)(actor + 0xA4);
			unsigned char* timerData = *(unsigned char**)(actor + 0xA0);
			for (unsigned int t = 0; t < timerCount; t++) {
				unsigned char* td = timerData + t * 0x1C;
				if (*(void**)(td + 0x14) == (void*)applied) {
					*(unsigned int*)(td + 0x0C) = 0;
				}
			}

			// 2. Decrement invulnerability count if this was an invuln effect (category 862)
			if (nCategoryCode == 862 && pThis->r_nInvulnerableCount > 0) {
				pThis->r_nInvulnerableCount--;
			}

			// 3. Clear the managed effect list rep slot if this group had one
			int slotIdx = applied->s_ManagedEffectListIndex;
			if (slotIdx >= 0 && slotIdx < 0x10) {
				pThis->r_ManagedEffectList[slotIdx].nEffectGroupID = 0;
				pThis->r_ManagedEffectList[slotIdx].byNumStacks = 0;
				pThis->r_ManagedEffectList[slotIdx].fInitTimeRemaining = 0.0f;
				pThis->r_ManagedEffectList[slotIdx].nExtraInfo = 0;
				pThis->m_fTimeRemaining[slotIdx] = 0.0f;
			}

			// 4. Remove from array by swapping with last element
			pThis->s_AppliedEffectGroups.Data[i] = pThis->s_AppliedEffectGroups.Data[pThis->s_AppliedEffectGroups.Count - 1];
			pThis->s_AppliedEffectGroups.Count--;
		}
	}

	pThis->bNetDirty = 1;

	LogCallEnd();
}

