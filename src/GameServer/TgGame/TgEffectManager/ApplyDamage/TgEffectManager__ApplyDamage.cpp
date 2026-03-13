#include "src/GameServer/TgGame/TgEffectManager/ApplyDamage/TgEffectManager__ApplyDamage.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgEffectManager__ApplyDamage::Call(ATgEffectManager* pThis, void* edx, int nDamage, AActor* aInstigator, int nAttackType, int nDamageType, FImpactInfo* Impact, int nEffectGroupCategory) {
	Logger::Log(GetLogChannel(), "TgEffectManager::ApplyDamage: pThis=%s nDamage=%d instigator=%s attackType=%d damageType=%d effectGroupCategory=%d\n",
		pThis ? pThis->GetFullName() : "null",
		nDamage,
		aInstigator ? aInstigator->GetFullName() : "null",
		nAttackType,
		nDamageType,
		nEffectGroupCategory);

	CallOriginal(pThis, edx, nDamage, aInstigator, nAttackType, nDamageType, Impact, nEffectGroupCategory);
}

