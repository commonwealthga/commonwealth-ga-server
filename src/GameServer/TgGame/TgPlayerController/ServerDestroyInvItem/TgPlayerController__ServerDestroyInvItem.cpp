#include "src/GameServer/TgGame/TgPlayerController/ServerDestroyInvItem/TgPlayerController__ServerDestroyInvItem.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPlayerController__ServerDestroyInvItem::Call(ATgPlayerController* Controller, void* edx, int nInvId, int nCount, unsigned long bSellIt) {
	LogCallBegin();
	CallOriginal(Controller, edx, nInvId, nCount, bSellIt);
	LogCallEnd();
}
