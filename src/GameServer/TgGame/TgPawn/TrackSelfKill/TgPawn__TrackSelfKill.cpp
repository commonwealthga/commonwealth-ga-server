#include "src/GameServer/TgGame/TgPawn/TrackSelfKill/TgPawn__TrackSelfKill.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn__TrackSelfKill::Call(ATgPawn* Pawn, void* edx, int nDeviceModeID) {
	LogCallBegin();
	CallOriginal(Pawn, edx, nDeviceModeID);
	LogCallEnd();
}
