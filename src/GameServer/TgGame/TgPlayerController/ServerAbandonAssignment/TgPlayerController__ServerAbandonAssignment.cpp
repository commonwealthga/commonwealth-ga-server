#include "src/GameServer/TgGame/TgPlayerController/ServerAbandonAssignment/TgPlayerController__ServerAbandonAssignment.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPlayerController__ServerAbandonAssignment::Call(ATgPlayerController* Controller, void* edx) {
	LogCallBegin();
	CallOriginal(Controller, edx);
	LogCallEnd();
}
