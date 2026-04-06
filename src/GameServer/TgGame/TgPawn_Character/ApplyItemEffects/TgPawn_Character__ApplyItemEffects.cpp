#include "src/GameServer/TgGame/TgPawn_Character/ApplyItemEffects/TgPawn_Character__ApplyItemEffects.hpp"
#include "src/Utils/Logger/Logger.hpp"

bool __fastcall TgPawn_Character__ApplyItemEffects::Call(ATgPawn_Character* Pawn, void* edx, UTgInventoryObject* pItem, unsigned long bRemove) {
	LogCallBegin();
	bool result = CallOriginal(Pawn, edx, pItem, bRemove);
	LogCallEnd();
	return result;
}
