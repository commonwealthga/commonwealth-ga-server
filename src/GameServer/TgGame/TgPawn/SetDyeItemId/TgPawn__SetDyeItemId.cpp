#include "src/GameServer/TgGame/TgPawn/SetDyeItemId/TgPawn__SetDyeItemId.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn__SetDyeItemId::Call(ATgPawn* Pawn, void* edx, int nId, unsigned char eSlot) {
	LogCallBegin();
	CallOriginal(Pawn, edx, nId, eSlot);
	LogCallEnd();
}
