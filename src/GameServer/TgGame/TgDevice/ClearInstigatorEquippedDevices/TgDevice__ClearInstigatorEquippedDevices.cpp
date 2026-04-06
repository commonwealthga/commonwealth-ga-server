#include "src/GameServer/TgGame/TgDevice/ClearInstigatorEquippedDevices/TgDevice__ClearInstigatorEquippedDevices.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgDevice__ClearInstigatorEquippedDevices::Call(ATgDevice* Device, void* edx) {
	LogCallBegin();
	CallOriginal(Device, edx);
	LogCallEnd();
}
