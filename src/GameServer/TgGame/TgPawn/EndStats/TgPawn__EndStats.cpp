#include "src/GameServer/TgGame/TgPawn/EndStats/TgPawn__EndStats.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn__EndStats::Call(ATgPawn* Pawn, void* edx) {
	LogCallBegin();
	CallOriginal(Pawn, edx);
	LogCallEnd();
}
