#include "src/GameServer/TgGame/TgPawn/TrackTeamKill/TgPawn__TrackTeamKill.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn__TrackTeamKill::Call(ATgPawn* Pawn, void* edx, int nDeviceModeID) {
	LogCallBegin();
	CallOriginal(Pawn, edx, nDeviceModeID);
	LogCallEnd();
}
