#include "src/GameServer/TgGame/TgPawn/TrackObjectivePoints/TgPawn__TrackObjectivePoints.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn__TrackObjectivePoints::Call(ATgPawn* Pawn, void* edx, int nPoints) {
	LogCallBegin();
	CallOriginal(Pawn, edx, nPoints);
	LogCallEnd();
}
