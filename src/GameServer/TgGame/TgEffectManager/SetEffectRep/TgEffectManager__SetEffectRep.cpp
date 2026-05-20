#include "src/GameServer/TgGame/TgEffectManager/SetEffectRep/TgEffectManager__SetEffectRep.hpp"
#include "src/GameServer/TgGame/BuffEffectRegistry/ApplyBuffEffect.hpp"
#include "src/GameServer/TgGame/BuffEffectRegistry/BuffEffectRegistry.hpp"
#include "src/GameServer/TgGame/BuffEffectRegistry/EffectDisplacementMarker.hpp"
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

	// Find the just-added clone in s_AppliedEffectGroups once, up front. UC's
	// ProcessEffect adds the clone before calling SetEffectRep, so the lookup
	// always succeeds for normal apply paths. We use this single clone pointer
	// for THREE downstream decisions in this function (suppress-managed,
	// sustained-stealth queue suppression, buff routing) — keeping it as a
	// single scan avoids re-walking s_AppliedEffectGroups multiple times per
	// apply (was twice before; in worst case three with the buff route).
	//
	// Also count OTHER pre-existing clones with the same egId (besides the
	// just-added one). The queue-push gate below uses this to distinguish:
	//   - sameEgIdOthers == 0: Branch B will allocate a fresh slot (no prior
	//     entry for this egId) → the slot fill itself spawns a fresh FX form
	//     on the client. Queue-pushing would spawn a SECOND form for the same
	//     fx_id — that's the "double sound on Colony scope-in / double visual
	//     on Power Station aura" symptom.
	//   - sameEgIdOthers >= 1: Branch B's "scan/place" finds the existing slot
	//     and refreshes in place (byNumStacks++, fTimeRemaining refreshed) —
	//     this does NOT respawn the form on the client, so per-tick visuals
	//     for continuous-fire stacking weapons (Focused Repair Arm) require
	//     a manual queue push to spawn a fresh transient form each tick.
	UTgEffectGroup* applyingClone = nullptr;
	int sameEgIdOthers = 0;
	if (Manager && Manager->r_Owner) {
		for (int i = Manager->s_AppliedEffectGroups.Count - 1; i >= 0; --i) {
			UTgEffectGroup* g = Manager->s_AppliedEffectGroups.Data[i];
			if (!g || g->m_nEffectGroupId != nEffectGroupID) continue;
			if (applyingClone == nullptr) applyingClone = g;  // newest (last added)
			else                          ++sameEgIdOthers;   // pre-existing
		}
	}

	// Was an entry with this egId just removed by RemoveAllEffectGroups (case
	// 156/157/874 displacement) or RemoveEffectGroup (case 155 max-stack
	// rotation) earlier in this same ProcessEffect call? If so, the slot for
	// this egId was zeroed and Branch B will re-allocate it within the same
	// server frame — engine replication coalesces the intra-frame writes →
	// client doesn't see the transition → no respawn from the slot change.
	// We need to queue-push to compensate. ConsumeWasJustDisplaced reads and
	// clears the marker atomically.
	const bool wasJustDisplaced =
		EffectDisplacementMarker::ConsumeWasJustDisplaced(Manager, nEffectGroupID);

	// Suppress managed-list slot for one-shot Hit-family effects with fTime=0.
	// This is the discriminator the retail game's design relies on:
	//
	//   • Hold-to-use devices (Sprint/Spring Stealth, Targeting System, scope
	//     weapons, repair arm, station auras, equip buffs) emit their icon EGs
	//     as type 263 (Fire), 266 (Aim), 261 (Equip), 283 (Equip Mode). These
	//     have explicit cleanup paths (RemoveEffectType on fire-stop,
	//     RemoveAimEffects on release, Unequip cleanup) so the slot is reaped
	//     when the source stops emitting.
	//
	//   • Hit-family effects (type 264 Hit, 505 Hit Situational) apply on
	//     impact / proximity and have NO source-driven refresh and NO natural
	//     cleanup. The retail design pattern for "deal damage AND show icon" is
	//     to author TWO effect groups: one lifetime=0 with icon_id=0 for the
	//     damage delivery (e.g. Pain Gun EG 26245), and a separate lifetime>0
	//     with app=836 Refresh for the visible debuff icon (e.g. Pain Gun Slow
	//     EG 16909, life=1s; Additional Damage EG 16908, life=0.5s). The
	//     lifetime>0 EG's LifeDone timer reaps the icon naturally after the
	//     source stops refreshing.
	//
	//   • Stations follow the same pattern — medstation heal (EG 6026) and
	//     power station +10/sec (EG 13270) are both type=264, lifetime=0,
	//     icon_id=0. They don't put icons on the lifetime=0 tick precisely
	//     because there's no cleanup path; if a designer wanted an icon they'd
	//     have authored a lifetime>0 companion.
	//
	// So: lifetime=0 + type=264/505 + icon>0 in the DB is data baggage. The
	// retail SetEffectRep skipped slot allocation for these; we mirror that.
	//
	// Concrete cases this fixes:
	//   - Drop pickup HEAL+POWER (EG 4152: cat=775, type=264, life=0, icon=278)
	//   - Self-grenade damage (Longbow / Tremor launcher detonating on self)
	//   - Boss Shrike Energy Cannon AoE hit (EG 17701: cat=302, type=264,
	//     life=0, icon_id=20) — previously caught by the narrow Target!=Instigator
	//     check; now caught by the broader type-based rule.
	//
	// Returning -1 matches the native's Branch A return; UC writes
	// `s_ManagedEffectListIndex = -1` so our Remove paths correctly skip
	// slot-zeroing for this group. The downstream queue-push gate still fires
	// (gated on `suppressManaged`) so impact FX/sound play normally.
	const bool suppressManaged =
		(fTime == 0.0f && applyingClone &&
		 (applyingClone->m_nType == 264 || applyingClone->m_nType == 505));

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
			"[SET-REP] egId=%d fTime=%.3f bIsBuff=%d nHpΔ=%d -> ret=%d (branch=%s sameEgIdOthers=%d wasJustDisplaced=%d)\n",
			nEffectGroupID, fTime, (int)(bIsBuff ? 1 : 0), nHealthChange, ret,
			suppressManaged ? "X:hit-no-lifetime-suppressed"
			                : ((ret == -1) ? "A:queue" : "B:managed"),
			sameEgIdOthers, (int)wasJustDisplaced);

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

	// Force queue push for instant pulses (fTime == 0) — but only when Branch B
	// won't already spawn a fresh form by itself. The native's vtable[0x39c] is
	// `TgEffectManager::GetEffectGroup` (verified): it linearly searches
	// `s_AppliedEffectGroups` for a clone matching the egId. UC `ProcessEffect`
	// adds the new clone before calling SetEffectRep, so GetEffectGroup always
	// finds it → SetEffectRep always takes Branch B (managed list) and never
	// Branch A (queue push).
	//
	// Branch B's behavior depends on whether a slot already holds this egId:
	//   - No existing slot for this egId → Branch B allocates a FRESH slot,
	//     and the slot->egId transition triggers the client to spawn a new
	//     TgEffectForm rendering the effect group's target_fx_id. We do NOT
	//     queue-push here — that would spawn a second form for the same fx_id
	//     (the "double sound on Colony scope-in / double visual on Power
	//     Station aura" bug). The `sameEgIdOthers == 0` check at the top
	//     captures this case: no other clones with this egId means no prior
	//     slot to refresh in place.
	//   - Existing slot for this egId (continuous-fire Stacking pattern,
	//     e.g. Focused Repair Arm @ ~10-30 Hz) → Branch B refreshes the slot
	//     in place (byNumStacks++, fTimeRemaining refreshed). This does NOT
	//     respawn the form on the client; the form's intrinsic FX particle
	//     duration drains away. To keep per-tick visuals visible we manually
	//     push onto r_EventQueue → UpdateQueueEffectForms spawns a fresh
	//     transient TgEffectForm per push.
	//
	// We tested keeping the managed slot populated across ticks (gating
	// RemoveAllEffectGroups / RemoveEffectGroup slot-zeroing on m_fLifeTime>0)
	// to let Branch B's refresh-in-place sustain the form. It regressed: only
	// the very first tick's FX rendered. Branch B refresh genuinely doesn't
	// replay the particle.
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
	// Gate:
	//   - suppressManaged: we skipped Branch B; queue push is the only FX path.
	//   - ret >= 0 && sameEgIdOthers >= 1: Branch B refreshed an existing slot
	//     in place (Stacking tick 2+); no respawn on client → queue push for
	//     particle replay (continuous-fire weapons).
	//   - ret >= 0 && wasJustDisplaced: a same-egId entry was removed earlier
	//     in this frame (Newest/Strongest/SameInstigator displacement or
	//     Stacking max-stack rotation). Slot was zeroed and Branch B
	//     re-allocated it within the same frame — client doesn't see the
	//     transition due to intra-frame coalescing → queue push needed for
	//     the visual to respawn each tick. Concrete case: Power Station +10
	//     power/tick aura on the deploying player's own pawn.
	//   - ret >= 0 && sameEgIdOthers == 0 && !wasJustDisplaced: first apply,
	//     Branch B allocated a FRESH slot from 0 → egId, client respawns
	//     naturally. Queue push would double the FX — skip.
	if (Manager && fTime == 0.0f
	    && (suppressManaged
	        || (ret >= 0 && (sameEgIdOthers >= 1 || wasJustDisplaced)))
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
	// Wins can have transient overlap during a per-pulse swap) — that scan
	// happened at the top of this function and the result is `applyingClone`.
	if (Manager && ret >= 0 && applyingClone && applyingClone->m_Effects.Data) {
		for (int i = 0; i < applyingClone->m_Effects.Count; ++i) {
			UTgEffect* e = applyingClone->m_Effects.Data[i];
			if (BuffEffectRegistry::IsBuff(e)) {
				ApplyBuffEffectFromHook(e, Manager->r_Owner, /*bRemove=*/false);
			}
		}
	}

	return ret;
}
