#include "src/GameServer/TgGame/TgEffectManager/RemoveAllEffectGroups/TgEffectManager__RemoveAllEffectGroups.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Removes all applied effect groups that share the same m_nCategoryCode as the given effect group.
// Called from GetNewEffectGroupByApp before applying a replacement effect (Replace/SameInstigator/Strongest).
// Must also update r_ManagedEffectList rep slots, r_nInvulnerableCount, and s_AppliedEffectGroups.
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
			// Decrement invulnerability count if this was an invuln effect (category 862)
			if (nCategoryCode == 862 && pThis->r_nInvulnerableCount > 0) {
				pThis->r_nInvulnerableCount--;
			}

			// Clear the managed effect list rep slot if this group had one
			int slotIdx = applied->s_ManagedEffectListIndex;
			if (slotIdx >= 0 && slotIdx < 0x10) {
				pThis->r_ManagedEffectList[slotIdx].nEffectGroupID = 0;
				pThis->r_ManagedEffectList[slotIdx].byNumStacks = 0;
				pThis->r_ManagedEffectList[slotIdx].fInitTimeRemaining = 0.0f;
				pThis->r_ManagedEffectList[slotIdx].nExtraInfo = 0;
				pThis->m_fTimeRemaining[slotIdx] = 0.0f;
			}

			// Remove from array by swapping with last element
			pThis->s_AppliedEffectGroups.Data[i] = pThis->s_AppliedEffectGroups.Data[pThis->s_AppliedEffectGroups.Count - 1];
			pThis->s_AppliedEffectGroups.Count--;
		}
	}

	pThis->bNetDirty = 1;

	LogCallEnd();
}

