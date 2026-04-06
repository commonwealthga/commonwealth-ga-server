#include "src/GameServer/TgGame/TgPlayerController/ServerSetSpawnAtMe/TgPlayerController__ServerSetSpawnAtMe.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPlayerController__ServerSetSpawnAtMe::Call(ATgPlayerController* Controller, void* edx, int bSetToMe) {
	LogCallBegin();
	CallOriginal(Controller, edx, bSetToMe);
	LogCallEnd();
}
