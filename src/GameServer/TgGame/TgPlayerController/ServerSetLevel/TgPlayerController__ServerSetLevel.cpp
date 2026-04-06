#include "src/GameServer/TgGame/TgPlayerController/ServerSetLevel/TgPlayerController__ServerSetLevel.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPlayerController__ServerSetLevel::Call(ATgPlayerController* Controller, void* edx, int nLevel) {
	LogCallBegin();
	CallOriginal(Controller, edx, nLevel);
	LogCallEnd();
}
