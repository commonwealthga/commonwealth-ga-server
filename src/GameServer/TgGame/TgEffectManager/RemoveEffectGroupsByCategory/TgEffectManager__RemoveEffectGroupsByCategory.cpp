#include "src/GameServer/TgGame/TgEffectManager/RemoveEffectGroupsByCategory/TgEffectManager__RemoveEffectGroupsByCategory.hpp"
#include "src/GameServer/TgGame/TgEffectGroup/RemoveEffects/TgEffectGroup__RemoveEffects.hpp"
#include "src/Utils/Logger/Logger.hpp"

// UTgEffectManager::RemoveEffectGroupsByCategory — original is empty stub (0).
//
// UC declaration:
//   native function bool RemoveEffectGroupsByCategory(int nCategoryCode, int nQuantity);
//
// Called from TgEffect::ApplyEffect when m_nPropertyId == 140 — the "cure" /
// "cleanse" effect type:
//   PawnTarget.r_EffectManager.RemoveEffectGroupsByCategory(
//       m_nPropertyValueId,        // the category to purge (e.g. poison cat)
//       int(fProratedAmount));     // how many entries to clear
//
// Without this implemented, healing grenade's cure component silently no-ops
// and poison DoT keeps ticking through the heal.
//
// Implementation mirrors RemoveAllEffectGroups (already implemented) but:
//   - takes a category id directly (not an effect-group template)
//   - respects a quantity cap
//   - cancels the ApplyInterval / LifeDone timer on each removed group so the
//     DoT tick actually stops (not covered by RemoveAllEffectGroups)
//
// Timer layout on AActor (derived from AActor::ClearTimer decomp @ 0x10c80720):
//   AActor+0xA0  Timers.Data    (pointer to FTimerData array)
//   AActor+0xA4  Timers.Count   (int)
//   Each FTimerData is 0x1C bytes:
//     +0x04 FName.Index
//     +0x08 FName.Number
//     +0x0C bLoop/bPaused flags  ← setting this to 0 cancels the timer
//     +0x14 UObject* TimerObject (the group/actor the timer belongs to)
bool __fastcall TgEffectManager__RemoveEffectGroupsByCategory::Call(ATgEffectManager* Manager, void* /*edx*/, int nCategoryCode, int nQuantity) {
	if (!Manager || nQuantity <= 0) return false;

	// Cap nQuantity defensively so pathological 99-type "remove all" values are
	// bounded by the actual list length.
	int removed = 0;

	// Iterate s_AppliedEffectGroups in reverse — lets us swap-delete in place
	// without disrupting earlier indices.
	for (int i = Manager->s_AppliedEffectGroups.Count - 1; i >= 0 && removed < nQuantity; i--) {
		UTgEffectGroup* group = Manager->s_AppliedEffectGroups.Data[i];
		if (!group) continue;
		if (group->m_nCategoryCode != nCategoryCode) continue;

		// 0. Reverse any modifiers the group's effects installed (power regen,
		//    movement speed, damage protection, etc.). Without this, properties
		//    stay clamped at the modified value forever after the group goes.
		AActor* target = group->m_Target ? group->m_Target : Manager->r_Owner;
		if (target) {
			TgEffectGroup__RemoveEffects::Call(group, nullptr, target, 0);
		}

		// 1. Cancel any timers (ApplyInterval, LifeDone) this group armed on
		//    the effect manager. Without this the DoT tick keeps firing even
		//    after the group is removed from s_AppliedEffectGroups.
		unsigned char* actor = (unsigned char*)Manager;
		unsigned int timerCount = *(unsigned int*)(actor + 0xA4);
		unsigned char* timerData = *(unsigned char**)(actor + 0xA0);
		for (unsigned int t = 0; t < timerCount; t++) {
			unsigned char* td = timerData + t * 0x1C;
			if (*(void**)(td + 0x14) == (void*)group) {
				// Match AActor::ClearTimer's cancel action (sets flags dword to 0).
				*(unsigned int*)(td + 0x0C) = 0;
			}
		}

		// 2. Invulnerability refcount: category 862 is invuln per prior
		//    RemoveAllEffectGroups convention; decrement when removing.
		if (nCategoryCode == 862 && Manager->r_nInvulnerableCount > 0) {
			Manager->r_nInvulnerableCount--;
		}

		// 3. Free the HUD slot so the buff icon clears.
		int slot = group->s_ManagedEffectListIndex;
		if (slot >= 0 && slot < 0x10) {
			Manager->r_ManagedEffectList[slot].nEffectGroupID     = 0;
			Manager->r_ManagedEffectList[slot].byNumStacks        = 0;
			Manager->r_ManagedEffectList[slot].fInitTimeRemaining = 0.0f;
			Manager->r_ManagedEffectList[slot].nExtraInfo         = 0;
			Manager->m_fTimeRemaining[slot]                       = 0.0f;
		}

		// 4. Swap-remove from s_AppliedEffectGroups.
		int last = Manager->s_AppliedEffectGroups.Count - 1;
		Manager->s_AppliedEffectGroups.Data[i] = Manager->s_AppliedEffectGroups.Data[last];
		Manager->s_AppliedEffectGroups.Count--;

		removed++;
	}

	if (removed > 0) {
		Manager->bNetDirty = 1;
	}

	return removed > 0;
}
