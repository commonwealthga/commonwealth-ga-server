#include "src/GameServer/TgGame/TgPawn/TrackBoost/TgPawn__TrackBoost.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn__TrackBoost::Call(ATgPawn* Pawn, void* edx) {
	LogCallBegin();
	CallOriginal(Pawn, edx);
	LogCallEnd();
}
