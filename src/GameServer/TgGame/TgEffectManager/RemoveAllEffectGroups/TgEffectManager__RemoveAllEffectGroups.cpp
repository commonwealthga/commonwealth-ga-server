#include "src/GameServer/TgGame/TgEffectManager/RemoveAllEffectGroups/TgEffectManager__RemoveAllEffectGroups.hpp"
#include "src/GameServer/TgGame/TgEffectGroup/RemoveEffects/TgEffectGroup__RemoveEffects.hpp"
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
			//    effects — UNLESS this is a regen-ticker (any effect with
			//    apply_on_interval_flag = 1, bit 0x01 at effect+0x48).
			//
			//    Why this gate exists at all: regen tickers (power-station
			//    +N HP / tick, lifetime=0 + interval>0 + aoi=1) re-emit the
			//    same effect group every tick. Each tick's submission runs
			//    Strongest-displacement against the previous tick → if we
			//    reverse, we undo the just-given +N, then the new tick
			//    re-applies +N — net 0 per tick, no regen.
			//
			//    Earlier shape used `applied->m_fLifeTime > 0.0f`. That gate
			//    correctly skipped reversal for power-station regen (life=0)
			//    but ALSO skipped reversal for one-shot buffs that happen to
			//    have lifetime=0 — most importantly the jetpack-fire effect
			//    (eg 10450/10452/10454/10456: prop 70 +45 AirSpeed,
			//    prop 59 +0.02 FlightAccel, life=0, itv=1, **aoi=0**).
			//    Each fire-pulse triggered Newest-Wins displacement of the
			//    previous pulse; the gate skipped reversal → +45 AirSpeed
			//    compounded per pulse, observed in playtest as jetpack
			//    flight speed creeping up over time and especially across
			//    multiple respec cycles (the static +10/+15% AirSpeed skill
			//    deltas amplified the compounded base).
			//
			//    `apply_on_interval_flag` is the right discriminator: it
			//    distinguishes "ticker that gives a one-shot resource each
			//    interval" (aoi=1, e.g. regen, DoT) from "buff that installs
			//    a temporary modifier" (aoi=0, e.g. jetpack, scope, generic
			//    speed buff). For aoi=1, each tick's apply is meant to
			//    persist; reversing on displacement would un-give the
			//    resource. For aoi=0, each application is a single delta
			//    that pairs with a single reversal at remove/displace time.
			bool isRegenTicker = false;
			for (int e = 0; e < applied->m_Effects.Count; ++e) {
				UTgEffect* eff = applied->m_Effects.Data[e];
				if (!eff) continue;
				unsigned int eflags = *(unsigned int*)((char*)eff + 0x48);
				if (eflags & 0x01) { isRegenTicker = true; break; }
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
		}
	}

	pThis->bNetDirty = 1;

	LogCallEnd();
}

