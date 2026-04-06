#include "src/GameServer/TgGame/TgPlayerController/ServerCombineItems/TgPlayerController__ServerCombineItems.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPlayerController__ServerCombineItems::Call(ATgPlayerController* Controller, void* edx, int nInvId, int nModKitInvId) {
	LogCallBegin();
	CallOriginal(Controller, edx, nInvId, nModKitInvId);
	LogCallEnd();
}
