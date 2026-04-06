#include "src/GameServer/TgGame/TgPlayerController/ServerAddToken/TgPlayerController__ServerAddToken.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPlayerController__ServerAddToken::Call(ATgPlayerController* Controller, void* edx, int nValue, int nCharId) {
	LogCallBegin();
	CallOriginal(Controller, edx, nValue, nCharId);
	LogCallEnd();
}
