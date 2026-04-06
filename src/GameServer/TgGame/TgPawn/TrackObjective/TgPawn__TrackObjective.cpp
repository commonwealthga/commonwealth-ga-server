#include "src/GameServer/TgGame/TgPawn/TrackObjective/TgPawn__TrackObjective.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn__TrackObjective::Call(ATgPawn* Pawn, void* edx, float fDeltaTime) {
	LogCallBegin();
	CallOriginal(Pawn, edx, fDeltaTime);
	LogCallEnd();
}
