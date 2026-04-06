#include "src/GameServer/TgGame/TgPlayerController/ServerSalvageInvItem/TgPlayerController__ServerSalvageInvItem.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPlayerController__ServerSalvageInvItem::Call(ATgPlayerController* Controller, void* edx, int nInvId, int nCount) {
	LogCallBegin();
	CallOriginal(Controller, edx, nInvId, nCount);
	LogCallEnd();
}
