#include "src/GameServer/TgGame/TgPawn/TrackFromPlayerDeath/TgPawn__TrackFromPlayerDeath.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn__TrackFromPlayerDeath::Call(ATgPawn* Pawn, void* edx, int nDeviceModeID) {
	LogCallBegin();
	CallOriginal(Pawn, edx, nDeviceModeID);
	LogCallEnd();
}
