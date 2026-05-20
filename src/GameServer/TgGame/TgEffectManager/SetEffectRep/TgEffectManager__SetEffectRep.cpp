#include "src/GameServer/TgGame/TgEffectManager/SetEffectRep/TgEffectManager__SetEffectRep.hpp"
#include "src/GameServer/TgGame/BuffEffectRegistry/ApplyBuffEffect.hpp"
#include "src/GameServer/TgGame/BuffEffectRegistry/BuffEffectRegistry.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Diagnostic wrap. The native at 0x10a6ffe0 has two internal branches:
//   - vtable[0x39c](this, nEffectGroupID) returns 0 → push to r_EventQueue
//     (one-shot visual FX), returns -1.
//   - vtable[0x39c] returns non-zero → scan/place into r_ManagedEffectList
//     (HUD buff icon slot), returns slot index 0..15 or -1 if full.
// Logging the args + return value lets us tell, per fire, whether the call
// hit the queue path or the managed-list path, and which slot it landed in.
int __fastcall TgEffectManager__SetEffectRep::Call(ATgEffectManager* Manager, void* edx, int nEffectGroupID, float fTime, unsigned long bIsBuff, int nHealthChange) {
	// The snapshot/diff diagnostic is the only consumer of the "effects"
	// channel from this function; the post-call queue-push and buff-routing
	// blocks below are unconditional. Skip the 16-slot snapshot when the
	// channel is off — it ran every effect application in production
	// otherwise.
	const bool effectsLog = Logger::IsChannelEnabled("effects");
	int slotBefore[0x10];
	if (effectsLog) {
		for (int i = 0; i < 0x10; i++) {
			slotBefore[i] = Manager ? Manager->r_ManagedEffectList[i].nEffectGroupID : 0;
		}
	}

	// Suppress managed-list slot for "instant effect from someone else" — the
	// concrete failure was Boss Shrike Energy Cannon AoE hit (eg 17701: cat
	// 302, lifetime 0, icon_id 20) and other enemy hit effects landing
	// permanent lingering icons on the player's HUD strip. fTime==0 means the
	// client never times the slot out, and for cross-pawn hits there's no
	// matching Remove path to clear it explicitly.
	//
	// We can't decide this at BuildEffectGroup time: the SAME template (e.g.
	// any lifetime=0 + icon_id>0 effect) may legitimately be self-cast (scope,
	// rest, certain stealth/aim modes) where the icon SHOULD show — for those,
	// the user releases the input and ServerStopFire/RemoveAimEffects → our
	// RemoveEffectType natives clear the slot via s_ManagedEffectListIndex.
	//
	// Decision at apply time: look up the just-added clone in
	// s_AppliedEffectGroups by egId. If `m_Target != m_Instigator` AND fTime==0
	// (instant cross-pawn effect), skip the original SetEffectRep (no managed
	// slot allocated) but STILL push to the event queue below so the impact
	// sound/FX still play. Returning -1 matches the native's Branch A return,
	// and UC writes `s_ManagedEffectListIndex = -1` — our Remove paths then
	// correctly skip slot-zeroing for this group.
	bool suppressManaged = false;
	UTgEffectGroup* applyingClone = nullptr;
	if (Manager && fTime == 0.0f && Manager->r_Owner) {
		for (int i = Manager->s_AppliedEffectGroups.Count - 1; i >= 0; --i) {
			UTgEffectGroup* g = Manager->s_AppliedEffectGroups.Data[i];
			if (g && g->m_nEffectGroupId == nEffectGroupID) {
				applyingClone = g;
				if (g->m_Target && g->m_Instigator && g->m_Target != g->m_Instigator) {
					suppressManaged = true;
				}
				break;
			}
		}
	}

	int ret = suppressManaged
		? -1
		: CallOriginal(Manager, edx, nEffectGroupID, fTime, bIsBuff, nHealthChange);

	if (nEffectGroupID == 5716) {
		Logger::Log("debug",
			"  [SetEffectRep egId=5716] manager=0x%p fTime=%.2f bIsBuff=%d hpΔ=%d -> slot=%d (suppress=%d)\n",
			Manager, fTime, (int)(bIsBuff ? 1 : 0), nHealthChange, ret, (int)suppressManaged);
		if (Manager) {
			for (int i = 0; i < 0x10; i++) {
				int eg = Manager->r_ManagedEffectList[i].nEffectGroupID;
				if (eg == 5716 || (ret >= 0 && i == ret)) {
					Logger::Log("debug",
						"  [SetEffectRep egId=5716]   slot[%d] egId=%d byNumStacks=%u fTimeRem=%.2f\n",
						i, eg, (unsigned)Manager->r_ManagedEffectList[i].byNumStacks,
						Manager->r_ManagedEffectList[i].fInitTimeRemaining);
				}
			}
		}
	}

	if (effectsLog) {
		Logger::Log("effects",
			"[SET-REP] egId=%d fTime=%.3f bIsBuff=%d nHpΔ=%d -> ret=%d (branch=%s)\n",
			nEffectGroupID, fTime, (int)(bIsBuff ? 1 : 0), nHealthChange, ret,
			suppressManaged ? "X:cross-pawn-instant-suppressed"
			                : ((ret == -1) ? "A:queue" : "B:managed"));

		if (Manager) {
			for (int i = 0; i < 0x10; i++) {
				int after = Manager->r_ManagedEffectList[i].nEffectGroupID;
				if (slotBefore[i] != after) {
					FEffectListEntry& e = Manager->r_ManagedEffectList[i];
					Logger::Log("effects",
						"[SET-REP]   slot[%d]: egId %d -> %d  byNumStacks=%u fTimeRem=%.3f nExtra=0x%x\n",
						i, slotBefore[i], after, (unsigned)e.byNumStacks, e.fInitTimeRemaining, e.nExtraInfo);
				}
			}
		}
	}

	// Force queue push for instant per-tick pulses (fTime == 0). The native's
	// vtable[0x39c] is `TgEffectManager::GetEffectGroup` (verified): it
	// linearly searches `s_AppliedEffectGroups` for a clone matching the egId.
	// UC `ProcessEffect` adds the new clone before calling SetEffectRep, so
	// GetEffectGroup always finds it → SetEffectRep always takes Branch B
	// (managed list refresh-in-place) and never Branch A (queue push).
	//
	// We tested keeping the managed slot populated across ticks (gating
	// RemoveAllEffectGroups / RemoveEffectGroup slot-zeroing on m_fLifeTime>0)
	// to let Branch B's refresh-in-place sustain the form. It regressed: only
	// the very first tick's FX rendered. Branch B refresh genuinely doesn't
	// replay the particle and the form's intrinsic FX duration drains away.
	//
	// Manually pushing onto r_EventQueue per call IS what makes per-tick
	// visuals visible — UpdateQueueEffectForms spawns a fresh transient
	// TgEffectForm per push. Cosmetic gap: continuous-fire weapons (focused
	// repair arm, ~10-30 Hz) respawn so fast it looks like the animation
	// restarts. The underlying root cause is that this binary's UC pipeline
	// always classifies effects as Branch B; without intercepting deeper in
	// ProcessEffect (e.g. removing the clone from s_AppliedEffectGroups
	// momentarily before SetEffectRep) we can't make Branch A fire naturally.
	// User accepts the cosmetic gap — repair arm visuals are at least visible
	// and gameplay works.
	//
	// `ret >= 0` rules out Branch A (which already pushed to the queue itself
	// and returned -1) — guards against a future case where vtable[0x39c]
	// classifies an egId differently. `suppressManaged` is our own skip-the-
	// managed-slot path above; we skipped CallOriginal entirely, so we need
	// to do the queue push ourselves to keep impact sound/FX (the user-visible
	// part of cross-pawn instant hits that we DO want).
	// Suppress queue push for the beacon equip-effect (egId 5716). Queue
	// pushes spawn transient TgEffectForms on the client per push; for FX 525
	// (the carrier ring) the particle loops indefinitely, so the queue-spawned
	// form never tears down even after we clear the managed-list slot at
	// RemoveEffectGroup time. Managed-list slot alone is enough for the
	// sustained pickup visual since only one apply ever happens (no per-tick
	// replay needed), and clearing the slot replicates the teardown.
	const bool isBeaconEquipEffect = (nEffectGroupID == 5716);
	// Suppress queue push for sustained Stealth-category (621) groups. UC's
	// case-156 ("Newest Wins") application logic remove-and-readds on every
	// continuous-fire pulse (e.g. Spring Stealth refire @ 0.5s while held),
	// which without this guard fires a fresh queue push per pulse. Each push
	// spawns a transient TgEffectForm on the client with the FX's full
	// material transition (e.g. fx_id=240 transition_sec=2.0), and the
	// per-pulse cadence restarts that transition over and over for the first
	// 1-2s of fire — visible to the player as the stealth fade-in flickering
	// and then settling once they're fully stealthed. Same shape on release.
	// The managed-list slot alone carries the sustained icon; the stealth
	// material is driven by r_bIsStealthed / r_nApplyStealth replication on
	// the pawn, not by per-pulse TgEffectForm spawns.
	const bool isSustainedStealth =
		(applyingClone && applyingClone->m_nCategoryCode == 621);
	if (Manager && fTime == 0.0f && (ret >= 0 || suppressManaged)
	    && !isBeaconEquipEffect && !isSustainedStealth) {
		int qIdx = Manager->r_nNextQueueIndex;
		if (qIdx >= 0 && qIdx < 0x20) {
			Manager->r_EventQueue[qIdx].nEffectGroupID = nEffectGroupID;
			Manager->r_EventQueue[qIdx].nExtraInfo =
				(nHealthChange & 0xFFFF) | (bIsBuff ? 0x01000000 : 0);
			Manager->r_nNextQueueIndex = (qIdx + 1) % 0x20;
			Manager->bNetDirty = 1;
			if (effectsLog) {
				Logger::Log("effects",
					"[SET-REP]   queue-push idx=%d egId=%d nExtra=0x%x  next=%d\n",
					qIdx, nEffectGroupID, Manager->r_EventQueue[qIdx].nExtraInfo,
					Manager->r_nNextQueueIndex);
			}
		}
	}
	if (isBeaconEquipEffect) {
		Logger::Log("debug",
			"  [SetEffectRep egId=5716] queue-push SUPPRESSED (managed-list slot=%d carries the visual)\n", ret);
	}
	if (isSustainedStealth && effectsLog) {
		Logger::Log("effects",
			"[SET-REP]   queue-push SUPPRESSED egId=%d cat=621 (sustained stealth; FX driven by r_bIsStealthed)\n",
			nEffectGroupID);
	}

	// Buff-routing for class-157 (TgEffectBuff) effects. The class is
	// stripped from this binary, so `BuildEffectGroup` constructed them as
	// base TgEffect and flagged them in `BuffEffectRegistry`. UC's apply
	// chain runs the base `TgEffect.ApplyEffect` body, which writes through
	// `ApplyToProperty → SetProperty(target, propId, ...)` — a silent no-op
	// for "modifier" props (113 Accuracy, 65 Effect Damage Modifier, 352
	// AOE Radius, …) the pawn doesn't carry in `s_Properties`.
	//
	// We need an extra step that routes those effects through
	// `TgPawn::ApplyBuff` so the entry lands in `m_EffectBuffInfo` where
	// `GetBuffedProperty`'s formula consults them. SetEffectRep is the right
	// hook: it fires once per applied clone (Branch B) AFTER the apply
	// chain has run, with the clone already in `s_AppliedEffectGroups`.
	// Find that clone, walk its m_Effects, and route the flagged ones.
	//
	// Scanning in reverse picks the most-recently-added clone if multiple
	// share an egId (Strongest-Wins normally collapses dupes, but Newest-
	// Wins can have transient overlap during a per-pulse swap).
	if (Manager && ret >= 0 && Manager->r_Owner) {
		UTgEffectGroup* clone = nullptr;
		for (int i = Manager->s_AppliedEffectGroups.Count - 1; i >= 0; --i) {
			UTgEffectGroup* g = Manager->s_AppliedEffectGroups.Data[i];
			if (g && g->m_nEffectGroupId == nEffectGroupID) {
				clone = g;
				break;
			}
		}
		if (clone && clone->m_Effects.Data) {
			for (int i = 0; i < clone->m_Effects.Count; ++i) {
				UTgEffect* e = clone->m_Effects.Data[i];
				if (BuffEffectRegistry::IsBuff(e)) {
					ApplyBuffEffectFromHook(e, Manager->r_Owner, /*bRemove=*/false);
				}
			}
		}
	}

	return ret;
}
