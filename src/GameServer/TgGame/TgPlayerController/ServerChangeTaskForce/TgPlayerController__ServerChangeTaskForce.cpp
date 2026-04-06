#include "src/GameServer/TgGame/TgPlayerController/ServerChangeTaskForce/TgPlayerController__ServerChangeTaskForce.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPlayerController__ServerChangeTaskForce::Call(ATgPlayerController* Controller, void* edx, unsigned char nTaskForce) {
	LogCallBegin();
	CallOriginal(Controller, edx, nTaskForce);
	LogCallEnd();
}
