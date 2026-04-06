#include "src/GameServer/TgGame/TgPlayerController/ServerTestSystemMailItem/TgPlayerController__ServerTestSystemMailItem.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPlayerController__ServerTestSystemMailItem::Call(ATgPlayerController* Controller, void* edx, int nPlayerId, int nItemId, int nQuantity, unsigned long bSystemCraft, int nMaxQuanlityValueId) {
	LogCallBegin();
	CallOriginal(Controller, edx, nPlayerId, nItemId, nQuantity, bSystemCraft, nMaxQuanlityValueId);
	LogCallEnd();
}
