#include "src/GameServer/TgGame/TgPawn/TrackFired/TgPawn__TrackFired.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn__TrackFired::Call(ATgPawn* Pawn, void* edx, int nDeviceModeID) {
	LogCallBegin();
	CallOriginal(Pawn, edx, nDeviceModeID);
	LogCallEnd();
}
