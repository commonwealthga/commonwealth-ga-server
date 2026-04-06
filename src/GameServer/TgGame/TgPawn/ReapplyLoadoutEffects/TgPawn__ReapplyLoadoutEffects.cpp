#include "src/GameServer/TgGame/TgPawn/ReapplyLoadoutEffects/TgPawn__ReapplyLoadoutEffects.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn__ReapplyLoadoutEffects::Call(ATgPawn* Pawn, void* edx) {
	LogCallBegin();
	CallOriginal(Pawn, edx);
	LogCallEnd();
}
