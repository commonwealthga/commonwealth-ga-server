#include "src/GameServer/TgGame/TgEffectManager/RemoveAllEffects/TgEffectManager__RemoveAllEffects.hpp"
#include "src/GameServer/TgGame/TgEffectGroup/RemoveEffects/TgEffectGroup__RemoveEffects.hpp"
#include "src/GameServer/TgGame/BuffEffectRegistry/BuffEffectRegistry.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Removes all effects from this effect manager.
// ExcludeCategoryCodes is a TArray<int>* (Data ptr + Count + Max, 12 bytes).
// May be null when called without the optional parm.
//
// UC contract: walk every applied effect group, call its UC-level Remove path
// to reverse property modifiers (stun, accuracy debuffs, movement-speed debuff,
// etc.), tear down timers, free up the rep slot, and pop from the array.
//
// Why this matters for death/respawn: TgPawn.uc:4456-4462 schedules
// `RemoveAllEffects` (or the slightly-narrower RemoveEffectsTimer) on
// transition to Dead state. Without iterating + reversing, every property
// modifier the pawn was carrying at death stays on r_Owner's properties —
// so stun's prop 166 keeps the pawn frozen, scope debuff keeps movement at
// reduced speed, etc., and the respawned pawn inherits all of it.
void __fastcall TgEffectManager__RemoveAllEffects::Call(ATgEffectManager* pThis, void* /*edx*/, void* ExcludeCategoryCodes) {
	LogCallBegin();

	if (pThis == nullptr) {
		LogCallEnd();
		return;
	}

	// Optional exclude-by-category list. UC's RemoveEffectsTimer passes
	// {303, 719} so DeathFade visuals and similar post-death cosmetic effects
	// survive. Read the TArray inline (Data, Count, Max).
	struct ArrInt { int* Data; int Count; int Max; };
	const ArrInt* excl = reinterpret_cast<const ArrInt*>(ExcludeCategoryCodes);
	auto isExcluded = [&](int catCode) -> bool {
		if (!excl || excl->Count <= 0 || excl->Data == nullptr) return false;
		for (int i = 0; i < excl->Count; ++i) {
			if (excl->Data[i] == catCode) return true;
		}
		return false;
	};

	Logger::Log("effects",
		"[REMOVE-ALL-EFFECTS] mgr=%p applied=%d excludes=%d\n",
		(void*)pThis, pThis->s_AppliedEffectGroups.Count,
		excl ? excl->Count : 0);

	// Iterate in reverse so swap-removes don't skip entries.
	for (int i = pThis->s_AppliedEffectGroups.Count - 1; i >= 0; --i) {
		UTgEffectGroup* applied = pThis->s_AppliedEffectGroups.Data[i];
		if (applied == nullptr) continue;

		if (isExcluded(applied->m_nCategoryCode)) {
			Logger::Log("effects",
				"[REMOVE-ALL-EFFECTS]   [%d] eg=%p egId=%d cat=%d -> EXCLUDED\n",
				i, (void*)applied, applied->m_nEffectGroupId, applied->m_nCategoryCode);
			continue;
		}

		// 1. Reverse property modifiers — PER-EFFECT classification, not per-
		//    group. The previous coarse gate (skip the whole group if ANY
		//    effect has aoi=1) was wrong for mixed-aoi groups: the REST device
		//    EG 2654 (cat=772, life=10s, itvl=1s) bundles ONE aoi=1 heal-over-
		//    time tick (prop 211 Missing Health +10%) with SEVEN aoi=0 penalty
		//    effects (prop 49 GroundSpeed -50%, prop 244 PowerPoolRechargeRate
		//    -3%, four protection -100s, prop 65 EffectDamageModifier -50%).
		//    The single aoi=1 tripped the classifier and the seven aoi=0
		//    penalties stayed permanently applied on the pawn's s_Properties.
		//    Same shape affects the cat=303 Poison DoT family (aoi=1 damage
		//    tick + aoi=0 sibling debuff like slow/blind) and any other
		//    "regen-ticker with side effects" pattern.
		//
		//    Per-effect logic:
		//      • aoi=1: tick-style contribution that's already committed as a
		//        one-shot resource gift. Reversing on death would un-give it.
		//      • aoi=0: temporary modifier paired with an explicit remove that
		//        the death path is replacing. MUST be reversed (jetpack-fire
		//        +AirSpeed, iMINIGUN root, scope -GroundSpeed, REST penalties).
		//
		//    Station-aura group-level skip survives below as a separate gate —
		//    stations are PURE aoi=0 (e.g. medstation EG 6026 only effect has
		//    aoi=0) but their applies are one-shot resource gifts via lifetime=0
		//    + interval=0 + type=264 + external re-fire. Per-effect classification
		//    alone wouldn't catch that, so we keep the pattern-based group skip.
		AActor* target = applied->m_Target ? applied->m_Target : pThis->r_Owner;

		// Station-aura group-level skip: pure aoi=0 but lifetime=0 + interval=0
		// + type=264 means the per-tick re-fire from the source deployable owns
		// the lifecycle. Don't reverse anything for these — the gift sticks.
		bool isStationAuraPattern =
			(applied->m_nType == 264 &&
			 applied->m_fLifeTime    == 0.0f &&
			 applied->m_fApplyInterval == 0.0f);

		if (target && !isStationAuraPattern) {
			// Per-effect reversal. Inline rather than calling
			// TgEffectGroup__RemoveEffects so we can filter aoi=1 effects
			// while still using the same m_fCurrent==0 phantom-clone guard
			// and BuffEffectRegistry::Forget cleanup that helper does.
			const bool effectsLog = Logger::IsChannelEnabled("effects");
			for (int e = 0; e < applied->m_Effects.Count; ++e) {
				UTgEffect* eff = applied->m_Effects.Data[e];
				if (!eff) continue;
				unsigned int eflags = *(unsigned int*)((char*)eff + 0x48);
				if (eflags & 0x01) {
					// aoi=1: tick-gift already committed, skip reversal.
					if (effectsLog) {
						Logger::Log("effects",
							"[REMOVE-ALL-EFFECTS]   [%d/eff%d] propId=%d aoi=1 -> SKIP (tick gift)\n",
							i, e, eff->m_nPropertyId);
					}
					continue;
				}
				if (eff->m_fCurrent == 0.0f) {
					// Apply never ran (phantom clone path). UC `event Remove`
					// has unconditional branches for several propIds that
					// would clobber unrelated state if we let them run.
					continue;
				}
				eff->eventRemove(target, 0);
				BuffEffectRegistry::Forget(eff);
			}
		}

		// 2. Cancel any timers (ApplyInterval, LifeDone) this group armed.
		//    Layout matches RemoveAllEffectGroups: timer table at +0xA0/+0xA4
		//    of the EffectManager (an AActor) with 0x1C-byte entries; entry
		//    +0x14 holds the bound object, entry +0x0C the rate. Zeroing rate
		//    deactivates the timer without disturbing the iteration.
		unsigned char* actor = (unsigned char*)pThis;
		unsigned int  timerCount = *(unsigned int*) (actor + 0xA4);
		unsigned char* timerData = *(unsigned char**)(actor + 0xA0);
		for (unsigned int t = 0; t < timerCount; ++t) {
			unsigned char* td = timerData + t * 0x1C;
			if (*(void**)(td + 0x14) == (void*)applied) {
				*(unsigned int*)(td + 0x0C) = 0;
			}
		}

		// 3. Decrement invulnerability count for invuln-category groups.
		if (applied->m_nCategoryCode == 862 && pThis->r_nInvulnerableCount > 0) {
			pThis->r_nInvulnerableCount--;
		}

		// 4. Clear the managed effect rep slot if this group owned one.
		const int slotIdx = applied->s_ManagedEffectListIndex;
		if (slotIdx >= 0 && slotIdx < 0x10) {
			pThis->r_ManagedEffectList[slotIdx].nEffectGroupID      = 0;
			pThis->r_ManagedEffectList[slotIdx].byNumStacks         = 0;
			pThis->r_ManagedEffectList[slotIdx].fInitTimeRemaining  = 0.0f;
			pThis->r_ManagedEffectList[slotIdx].nExtraInfo          = 0;
			pThis->m_fTimeRemaining[slotIdx]                        = 0.0f;
		}

		// 5. Swap-remove from s_AppliedEffectGroups.
		pThis->s_AppliedEffectGroups.Data[i] =
			pThis->s_AppliedEffectGroups.Data[pThis->s_AppliedEffectGroups.Count - 1];
		pThis->s_AppliedEffectGroups.Count--;
	}

	pThis->bNetDirty = 1;

	LogCallEnd();
}
