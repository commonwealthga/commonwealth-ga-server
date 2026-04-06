#include "src/GameServer/TgGame/TgPlayerController/ServerSetTaskForceLead/TgPlayerController__ServerSetTaskForceLead.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPlayerController__ServerSetTaskForceLead::Call(ATgPlayerController* Controller, void* edx) {
	LogCallBegin();
	CallOriginal(Controller, edx);
	LogCallEnd();
}
