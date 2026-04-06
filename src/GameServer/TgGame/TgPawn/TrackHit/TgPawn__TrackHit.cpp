#include "src/GameServer/TgGame/TgPawn/TrackHit/TgPawn__TrackHit.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn__TrackHit::Call(ATgPawn* Pawn, void* edx, int nDeviceModeID, float fDistance, unsigned long bHitPlayer) {
	LogCallBegin();
	CallOriginal(Pawn, edx, nDeviceModeID, fDistance, bHitPlayer);
	LogCallEnd();
}
