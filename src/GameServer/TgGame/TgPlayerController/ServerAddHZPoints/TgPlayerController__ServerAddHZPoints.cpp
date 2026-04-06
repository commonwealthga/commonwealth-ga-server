#include "src/GameServer/TgGame/TgPlayerController/ServerAddHZPoints/TgPlayerController__ServerAddHZPoints.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPlayerController__ServerAddHZPoints::Call(ATgPlayerController* Controller, void* edx, int nValue, int nCharId) {
	LogCallBegin();
	CallOriginal(Controller, edx, nValue, nCharId);
	LogCallEnd();
}
