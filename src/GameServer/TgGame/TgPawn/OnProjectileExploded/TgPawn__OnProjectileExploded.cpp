#include "src/GameServer/TgGame/TgPawn/OnProjectileExploded/TgPawn__OnProjectileExploded.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn__OnProjectileExploded::Call(ATgPawn* Pawn, void* edx) {
	LogCallBegin();
	CallOriginal(Pawn, edx);
	LogCallEnd();
}
