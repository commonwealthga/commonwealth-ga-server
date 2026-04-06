#include "src/GameServer/TgGame/TgPlayerController/ServerMarkSpawnReturn/TgPlayerController__ServerMarkSpawnReturn.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPlayerController__ServerMarkSpawnReturn::Call(ATgPlayerController* Controller, void* edx, ATeleporter* pTeleporter) {
	LogCallBegin();
	CallOriginal(Controller, edx, pTeleporter);
	LogCallEnd();
}
