#include "src/GameServer/TgGame/TgEffectManager/RemoveAllEffects/TgEffectManager__RemoveAllEffects.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Removes all effects from this effect manager.
// ExcludeCategoryCodes is a TArray<int>* of category codes to skip (may be null).
// Clears all 16 managed effect list slots, s_AppliedEffectGroups, and resets invulnerability.
void __fastcall TgEffectManager__RemoveAllEffects::Call(ATgEffectManager* pThis, void* edx, void* ExcludeCategoryCodes) {
	LogCallBegin();

	// Clear all 16 managed effect rep slots (TGEA_MAX_EFFECTS = 16)
	for (int i = 0; i < 0x10; i++) {
		pThis->r_ManagedEffectList[i].nEffectGroupID = 0;
		pThis->r_ManagedEffectList[i].byNumStacks = 0;
		pThis->r_ManagedEffectList[i].fInitTimeRemaining = 0.0f;
		pThis->r_ManagedEffectList[i].nExtraInfo = 0;
		pThis->m_fTimeRemaining[i] = 0.0f;
	}

	// Clear applied effect groups (don't free Data — UObjects are GC-managed)
	pThis->s_AppliedEffectGroups.Count = 0;

	// Reset invulnerability count
	pThis->r_nInvulnerableCount = 0;

	// Mark dirty for replication
	pThis->bNetDirty = 1;

	LogCallEnd();
}

