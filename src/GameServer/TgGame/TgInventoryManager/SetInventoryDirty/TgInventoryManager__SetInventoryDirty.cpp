#include "src/GameServer/TgGame/TgInventoryManager/SetInventoryDirty/TgInventoryManager__SetInventoryDirty.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgInventoryManager__SetInventoryDirty::Call(ATgInventoryManager* Manager, void* edx) {
	LogCallBegin();
	CallOriginal(Manager, edx);
	LogCallEnd();
}
