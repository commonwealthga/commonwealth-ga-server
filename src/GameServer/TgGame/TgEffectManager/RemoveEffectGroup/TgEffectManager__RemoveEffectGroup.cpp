#include "src/GameServer/TgGame/TgEffectManager/RemoveEffectGroup/TgEffectManager__RemoveEffectGroup.hpp"
#include "src/GameServer/TgGame/TgEffectGroup/RemoveEffects/TgEffectGroup__RemoveEffects.hpp"
#include "src/GameServer/TgGame/TgEffectManager/ProcessReactiveSkillBasedEffectGroup/TgEffectManager__ProcessReactiveSkillBasedEffectGroup.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/Utils/Logger/Logger.hpp"

// TgEffectManager::RemoveEffectGroup — original is the empty stub @ 0x10a6ef10.
// UC: `native function bool RemoveEffectGroup(TgEffectGroup eg);` Called by
// LifeOver (LifeDone timer), ProcessEffect(bRemove=true), and the stack/replace
// paths. Without it, expired effects stay in s_AppliedEffectGroups with their
// HUD slot + ApplyInterval timer still live.
//
// Clean-room rebuild:
//   * Slot/form teardown is delegated to the INTACT, refcount-aware
//     `ClearEffectRep(nEffectId, nManagedListIndex)` (0x10a6f030) instead of
//     manually zeroing r_ManagedEffectList — the manual zero ignored
//     c_byCounterList refcounts and could wipe a slot still shared by another
//     group. ClearEffectRep is the counterpart the original native used.
//   * The EffectDisplacementMarker is GONE — SetEffectRep is now the intact
//     native and re-emission/same-frame realloc is handled by its own
//     queue/managed reconciliation (canonical Q4), so there is nothing to mark.

// Intact native counterpart to SetEffectRep.
typedef void(__fastcall* ClearEffectRepFn)(ATgEffectManager*, void*, int /*nEffectId*/, int /*nManagedListIndex*/);
static const ClearEffectRepFn ClearEffectRepNative = (ClearEffectRepFn)0x10a6f030;

