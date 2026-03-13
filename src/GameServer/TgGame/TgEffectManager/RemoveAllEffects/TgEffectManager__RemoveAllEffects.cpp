#include "src/GameServer/TgGame/TgEffectManager/RemoveAllEffects/TgEffectManager__RemoveAllEffects.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgEffectManager__RemoveAllEffects::Call(ATgEffectManager* pThis, void* edx, void* ExcludeCategoryCodes) {
	
	for (int i = 0; i < 10; i++) {
		// todo: exclude categories
		pThis->r_ManagedEffectList[i].nEffectGroupID = 0;
		pThis->r_ManagedEffectList[i].byNumStacks = 0;
		pThis->r_ManagedEffectList[i].fInitTimeRemaining = 0.0f;
		pThis->r_ManagedEffectList[i].nExtraInfo = 0;
	}
	
}

