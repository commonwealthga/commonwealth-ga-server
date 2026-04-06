#include "src/GameServer/TgGame/TgPlayerController/ServerLoadItemProfile/TgPlayerController__ServerLoadItemProfile.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPlayerController__ServerLoadItemProfile::Call(ATgPlayerController* Controller, void* edx, int nId) {
	LogCallBegin();
	CallOriginal(Controller, edx, nId);
	LogCallEnd();
}
