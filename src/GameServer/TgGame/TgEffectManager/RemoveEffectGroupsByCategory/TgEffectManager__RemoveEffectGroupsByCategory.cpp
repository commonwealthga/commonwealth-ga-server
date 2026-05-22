#include "src/GameServer/TgGame/TgEffectManager/RemoveEffectGroupsByCategory/TgEffectManager__RemoveEffectGroupsByCategory.hpp"
#include "src/GameServer/TgGame/TgEffectGroup/RemoveEffects/TgEffectGroup__RemoveEffects.hpp"
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
	if (!Manager || nQuantity <= 0) return false;

	int removed = 0;

	// Reverse iteration so swap-deletes don't disturb earlier indices.
	for (int i = Manager->s_AppliedEffectGroups.Count - 1; i >= 0 && removed < nQuantity; i--) {
		UTgEffectGroup* group = Manager->s_AppliedEffectGroups.Data[i];
		if (!group) continue;
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

		// 4. Swap-remove from s_AppliedEffectGroups.
		const int last = Manager->s_AppliedEffectGroups.Count - 1;
		Manager->s_AppliedEffectGroups.Data[i] = Manager->s_AppliedEffectGroups.Data[last];
		Manager->s_AppliedEffectGroups.Count--;

		removed++;
	}

	if (removed > 0) Manager->bNetDirty = 1;
	return removed > 0;
}
