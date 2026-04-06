#include "src/GameServer/TgGame/TgPlayerController/ServerApplyFlair/TgPlayerController__ServerApplyFlair.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPlayerController__ServerApplyFlair::Call(ATgPlayerController* Controller, void* edx, FString* flairName) {
	LogCallBegin();
	CallOriginal(Controller, edx, flairName);
	LogCallEnd();
}
