#include "src/GameServer/TgGame/TgPawn/TrackEscortObjective/TgPawn__TrackEscortObjective.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn__TrackEscortObjective::Call(ATgPawn* Pawn, void* edx, float fDeltaTime) {
	LogCallBegin();
	CallOriginal(Pawn, edx, fDeltaTime);
	LogCallEnd();
}
