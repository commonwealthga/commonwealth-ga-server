#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// native function ApplyDamage(int nDamage, optional Actor aInstigator, optional int nAttackType,
//                             optional int nDamageType, optional ImpactInfo Impact,
//                             optional int nEffectGroupCategory);
// Direct damage path used e.g. for falling damage — bypasses TgEffectGroup pipeline.
// Address identified at 0x10a73960.
class TgEffectManager__ApplyDamage : public HookBase<
	void(__fastcall*)(ATgEffectManager*, void*, int, AActor*, int, int, FImpactInfo*, int),
	0x10a73960,
	TgEffectManager__ApplyDamage> {
public:
	static void __fastcall Call(ATgEffectManager* pThis, void* edx, int nDamage, AActor* aInstigator, int nAttackType, int nDamageType, FImpactInfo* Impact, int nEffectGroupCategory);
	static inline void __fastcall CallOriginal(ATgEffectManager* pThis, void* edx, int nDamage, AActor* aInstigator, int nAttackType, int nDamageType, FImpactInfo* Impact, int nEffectGroupCategory) {
		m_original(pThis, edx, nDamage, aInstigator, nAttackType, nDamageType, Impact, nEffectGroupCategory);
	};
};
