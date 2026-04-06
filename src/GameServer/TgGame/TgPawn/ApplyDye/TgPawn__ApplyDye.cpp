#include "src/GameServer/TgGame/TgPawn/ApplyDye/TgPawn__ApplyDye.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn__ApplyDye::Call(ATgPawn* Pawn, void* edx, unsigned long bResetDyeMIC) {
	LogCallBegin();
	CallOriginal(Pawn, edx, bResetDyeMIC);
	LogCallEnd();
}
