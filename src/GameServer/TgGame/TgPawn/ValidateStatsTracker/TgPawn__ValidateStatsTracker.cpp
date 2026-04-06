#include "src/GameServer/TgGame/TgPawn/ValidateStatsTracker/TgPawn__ValidateStatsTracker.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn__ValidateStatsTracker::Call(ATgPawn* Pawn, void* edx) {
	LogCallBegin();
	CallOriginal(Pawn, edx);
	LogCallEnd();
}
