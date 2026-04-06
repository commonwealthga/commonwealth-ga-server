#include "src/GameServer/TgGame/TgPawn/BeginStats/TgPawn__BeginStats.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn__BeginStats::Call(ATgPawn* Pawn, void* edx) {
	LogCallBegin();
	CallOriginal(Pawn, edx);
	LogCallEnd();
}
