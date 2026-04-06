#include "src/GameServer/TgGame/TgPlayerController/ServerDevGiveXP/TgPlayerController__ServerDevGiveXP.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPlayerController__ServerDevGiveXP::Call(ATgPlayerController* Controller, void* edx, int Amt) {
	LogCallBegin();
	CallOriginal(Controller, edx, Amt);
	LogCallEnd();
}
