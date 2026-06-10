#include "src/GameServer/TgGame/TgEffectGroup/RemoveEffects/TgEffectGroup__RemoveEffects.hpp"
#include "src/GameServer/Utils/ObjectClassCache/ObjectClassCache.hpp"
#include "src/Utils/Logger/Logger.hpp"

// TgEffectGroup::RemoveEffects — reimplements stripped stub @ 0x10a6f3d0.
// Reverse each effect via its OWN-class Remove. The SDK eventRemove wrapper
// resolves base TgEffect.Remove and ProcessEvent runs it without virtual
// re-resolution, so a TgEffectBuff would run the base no-op instead of its
// ApplyBuff(remove) override — the bug that made buffs (e.g. +%maxHP) leak.
void DispatchEffectRemove(UTgEffect* effect, AActor* Target, unsigned long bResetToFollow) {
	// Per-class name cache — fires in the per-effect remove loop on every
	// teardown (life-timer expiry / death / dispel / stack-replace), so the
	// previous per-call GetFullName + std::string allocation showed up as
	// random spikes during heavy gameplay. ClassNameContains is one
	// UClass*-keyed hash lookup + string::find against the cached name.
	if      (ObjectClassCache::ClassNameContains(effect, "TgEffectBuff"))       ((UTgEffectBuff*)effect)->Remove(Target, bResetToFollow);
	else if (ObjectClassCache::ClassNameContains(effect, "TgEffectSensor"))     ((UTgEffectSensor*)effect)->Remove(Target, bResetToFollow);
	else if (ObjectClassCache::ClassNameContains(effect, "TgEffectVisibility")) ((UTgEffectVisibility*)effect)->Remove(Target, bResetToFollow);
	else                                                                        effect->eventRemove(Target, bResetToFollow);
}

void __fastcall TgEffectGroup__RemoveEffects::Call(UTgEffectGroup* eg, void* /*edx*/, AActor* Target, unsigned long bResetToFollow) {
	if (!eg) return;

	const bool effectsLog = Logger::IsChannelEnabled("effects");
	if (effectsLog) {
		Logger::Log("effects",
			"[REMOVE-EFFECTS] eg=%p (egId=%d) target=%p effects=%d bReset=%d\n",
			(void*)eg, eg->m_nEffectGroupId, (void*)Target, eg->m_Effects.Count, (int)bResetToFollow);
	}

	for (int i = 0; i < eg->m_Effects.Count; ++i) {
		UTgEffect* effect = eg->m_Effects.Data[i];
		// Two-tier validity check: null AND "small int that snuck in." Live
		// crash signature (May 28-29): effect=0x35d (=861, the
		// TG_PHYSICALITY_MECHANICAL constant our deployable code writes into
		// r_nPhysicalType). Some upstream path is depositing this value into
		// the m_Effects array — likely a stride/offset bug in the new
		// pets/deployables code touching adjacent UObject* fields. The plain
		// `!effect` check passes 861 through; first deref (`effect->Class` at
		// +0x34) then faults at 0x391. Until the corruption source is found,
		// reject anything below the user-mode NULL guard region (first 64KB).
		if (!effect || reinterpret_cast<uintptr_t>(effect) < 0x10000u) {
			if (effect) {
				Logger::Log("effects",
					"[REMOVE-EFFECTS] eg=%p egId=%d effect[%d]=%p — small-int "
					"value, skipping (likely corrupted m_Effects entry)\n",
					(void*)eg, eg->m_nEffectGroupId, i, (void*)effect);
			}
			continue;
		}

		// Phantom-clone guard: m_fCurrent is set by ApplyEffect as its first
		// statement (TgEffect.uc:116), so 0 here means Apply was gated out
		// (e.g. ProtectionModifier returned 0). The clone still entered
		// s_AppliedEffectGroups via m_bIsManaged, and several UC `Remove`
		// branches mutate pawn state UNCONDITIONALLY (Stun(false), Unhacked,
		// r_bResistTagging, ForceResetTaskForce, yaw-lock) — letting them fire
		// would clobber state a *different* group's Apply set. Skip them.
		//
		// EXEMPT TgEffectSensor / TgEffectVisibility: their ApplyEffect
		// overrides never write m_fCurrent (they mutate pawn state directly),
		// so the guard skipped their Remove forever — scanner vis configs
		// persisted after Sensor Boost / Visual Scanner expired, and prop-141
		// r_nStealthDisabled leaked. Their Removes are internally guarded
		// (slot-keyed clear / `>0` decrement / idempotent SetProperty-0).
		const bool guardExempt =
			ObjectClassCache::ClassNameContains(effect, "TgEffectSensor") ||
			ObjectClassCache::ClassNameContains(effect, "TgEffectVisibility");
		if (!guardExempt && effect->m_fCurrent == 0.0f) {
			continue;
		}

		const float beforeCurrent = effect->m_fCurrent;
		DispatchEffectRemove(effect, Target, bResetToFollow);

		if (effectsLog) {
			Logger::Log("effects",
				"[REMOVE-EFFECTS]   effect[%d] propId=%d m_fCurrent=%.2f -> Remove dispatched\n",
				i, effect->m_nPropertyId, beforeCurrent);
		}
	}
}
