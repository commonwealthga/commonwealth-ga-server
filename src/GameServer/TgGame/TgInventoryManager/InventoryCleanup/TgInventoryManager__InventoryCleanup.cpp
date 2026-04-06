#include "src/GameServer/TgGame/TgInventoryManager/InventoryCleanup/TgInventoryManager__InventoryCleanup.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgInventoryManager__InventoryCleanup::Call(ATgInventoryManager* Manager, void* edx) {
	LogCallBegin();
	CallOriginal(Manager, edx);
	LogCallEnd();
}
