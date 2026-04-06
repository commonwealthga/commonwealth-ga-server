#include "src/GameServer/TgGame/TgPlayerController/ServerLogSpeedHack/TgPlayerController__ServerLogSpeedHack.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPlayerController__ServerLogSpeedHack::Call(ATgPlayerController* Controller, void* edx) {
	LogCallBegin();
	CallOriginal(Controller, edx);
	LogCallEnd();
}
