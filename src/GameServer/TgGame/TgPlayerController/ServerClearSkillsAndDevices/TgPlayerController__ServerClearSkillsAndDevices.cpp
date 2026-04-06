#include "src/GameServer/TgGame/TgPlayerController/ServerClearSkillsAndDevices/TgPlayerController__ServerClearSkillsAndDevices.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPlayerController__ServerClearSkillsAndDevices::Call(ATgPlayerController* Controller, void* edx) {
	LogCallBegin();
	CallOriginal(Controller, edx);
	LogCallEnd();
}
