#include "src/GameServer/TgGame/TgDevice/PopulateInstigatorEquippedDevices/TgDevice__PopulateInstigatorEquippedDevices.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgDevice__PopulateInstigatorEquippedDevices::Call(ATgDevice* Device, void* edx) {
	LogCallBegin();
	CallOriginal(Device, edx);
	LogCallEnd();
}
