#include "src/GameServer/TgGame/TgPawn/RemoveTrackFired/TgPawn__RemoveTrackFired.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn__RemoveTrackFired::Call(ATgPawn* Pawn, void* edx, int nDeviceModeID) {
	LogCallBegin();
	CallOriginal(Pawn, edx, nDeviceModeID);
	LogCallEnd();
}
