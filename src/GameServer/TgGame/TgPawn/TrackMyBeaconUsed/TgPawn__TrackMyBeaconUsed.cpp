#include "src/GameServer/TgGame/TgPawn/TrackMyBeaconUsed/TgPawn__TrackMyBeaconUsed.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn__TrackMyBeaconUsed::Call(ATgPawn* Pawn, void* edx) {
	LogCallBegin();
	CallOriginal(Pawn, edx);
	LogCallEnd();
}
