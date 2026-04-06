#include "src/GameServer/TgGame/TgPlayerController/ServerRepairAllUpgrades/TgPlayerController__ServerRepairAllUpgrades.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPlayerController__ServerRepairAllUpgrades::Call(ATgPlayerController* Controller, void* edx) {
	LogCallBegin();
	CallOriginal(Controller, edx);
	LogCallEnd();
}
