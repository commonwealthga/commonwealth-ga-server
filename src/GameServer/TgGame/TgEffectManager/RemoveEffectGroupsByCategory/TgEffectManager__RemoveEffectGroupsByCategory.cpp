#include "src/GameServer/TgGame/TgEffectManager/RemoveEffectGroupsByCategory/TgEffectManager__RemoveEffectGroupsByCategory.hpp"
#include "src/Utils/Logger/Logger.hpp"

bool __fastcall TgEffectManager__RemoveEffectGroupsByCategory::Call(ATgEffectManager* Manager, void* edx, int nCategoryCode, int nQuantity) {
	LogCallBegin();
	bool result = CallOriginal(Manager, edx, nCategoryCode, nQuantity);
	LogCallEnd();
	return result;
}
