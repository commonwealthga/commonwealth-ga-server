#include "src/GameServer/TgGame/TgEffectManager/RemoveEffectGroup/TgEffectManager__RemoveEffectGroup.hpp"
#include "src/GameServer/TgGame/TgEffectGroup/RemoveEffects/TgEffectGroup__RemoveEffects.hpp"

// UTgEffectManager::RemoveEffectGroup — original is empty stub @ 0x10a6ef10.
//
// UC declaration: native function bool RemoveEffectGroup(TgEffectGroup eg);
// Called by TgEffectManager.LifeOver (when LifeDone timer fires), by
// ProcessEffect when bRemove=true, and by the stack/replace paths in
// GetNewEffectGroupByApp. Without this, expired effects stay in
// s_AppliedEffectGroups and their HUD slot + ApplyInterval timer keep firing.
//
// Same cleanup as RemoveEffectGroupsByCategory, but for a single group.
bool __fastcall TgEffectManager__RemoveEffectGroup::Call(ATgEffectManager* Manager, void* /*edx*/, UTgEffectGroup* EffectGroup) {
	if (!Manager || !EffectGroup) return false;

	// Find the group in s_AppliedEffectGroups.
	int idx = -1;
	for (int i = 0; i < Manager->s_AppliedEffectGroups.Count; i++) {
		if (Manager->s_AppliedEffectGroups.Data[i] == EffectGroup) { idx = i; break; }
	}
	if (idx < 0) return false;

	// 0. Reverse every modifier the group's effects installed (power regen,
	//    movement speed, damage protection, etc.). Without this step, the
	//    properties stay clamped at the modified value forever after the
	//    group is torn down.  Prefer the group's own m_Target; fall back to
	//    the manager's r_Owner if the group never had InitInstance called.
	AActor* target = EffectGroup->m_Target ? EffectGroup->m_Target : Manager->r_Owner;
	if (target) {
		// Direct call into our impl — `CallOriginal` would route to the empty
		// 0x10a6f3d0 stub even after Install().
		TgEffectGroup__RemoveEffects::Call(EffectGroup, nullptr, target, 0);
	}

	// 1. Cancel any timers (ApplyInterval, LifeDone) this group armed on the
	//    effect manager. Timer layout on AActor (from AActor::ClearTimer):
	//      AActor+0xA0 Timers.Data, AActor+0xA4 Timers.Count, entries 0x1C bytes,
	//      +0x0C flags dword (zero to cancel), +0x14 TimerObject.
	unsigned char* actor = (unsigned char*)Manager;
	unsigned int timerCount = *(unsigned int*)(actor + 0xA4);
	unsigned char* timerData = *(unsigned char**)(actor + 0xA0);
	for (unsigned int t = 0; t < timerCount; t++) {
		unsigned char* td = timerData + t * 0x1C;
		if (*(void**)(td + 0x14) == (void*)EffectGroup) {
			*(unsigned int*)(td + 0x0C) = 0;
		}
	}

	// 2. Invulnerability refcount: category 862 is invuln; decrement on removal.
	if (EffectGroup->m_nCategoryCode == 862 && Manager->r_nInvulnerableCount > 0) {
		Manager->r_nInvulnerableCount--;
	}

	// 3. Free the HUD slot so the buff icon clears on the client.
	int slot = EffectGroup->s_ManagedEffectListIndex;
	if (slot >= 0 && slot < 0x10) {
		Manager->r_ManagedEffectList[slot].nEffectGroupID     = 0;
		Manager->r_ManagedEffectList[slot].byNumStacks        = 0;
		Manager->r_ManagedEffectList[slot].fInitTimeRemaining = 0.0f;
		Manager->r_ManagedEffectList[slot].nExtraInfo         = 0;
		Manager->m_fTimeRemaining[slot]                       = 0.0f;
	}

	// 4. Swap-remove from s_AppliedEffectGroups.
	int last = Manager->s_AppliedEffectGroups.Count - 1;
	Manager->s_AppliedEffectGroups.Data[idx] = Manager->s_AppliedEffectGroups.Data[last];
	Manager->s_AppliedEffectGroups.Count--;

	Manager->bNetDirty = 1;
	return true;
}
