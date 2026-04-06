#include "src/GameServer/TgGame/TgInventoryManager/ApplyAllEnhancementEffects/TgInventoryManager__ApplyAllEnhancementEffects.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgInventoryManager__ApplyAllEnhancementEffects::Call(ATgInventoryManager* Manager, void* edx, unsigned long bRemove) {
	LogCallBegin();
	CallOriginal(Manager, edx, bRemove);
	LogCallEnd();
}
