#include "src/GameServer/TgGame/TgDevice/ApplyInventoryEquipEffects/TgDevice__ApplyInventoryEquipEffects.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgDevice__ApplyInventoryEquipEffects::Call(ATgDevice* Device, void* edx, unsigned long bRemove) {
	LogCallBegin();
	CallOriginal(Device, edx, bRemove);
	LogCallEnd();
}
