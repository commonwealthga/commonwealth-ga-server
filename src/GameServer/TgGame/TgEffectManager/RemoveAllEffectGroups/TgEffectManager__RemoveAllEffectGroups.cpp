#include "src/GameServer/TgGame/TgEffectManager/RemoveAllEffectGroups/TgEffectManager__RemoveAllEffectGroups.hpp"
#include "src/GameServer/TgGame/TgEffectGroup/RemoveEffects/TgEffectGroup__RemoveEffects.hpp"
#include "src/GameServer/TgGame/BuffEffectRegistry/EffectDisplacementMarker.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Removes all applied effect groups that share the same displacement bucket
// as the given effect group. Called from GetNewEffectGroupByApp before
// applying a replacement effect (Newest Wins / SameInstigator / Strongest
// cases) — which means any property mods those groups installed MUST be
// reversed here, otherwise the caller's subsequent ApplyEffects will stack
// on top of the un-reversed modifiers and compound m_fRaw on every re-apply.
//
// Bucket scope mirrors the cat-302 ("Local") special case used by every
// other displacement path in TgEffectManager.uc — see
// GetStackingEffectGroup (uc:464) and GetRefreshedEffectGroup (uc:504):
//   if (eg.cat == 302)  match by m_nEffectGroupId
//   else                match by m_nCategoryCode
// Without this scope, ANY cat-302 displacement (Newest Wins / Strongest
// Wins / SameInstigator) would wipe every other cat-302 effect — including
// the jetpack's per-pulse +AirSpeed / +FlightAccel effects (egs
// 10450/52/54/56). Dropping FlightAccel triggers the client's
// r_FlightAcceleration ReplicatedEvent → SetPhysics(2) → player falls
// out of the air mid-flight while jetpack VFX/SFX/power-drain continue
// (the jetpack device's UC state has no idea its effects were displaced).
// Concrete trigger: Targeting System (eg 24240) and Inferno-X Cannon
// Mk III (eg 9360) — the only two devices in the DB with cat-302 + app-157
// + type-263.
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

	const int nCategoryCode = EffectGroup->m_nCategoryCode;
	const int nEffectGroupId = EffectGroup->m_nEffectGroupId;
	const bool isLocal = (nCategoryCode == 302);

	// Iterate s_AppliedEffectGroups, remove matching bucket entries
	for (int i = pThis->s_AppliedEffectGroups.Count - 1; i >= 0; i--) {
		UTgEffectGroup* applied = pThis->s_AppliedEffectGroups.Data[i];
		if (applied == nullptr) continue;

		const bool matches = isLocal
			? (applied->m_nEffectGroupId == nEffectGroupId)
			: (applied->m_nCategoryCode == nCategoryCode);
		if (matches) {
			// 0. Reverse property modifiers installed by the applied group's
			//    effects — UNLESS this is a regen-ticker that re-emits each
			//    tick. Two patterns count as "regen-ticker":
			//
			//      (a) HoT pattern: lifetime>0 + interval>0 + any effect with
			//          apply_on_interval_flag=1 (bit 0x01 at effect+0x48).
			//          The effect manager owns the per-interval timer; each
			//          tick's apply is meant to persist as a resource gift.
			//
			//      (b) Station-aura pattern: lifetime=0 + interval=0 + type=264.
			//          NO effect-manager timer at all — the source deployable's
			//          fire rate handles re-emission. Each apply is a one-shot
			//          resource gift that the player has already consumed by
			//          the next tick.
			//
			//    Why this gate exists at all: regen tickers re-emit the same
			//    effect group every tick. Each tick's submission runs
			//    Strongest-displacement (app=157) against the previous tick →
			//    if we reverse, we undo the just-given +N from the player's
			//    resource (HP / POWERPOOL), then the new tick re-applies +N.
			//    Net 0 per tick when the resource is healthy, and worse when
			//    the resource is low: the reverse step (m_fRaw - N) clamps to
			//    0 at m_fMinimum, then the new apply lands at exactly
			//    base_value. Concrete failure: power station +10 power/sec
			//    (eg 13270, prop 243) → instead of +10/sec, the player sees
			//    their power pool "set to 10" whenever it drops below 10,
			//    because (current - 10) clamped to 0 then + 10 = 10. Same
			//    structural bug existed for medical station +154 HP/tick
			//    (eg 6026) but was masked by HP being a large pool.
			//
			//    Earlier shape used `applied->m_fLifeTime > 0.0f` as the
			//    skip-reversal gate. That correctly skipped reversal for
			//    station auras (life=0) but ALSO skipped reversal for one-shot
			//    buffs that happen to have lifetime=0 — most importantly the
			//    jetpack-fire effect (eg 10450/10452/10454/10456: prop 70
			//    +45 AirSpeed, prop 59 +0.02 FlightAccel, life=0, itv=1,
			//    **aoi=0**). Each fire-pulse triggered Newest-Wins displacement
			//    of the previous pulse; the gate skipped reversal → +45
			//    AirSpeed compounded per pulse. The aoi=1 check fixed jetpack
			//    but left stations broken (their aoi=0 too).
			//
			//    The right discriminator is the COMBINATION above:
			//      - HoT (lifetime>0 + aoi=1): aoi flag distinguishes "ticker
			//        gives one-shot resource each interval" from "buff installs
			//        temporary modifier for lifetime then auto-removes".
			//      - Station aura (lifetime=0 + interval=0 + type=264): no
			//        timer at all means there's no future event to reverse;
			//        the resource was given outright and should stick.
			//      - Jetpack-pulse (lifetime=0 + interval=1 + type=263): has
			//        an interval (its own per-pulse RefireCheck), no aoi flag,
			//        gets reversed on displacement so the temporary AirSpeed
			//        modifier doesn't compound.
			bool isRegenTicker = false;
			for (int e = 0; e < applied->m_Effects.Count; ++e) {
				UTgEffect* eff = applied->m_Effects.Data[e];
				if (!eff) continue;
				unsigned int eflags = *(unsigned int*)((char*)eff + 0x48);
				if (eflags & 0x01) { isRegenTicker = true; break; }
			}
			// Station-aura pattern: type=264 (aura), no in-manager timer.
			if (!isRegenTicker
				&& applied->m_nType == 264
				&& applied->m_fLifeTime    == 0.0f
				&& applied->m_fApplyInterval == 0.0f) {
				isRegenTicker = true;
			}
			AActor* target = applied->m_Target ? applied->m_Target : pThis->r_Owner;
			if (target && !isRegenTicker) {
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

			// 5. Mark the displaced entry's egId so the immediately-following
			//    SetEffectRep (called by UC's ProcessEffect after we return)
			//    can recognize this as a SAME-egId re-emission rather than a
			//    first-time apply. SetEffectRep's queue-push gate suppresses
			//    the manual r_EventQueue push when no pre-existing entries
			//    share the new egId, on the assumption that Branch B will
			//    allocate a FRESH slot whose 0 → egId transition triggers a
			//    client-side respawn. But in this displacement path the slot-
			//    zero (step 3 above) and Branch B's re-allocation happen in
			//    the same server frame → engine replication coalesces the
			//    intra-frame writes (final == initial == egId) → client never
			//    sees the transition → no respawn. The marker tells SetEffectRep
			//    "you can't trust the slot transition; queue-push anyway".
			//
			//    We only mark when the displaced entry's egId matches the
			//    incoming egId (same egId re-emission). For category-302 (Local)
			//    matching, that's always true by construction. For non-local
			//    category matching, displaced entries with a DIFFERENT egId
			//    (e.g., heal egId 7322 displacing heal egId 6026 — both cat 1324)
			//    transition slots from old_egId → new_egId observably and don't
			//    need the marker.
			//
			//    Concrete symptom before this fix: Power Station / Medical
			//    Station per-tick auras vanished on the deploying player's own
			//    pawn (cross-pawn path on teammates routed through
			//    suppressManaged which queue-pushed regardless).
			if (applied->m_nEffectGroupId == nEffectGroupId) {
				EffectDisplacementMarker::Mark(pThis, nEffectGroupId);
			}
		}
	}

	pThis->bNetDirty = 1;

	LogCallEnd();
}

