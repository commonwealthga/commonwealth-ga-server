#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// UC declaration: `native function CheckEffectThreatModifier(out float NewValue);`
// Stripped stub at 0x10a6f280.
//
// Called only from TgEffectDamage.uc:242 — scales fThreat (= fHealthChange the
// damage inflicted on an AI) before AddThreat builds AI hostility.
//
// The canonical scaling prop IS documented after all: 421 Threat Modifier
// (TGPID_THREAT_MODIFIER in TgProperty.uc:43), carried in data by Decoy (-20%),
// Assault Melee III (+50%), Super Tank (+15%) and Recon Rifle Damage (-10%) —
// an earlier note here claimed no such prop existed, which the 2026-08-06
// threat investigation disproved. The body now scales by the attacker's
// buffed 421, mirroring CheckEffectBuffModifier's GetBuffedProperty query.
class TgEffect__CheckEffectThreatModifier : public HookBase<
	void(__fastcall*)(UTgEffect*, void*, float*),
	0x10a6f280,
	TgEffect__CheckEffectThreatModifier> {
public:
	static void __fastcall Call(UTgEffect* effect, void* edx, float* NewValue);
	static inline void __fastcall CallOriginal(UTgEffect* effect, void* edx, float* NewValue) {
		m_original(effect, edx, NewValue);
	};
};
