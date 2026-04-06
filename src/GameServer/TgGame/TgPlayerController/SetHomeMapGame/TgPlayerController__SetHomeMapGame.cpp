#include "src/GameServer/TgGame/TgPlayerController/SetHomeMapGame/TgPlayerController__SetHomeMapGame.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPlayerController__SetHomeMapGame::Call(ATgPlayerController* Controller, void* edx) {
	LogCallBegin();
	CallOriginal(Controller, edx);
	LogCallEnd();
}
