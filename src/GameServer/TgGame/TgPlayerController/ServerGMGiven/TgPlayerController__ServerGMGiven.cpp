#include "src/GameServer/TgGame/TgPlayerController/ServerGMGiven/TgPlayerController__ServerGMGiven.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPlayerController__ServerGMGiven::Call(ATgPlayerController* Controller, void* edx, int nCharacterId, int nCurrency) {
	LogCallBegin();
	CallOriginal(Controller, edx, nCharacterId, nCurrency);
	LogCallEnd();
}
