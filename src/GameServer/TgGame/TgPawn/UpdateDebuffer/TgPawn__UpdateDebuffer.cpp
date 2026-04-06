#include "src/GameServer/TgGame/TgPawn/UpdateDebuffer/TgPawn__UpdateDebuffer.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn__UpdateDebuffer::Call(ATgPawn* Pawn, void* edx, ATgPawn* Debuffer) {
	LogCallBegin();
	CallOriginal(Pawn, edx, Debuffer);
	LogCallEnd();
}
