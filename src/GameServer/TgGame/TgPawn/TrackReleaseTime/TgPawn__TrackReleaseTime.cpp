#include "src/GameServer/TgGame/TgPawn/TrackReleaseTime/TgPawn__TrackReleaseTime.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn__TrackReleaseTime::Call(ATgPawn* Pawn, void* edx, int nDeviceModeID, float fReleaseTime) {
	LogCallBegin();
	CallOriginal(Pawn, edx, nDeviceModeID, fReleaseTime);
	LogCallEnd();
}
