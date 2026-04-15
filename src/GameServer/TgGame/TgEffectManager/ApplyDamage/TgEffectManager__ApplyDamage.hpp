#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// native function ApplyDamage(int nDamage, optional Actor aInstigator, optional int nAttackType,
//                             optional int nDamageType, optional ImpactInfo Impact,
//                             optional int nEffectGroupCategory);
// Direct damage path used e.g. for falling damage — bypasses TgEffectGroup pipeline.
// Address identified at 0x10a73960.
// IMPORTANT: FImpactInfo is passed BY VALUE (96 bytes on stack, RET 0x74).
// GCC __fastcall can't reliably forward large by-value structs, so the hook is a
// naked trampoline: it calls a logging helper then JMPs to the original, reusing
// the caller's stack frame verbatim.
class TgEffectManager__ApplyDamage : public HookBase<
	void(__fastcall*)(ATgEffectManager*, void*),   // minimal type — actual params stay on stack
	0x10a73960,
	TgEffectManager__ApplyDamage> {
public:
	static void __cdecl LogCall(ATgEffectManager* pThis, int nDamage, AActor* aInstigator,
	                            int nAttackType, int nDamageType, int nEffectGroupCategory);
	static void __fastcall Call(ATgEffectManager* pThis, void* edx);
};
