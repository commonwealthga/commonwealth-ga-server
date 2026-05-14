#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// UC declaration: `native function CheckEffectThreatModifier(out float NewValue);`
// Stripped stub at 0x10a6f280.
//
// Called only from TgEffectDamage.uc:242 — scales fThreat (= fHealthChange the
// damage inflicted on an AI) before AddThreat builds AI hostility. The stub
// returns nothing, so fThreat passes through unchanged.
//
// The "correct" implementation would scale by an instigator-side "Threat
// Output Modifier" prop, but there's no documented prop id for it in the DB
// or UC bytecode, and no observed game effect tied to such scaling. We
// install the hook for completeness — so any future "Threat skill" can land
// here without re-plumbing — but the body is intentionally a pass-through.
// If/when the canonical prop is identified, this is the single place to add
// the GetBuffedProperty call.
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
