#include "src/GameServer/TgGame/TgEffectManager/RemoveEffectGroup/TgEffectManager__RemoveEffectGroup.hpp"
#include "src/GameServer/TgGame/TgEffectGroup/RemoveEffects/TgEffectGroup__RemoveEffects.hpp"
#include "src/GameServer/TgGame/TgEffectManager/ProcessReactiveSkillBasedEffectGroup/TgEffectManager__ProcessReactiveSkillBasedEffectGroup.hpp"
#include "src/Utils/Logger/Logger.hpp"

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

	int inEgId = EffectGroup->m_nEffectGroupId;
	int inCat  = EffectGroup->m_nCategoryCode;
	int listCount = Manager->s_AppliedEffectGroups.Count;
	Logger::Log("effects",
		"[REMOVE-GROUP] called with egId=%d cat=%d ptr=%p  s_AppliedEffectGroups has %d entries\n",
		inEgId, inCat, (void*)EffectGroup, listCount);
	if (inEgId == 5716) {
		Logger::Log("debug",
			"  [RemoveEffectGroup egId=5716] ENTER manager=0x%p inPtr=0x%p s_Applied count=%d\n",
			Manager, (void*)EffectGroup, listCount);
		for (int i = 0; i < listCount; i++) {
			UTgEffectGroup* g = Manager->s_AppliedEffectGroups.Data[i];
			if (g && g->m_nEffectGroupId == 5716) {
				Logger::Log("debug",
					"  [RemoveEffectGroup egId=5716]   applied[%d]=0x%p slot=%d\n",
					i, (void*)g, g->s_ManagedEffectListIndex);
			}
		}
	}

	// Find the group in s_AppliedEffectGroups.
	//
	// UC's ProcessEffect(bRemove=true) calls RemoveEffectGroup with the
	// *template* pointer that came in via SubmitEffect, but s_AppliedEffectGroups
	// stores *clones* produced by GetClonedEffectGroup (clone shares template's
	// m_nEffectGroupId but is a distinct object). A pointer-identity match fails
	// in this case and the remove silently no-ops — that's the scope-in
	// compounding bug: ApplyEffects keeps stacking m_fRaw on every apply
	// because the paired remove never finds its clone.
	//
	// Primary match: pointer identity (caller passed the clone directly, e.g.
	// our RemoveEffectGroupsByCategory, or UC LifeOver timer).
	// Fallback match: same m_nEffectGroupId (caller passed the template —
	// UC's ProcessEffect and RemoveEffectType do this).
	int idx = -1;
	for (int i = 0; i < Manager->s_AppliedEffectGroups.Count; i++) {
		if (Manager->s_AppliedEffectGroups.Data[i] == EffectGroup) { idx = i; break; }
	}
	if (idx < 0) {
		int egId = EffectGroup->m_nEffectGroupId;
		for (int i = 0; i < Manager->s_AppliedEffectGroups.Count; i++) {
			UTgEffectGroup* applied = Manager->s_AppliedEffectGroups.Data[i];
			if (applied && applied->m_nEffectGroupId == egId) { idx = i; break; }
		}
	}
	if (idx < 0) {
		if (Logger::IsChannelEnabled("effects")) {
			Logger::Log("effects",
				"[REMOVE-GROUP]   NO MATCH for egId=%d — bailing out. Current list:\n", inEgId);
			for (int i = 0; i < Manager->s_AppliedEffectGroups.Count; i++) {
				UTgEffectGroup* a = Manager->s_AppliedEffectGroups.Data[i];
				Logger::Log("effects", "[REMOVE-GROUP]     [%d] egId=%d cat=%d ptr=%p\n",
					i, a ? a->m_nEffectGroupId : -1, a ? a->m_nCategoryCode : -1, (void*)a);
			}
		}
		if (inEgId == 5716) {
			Logger::Log("debug",
				"  [RemoveEffectGroup egId=5716] NO MATCH — clone never landed in s_AppliedEffectGroups\n");
		}
		return false;
	}
	Logger::Log("effects",
		"[REMOVE-GROUP]   matched applied clone at idx=%d  (ptr=%p egId=%d)\n",
		idx, (void*)Manager->s_AppliedEffectGroups.Data[idx],
		Manager->s_AppliedEffectGroups.Data[idx]->m_nEffectGroupId);

	// Work against the applied instance from here on — timer TimerObj, HUD slot
	// index, m_Target all live on the clone, not the template.
	UTgEffectGroup* applied = Manager->s_AppliedEffectGroups.Data[idx];

	// 0. Reverse every modifier the group's effects installed (movement speed,
	//    damage protection, etc.). Reversal is unconditional here — the
	//    `m_fCurrent == 0` skip inside `TgEffectGroup__RemoveEffects` already
	//    protects effects whose Apply was gated out (phantom clones, see
	//    reference_phantom_clone_skipped_apply.md), so we don't need a
	//    lifetime guard.
	//
	//    Earlier (2026-05-07) this had a `m_fLifeTime > 0` gate copied from
	//    `RemoveAllEffectGroups` to fix the power-station +10/tick case, where
	//    Strongest-displacement was reversing a permanent regen apply on every
	//    re-emission. **That gate belongs in RemoveAllEffectGroups (the
	//    displacement caller) only.** RemoveEffectGroup is also the explicit
	//    remove path used by UC `RemoveEffectType` → `ProcessEffect(bRemove=true)`
	//    (scope-out, jetpack-stop, fire-release) AND the Newest-Wins
	//    per-fire-pulse displacement; for both of those the caller's intent is
	//    "reverse this clone." The mis-placed gate broke them: scope-in
	//    repeatedly drove ground speed to zero (~-57.8/cycle), jetpack repeated
	//    use compounded air speed upward (~+115/cycle). Confirmed via SET-PROP
	//    diagnostic.
	//
	//    Prefer the group's own m_Target; fall back to r_Owner if InitInstance
	//    never wired it.
	AActor* target = applied->m_Target ? applied->m_Target : Manager->r_Owner;
	Logger::Log("effects",
		"[REMOVE-GROUP]   target=%p  effects=%d  lifetime=%.2f  reverseModifiers=%s\n",
		(void*)target, applied->m_Effects.Count, applied->m_fLifeTime,
		target ? "yes" : "no (no target)");
	if (target) {
		// Direct call into our impl — `CallOriginal` would route to the empty
		// 0x10a6f3d0 stub even after Install().
		TgEffectGroup__RemoveEffects::Call(applied, nullptr, target, 0);
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
		if (*(void**)(td + 0x14) == (void*)applied) {
			*(unsigned int*)(td + 0x0C) = 0;
		}
	}

	// 2. Invulnerability refcount: category 862 is invuln; decrement on removal.
	if (applied->m_nCategoryCode == 862 && Manager->r_nInvulnerableCount > 0) {
		Manager->r_nInvulnerableCount--;
	}

	// 3. Free the HUD slot so the buff icon clears on the client.
	int slot = applied->s_ManagedEffectListIndex;
	if (inEgId == 5716) {
		Logger::Log("debug",
			"  [RemoveEffectGroup egId=5716] cleared slot=%d (s_ManagedEffectListIndex on clone) — manager=0x%p\n",
			slot, Manager);
	}
	if (slot >= 0 && slot < 0x10) {
		Manager->r_ManagedEffectList[slot].nEffectGroupID     = 0;
		Manager->r_ManagedEffectList[slot].byNumStacks        = 0;
		Manager->r_ManagedEffectList[slot].fInitTimeRemaining = 0.0f;
		Manager->r_ManagedEffectList[slot].nExtraInfo         = 0;
		Manager->m_fTimeRemaining[slot]                       = 0.0f;
	}

	// 4. Swap-remove from s_AppliedEffectGroups.
	int removedCategory = applied->m_nCategoryCode;
	int last = Manager->s_AppliedEffectGroups.Count - 1;
	Manager->s_AppliedEffectGroups.Data[idx] = Manager->s_AppliedEffectGroups.Data[last];
	Manager->s_AppliedEffectGroups.Count--;

	// 5. If we just removed the LAST group of this category, fire the reactive
	//    skill OFF dispatch — mirrors UC `TgEffectManager.ProcessEffect:272-275`'s
	//    `!bCategoryExisted` apply branch. Reactive skills (e.g. Aegis Armament,
	//    +25% Protection-Physical while any shield is active) flip on when the
	//    first group of their required category arrives and off when the last
	//    leaves. The displacement-style remove path (RemoveAllEffectGroups,
	//    called from GetNewEffectGroupByApp before applying a replacement of
	//    the same category) intentionally skips this — the category is about to
	//    be re-populated, the reactive should stay on across the swap.
	bool stillHasCategory = false;
	for (int i = 0; i < Manager->s_AppliedEffectGroups.Count; i++) {
		UTgEffectGroup* g = Manager->s_AppliedEffectGroups.Data[i];
		if (g && g->m_nCategoryCode == removedCategory) { stillHasCategory = true; break; }
	}
	if (!stillHasCategory && removedCategory > 0) {
		TgEffectManager__ProcessReactiveSkillBasedEffectGroup::Call(
			Manager, nullptr, removedCategory, 1u);
	}

	Manager->bNetDirty = 1;
	return true;
}
