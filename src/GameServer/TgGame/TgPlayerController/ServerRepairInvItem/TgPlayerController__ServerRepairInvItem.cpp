#include "src/GameServer/TgGame/TgPlayerController/ServerRepairInvItem/TgPlayerController__ServerRepairInvItem.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPlayerController__ServerRepairInvItem::Call(ATgPlayerController* Controller, void* edx, int nInvId) {
	LogCallBegin();
	CallOriginal(Controller, edx, nInvId);
	LogCallEnd();
}
