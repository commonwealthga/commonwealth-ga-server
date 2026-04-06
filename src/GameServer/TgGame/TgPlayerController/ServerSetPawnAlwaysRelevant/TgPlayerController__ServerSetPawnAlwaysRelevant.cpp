#include "src/GameServer/TgGame/TgPlayerController/ServerSetPawnAlwaysRelevant/TgPlayerController__ServerSetPawnAlwaysRelevant.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPlayerController__ServerSetPawnAlwaysRelevant::Call(ATgPlayerController* Controller, void* edx, int nDistSquared) {
	LogCallBegin();
	CallOriginal(Controller, edx, nDistSquared);
	LogCallEnd();
}
