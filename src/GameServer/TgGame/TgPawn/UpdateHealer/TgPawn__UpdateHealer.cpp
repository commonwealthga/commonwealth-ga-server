#include "src/GameServer/TgGame/TgPawn/UpdateHealer/TgPawn__UpdateHealer.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn__UpdateHealer::Call(ATgPawn* Pawn, void* edx, ATgPawn* Healer) {
	LogCallBegin();
	CallOriginal(Pawn, edx, Healer);
	LogCallEnd();
}