bool __fastcall TgEffectManager__RemoveEffectGroup::Call(ATgEffectManager* Manager, void* /*edx*/, UTgEffectGroup* EffectGroup) {
	if (!Manager || !EffectGroup) return false;

	const int inEgId = EffectGroup->m_nEffectGroupId;

	// Find the group in s_AppliedEffectGroups. UC's ProcessEffect(bRemove=true)
	// passes the *template* pointer, but the list stores *clones* (same
	// m_nEffectGroupId, distinct object). Primary match: pointer identity
	// (LifeOver timer / our category remover pass the clone). Fallback: same
	// m_nEffectGroupId (template path) — without it, scope-in compounds because
	// the paired remove never finds its clone.
	int idx = -1;
	for (int i = 0; i < Manager->s_AppliedEffectGroups.Count; i++) {
		if (Manager->s_AppliedEffectGroups.Data[i] == EffectGroup) { idx = i; break; }
	}
	if (idx < 0) {
		for (int i = 0; i < Manager->s_AppliedEffectGroups.Count; i++) {
			UTgEffectGroup* a = Manager->s_AppliedEffectGroups.Data[i];
			if (a && a->m_nEffectGroupId == inEgId) { idx = i; break; }
		}
	}
	if (idx < 0) {
		if (Logger::IsChannelEnabled("effects")) {
			Logger::Log("effects", "[REMOVE-GROUP] no match for egId=%d — bail\n", inEgId);
		}
		return false;
	}

	// Work against the applied clone — timers, HUD slot, m_Target live on it.
	UTgEffectGroup* applied = Manager->s_AppliedEffectGroups.Data[idx];

	// 0. Reverse every modifier the group's effects installed. Unconditional —
	//    the m_fCurrent==0 skip inside RemoveEffects already protects phantom
	//    clones (Apply gated out). The old m_fLifeTime>0 gate belonged only to
	//    the displacement caller (RemoveAllEffectGroups), not this explicit path.
	//    Direct ::Call — CallOriginal would hit the empty 0x10a6f3d0 stub.
	AActor* target = applied->m_Target ? applied->m_Target : Manager->r_Owner;
	if (target) {
		TgEffectGroup__RemoveEffects::Call(applied, nullptr, target, 0);
	}

	// 0a. Reverse ApplyPosture (TgEffectGroup.uc:418). The apply path
	//    (TgEffectManager.uc:313) calls EffectGroup.ApplyPosture() which
	//    writes r_ePosture on the pawn AND sets m_nPostureOverride on the AI
	//    controller (the override then clamps every future SetPosture call to
	//    the stun pose — TgAIController.uc:2495). UC has no reverse path:
	//    TgEffect.Remove for stun-props (166/167/187) only flips
	//    r_eCurrentStunType + un-pauses the AI; nothing resets r_ePosture or
	//    m_nPostureOverride. Result without this block: bot's AI recovers
	//    behaviorally but the replicated posture sticks at STUNNED (5) /
	//    CRITICALFAILURE (11) / BLOCKSTUN (24) forever, so the client
	//    renders the stunned pose indefinitely. Mirror ApplyPosture's two
	//    writes: clear m_nPostureOverride first (so the SetPosture call
	//    isn't re-clamped), then SetPosture(0 = DEFAULT). Skip m_nPosture==31
	//    (TG_POSTURE_NONE — the "no posture change" sentinel, matches
	//    ApplyPosture's early-return).
	if (target && applied->m_nPosture != 0 && applied->m_nPosture != 31) {
		ATgPawn* posturePawn = (ATgPawn*)target;
		AController* ctrl = posturePawn->Controller;
		if (ctrl && ObjectClassCache::ClassNameContains(ctrl->Class, "TgAIController")) {
			((ATgAIController*)ctrl)->m_nPostureOverride = 0;
		}
		posturePawn->eventSetPosture(0, 1.0f, 0);
	}

	// 1. Cancel any timers (ApplyInterval / LifeDone) this group armed.
	//    AActor+0xA0 Timers.Data, +0xA4 Count; entries 0x1C bytes, +0x0C flags
	//    (zero to cancel), +0x14 TimerObject.
	unsigned char* actor = (unsigned char*)Manager;
	unsigned int timerCount = *(unsigned int*)(actor + 0xA4);
	unsigned char* timerData = *(unsigned char**)(actor + 0xA0);
	for (unsigned int t = 0; t < timerCount; t++) {
		unsigned char* td = timerData + t * 0x1C;
		if (*(void**)(td + 0x14) == (void*)applied) {
			*(unsigned int*)(td + 0x0C) = 0;
		}
	}

	// 2. Invulnerability refcount: category 862 is invuln.
	if (applied->m_nCategoryCode == 862 && Manager->r_nInvulnerableCount > 0) {
		Manager->r_nInvulnerableCount--;
	}

	// 3. Release the HUD slot via the intact refcount-aware native (clears the
	//    slot at refcount 0; next UpdateManagedEffectForms tick tears down the
	//    orphaned form). slot may be -1 (never got a slot) — the native then
	//    searches by egId and no-ops if absent.
	const int slot = applied->s_ManagedEffectListIndex;
	ClearEffectRepNative(Manager, nullptr, inEgId, slot);

	// 4. Swap-remove from s_AppliedEffectGroups.
	const int removedCategory = applied->m_nCategoryCode;
	const int last = Manager->s_AppliedEffectGroups.Count - 1;
	Manager->s_AppliedEffectGroups.Data[idx] = Manager->s_AppliedEffectGroups.Data[last];
	Manager->s_AppliedEffectGroups.Count--;

	// 5. If that was the last group of this category, fire the reactive-skill OFF
	//    dispatch (mirrors UC ProcessEffect:272-275 !bCategoryExisted). The
	//    displacement remover (RemoveAllEffectGroups) intentionally skips this —
	//    the category is about to be re-populated across the swap.
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
	if (Logger::IsChannelEnabled("effects")) {
		Logger::Log("effects", "[REMOVE-GROUP] removed egId=%d cat=%d slot=%d\n",
			inEgId, removedCategory, slot);
	}
	return true;
}
