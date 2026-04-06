#include "src/GameServer/TgGame/TgPawn/TrackObjectivePointsByProgress/TgPawn__TrackObjectivePointsByProgress.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn__TrackObjectivePointsByProgress::Call(ATgPawn* Pawn, void* edx, int nPoints) {
	LogCallBegin();
	CallOriginal(Pawn, edx, nPoints);
	LogCallEnd();
}
