#include "src/GameServer/TgGame/TgPlayerController/ServerChangeCoalition/TgPlayerController__ServerChangeCoalition.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPlayerController__ServerChangeCoalition::Call(ATgPlayerController* Controller, void* edx, unsigned char nCoalition) {
	LogCallBegin();
	CallOriginal(Controller, edx, nCoalition);
	LogCallEnd();
}
