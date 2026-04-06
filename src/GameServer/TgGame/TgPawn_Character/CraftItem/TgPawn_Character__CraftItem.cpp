#include "src/GameServer/TgGame/TgPawn_Character/CraftItem/TgPawn_Character__CraftItem.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn_Character__CraftItem::Call(ATgPawn_Character* Pawn, void* edx, int nBlueprintId, unsigned long bSystemCraft, unsigned long bUseComponents, int nSystemCraftLevel) {
	LogCallBegin();
	CallOriginal(Pawn, edx, nBlueprintId, bSystemCraft, bUseComponents, nSystemCraftLevel);
	LogCallEnd();
}
