#include "src/GameServer/TgGame/TgEffectManager/RemoveAllEffectGroups/TgEffectManager__RemoveAllEffectGroups.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgEffectManager__RemoveAllEffectGroups::Call(ATgEffectManager* pThis, void* edx, UTgEffectGroup* EffectGroup) {
	
	for (int i = 0; i < 0x10; i++) {
		pThis->r_ManagedEffectList[i].nEffectGroupID = 0;
		pThis->r_ManagedEffectList[i].byNumStacks = 0;
		pThis->r_ManagedEffectList[i].fInitTimeRemaining = 0.0f;
		pThis->r_ManagedEffectList[i].nExtraInfo = 0;
	}
	
}

