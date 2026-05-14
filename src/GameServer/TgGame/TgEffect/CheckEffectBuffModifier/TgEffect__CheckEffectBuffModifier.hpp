#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// UC declaration: `native function CheckEffectBuffModifier(out float NewValue);`
// Stripped stub at 0x10a6f270 (returns nothing — leaves NewValue unmodified).
//
// Called from FOUR UC sites:
//   - TgEffect.uc:115         — base ApplyEffect, scales every effect's prorated amount
//   - TgEffectDamage.uc:131   — after charge/absorb/weakspot, before final damage application
//   - TgEffectHeal.uc:67      — scales heal amount before applying
//   - TgEffectBuff.uc:156     — scales buff amount (class_res_id=157 — not actually
//                                instantiable in this build; documented for completeness)
//
// With the stub being a no-op, `out NewValue` stays at whatever GetProratedValue
// produced — typically the effect's `m_fBase` — and no instigator-side buff
// scaling reaches the apply path.
class TgEffect__CheckEffectBuffModifier : public HookBase<
	void(__fastcall*)(UTgEffect*, void*, float*),
	0x10a6f270,
	TgEffect__CheckEffectBuffModifier> {
public:
	static void __fastcall Call(UTgEffect* effect, void* edx, float* NewValue);
	static inline void __fastcall CallOriginal(UTgEffect* effect, void* edx, float* NewValue) {
		m_original(effect, edx, NewValue);
	};
};
