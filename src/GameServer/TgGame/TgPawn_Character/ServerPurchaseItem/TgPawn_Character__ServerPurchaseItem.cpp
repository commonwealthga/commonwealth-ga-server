#include "src/GameServer/TgGame/TgPawn_Character/ServerPurchaseItem/TgPawn_Character__ServerPurchaseItem.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn_Character__ServerPurchaseItem::Call(ATgPawn_Character* Pawn, void* edx, int nLootTableId, int nLootTableItemId, int nItemCount, int nAttemptedCurrencyType, int nAttemptedCostPerItem) {
	LogCallBegin();
	CallOriginal(Pawn, edx, nLootTableId, nLootTableItemId, nItemCount, nAttemptedCurrencyType, nAttemptedCostPerItem);
	LogCallEnd();
}
