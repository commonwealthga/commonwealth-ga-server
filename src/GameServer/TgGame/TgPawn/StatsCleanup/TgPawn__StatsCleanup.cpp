#include "src/GameServer/TgGame/TgPawn/StatsCleanup/TgPawn__StatsCleanup.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn__StatsCleanup::Call(ATgPawn* Pawn, void* edx) {
	LogCallBegin();
	CallOriginal(Pawn, edx);
	LogCallEnd();
}
