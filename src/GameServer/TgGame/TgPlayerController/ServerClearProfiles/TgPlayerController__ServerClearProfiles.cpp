#include "src/GameServer/TgGame/TgPlayerController/ServerClearProfiles/TgPlayerController__ServerClearProfiles.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPlayerController__ServerClearProfiles::Call(ATgPlayerController* Controller, void* edx) {
	LogCallBegin();
	CallOriginal(Controller, edx);
	LogCallEnd();
}
