#include "src/GameServer/TgGame/TgPawn_Character/ReapplyLoadoutEffects/TgPawn_Character__ReapplyLoadoutEffects.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn_Character__ReapplyLoadoutEffects::Call(ATgPawn_Character* Pawn, void* edx) {
	LogCallBegin();
	CallOriginal(Pawn, edx);
	LogCallEnd();
}
