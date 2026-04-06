#include "src/GameServer/TgGame/TgPlayerController/ServerActivateInvItem/TgPlayerController__ServerActivateInvItem.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPlayerController__ServerActivateInvItem::Call(ATgPlayerController* Controller, void* edx, int nInvId) {
	LogCallBegin();
	CallOriginal(Controller, edx, nInvId);
	LogCallEnd();
}
