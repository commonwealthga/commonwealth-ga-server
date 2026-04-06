#include "src/GameServer/TgGame/TgPlayerController/ServerCraftItem/TgPlayerController__ServerCraftItem.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPlayerController__ServerCraftItem::Call(ATgPlayerController* Controller, void* edx, int nBlueprintId) {
	LogCallBegin();
	CallOriginal(Controller, edx, nBlueprintId);
	LogCallEnd();
}
