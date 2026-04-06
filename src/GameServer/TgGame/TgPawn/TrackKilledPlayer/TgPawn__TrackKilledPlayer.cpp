#include "src/GameServer/TgGame/TgPawn/TrackKilledPlayer/TgPawn__TrackKilledPlayer.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn__TrackKilledPlayer::Call(ATgPawn* Pawn, void* edx, int nDeviceModeID) {
	LogCallBegin();
	CallOriginal(Pawn, edx, nDeviceModeID);
	LogCallEnd();
}
