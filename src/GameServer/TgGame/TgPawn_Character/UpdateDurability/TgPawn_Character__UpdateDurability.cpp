#include "src/GameServer/TgGame/TgPawn_Character/UpdateDurability/TgPawn_Character__UpdateDurability.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn_Character__UpdateDurability::Call(ATgPawn_Character* Pawn, void* edx) {
	LogCallBegin();
	CallOriginal(Pawn, edx);
	LogCallEnd();
}
