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

	int ret = CallOriginal(Manager, edx, nEffectGroupID, fTime, bIsBuff, nHealthChange);

	if (effectsLog) {
		Logger::Log("effects",
			"[SET-REP] egId=%d fTime=%.3f bIsBuff=%d nHpΔ=%d -> ret=%d (branch=%s)\n",
			nEffectGroupID, fTime, (int)(bIsBuff ? 1 : 0), nHealthChange, ret,
			(ret == -1) ? "A:queue" : "B:managed");

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
	// classifies an egId differently.
	if (Manager && fTime == 0.0f && ret >= 0) {
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
