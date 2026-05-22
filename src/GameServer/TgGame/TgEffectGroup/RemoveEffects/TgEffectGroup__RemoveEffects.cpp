#include "src/GameServer/TgGame/TgEffectGroup/RemoveEffects/TgEffectGroup__RemoveEffects.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <string>

// TgEffectGroup::RemoveEffects — reimplements stripped stub @ 0x10a6f3d0.
// Reverse each effect via its OWN-class Remove. The SDK eventRemove wrapper
// resolves base TgEffect.Remove and ProcessEvent runs it without virtual
// re-resolution, so a TgEffectBuff would run the base no-op instead of its
// ApplyBuff(remove) override — the bug that made buffs (e.g. +%maxHP) leak.
void DispatchEffectRemove(UTgEffect* effect, AActor* Target, unsigned long bResetToFollow) {
	const char* rawCls = effect->Class ? effect->Class->GetFullName() : nullptr;
	const std::string cls(rawCls ? rawCls : "<null>");
	if (cls.find("TgEffectBuff") != std::string::npos)
		((UTgEffectBuff*)effect)->Remove(Target, bResetToFollow);
	else if (cls.find("TgEffectSensor") != std::string::npos)
		((UTgEffectSensor*)effect)->Remove(Target, bResetToFollow);
	else if (cls.find("TgEffectVisibility") != std::string::npos)
		((UTgEffectVisibility*)effect)->Remove(Target, bResetToFollow);
	else
		effect->eventRemove(Target, bResetToFollow);
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
		if (!effect) continue;

		// Phantom-clone guard: m_fCurrent is set by ApplyEffect as its first
		// statement (TgEffect.uc:116), so 0 here means Apply was gated out
		// (e.g. ProtectionModifier returned 0). The clone still entered
		// s_AppliedEffectGroups via m_bIsManaged, and several UC `Remove`
		// branches mutate pawn state UNCONDITIONALLY (Stun(false), Unhacked,
		// r_bResistTagging, ForceResetTaskForce, yaw-lock) — letting them fire
		// would clobber state a *different* group's Apply set. Skip them.
		if (effect->m_fCurrent == 0.0f) {
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
