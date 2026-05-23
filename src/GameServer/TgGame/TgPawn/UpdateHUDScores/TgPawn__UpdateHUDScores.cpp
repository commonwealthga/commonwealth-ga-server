#include "src/GameServer/TgGame/TgPawn/UpdateHUDScores/TgPawn__UpdateHUDScores.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Called from PostBeginPlay, Unhacked flow, and ServerUpdateStats.
void __fastcall TgPawn__UpdateHUDScores::Call(ATgPawn* Pawn, void* edx) {
	LogCallBegin();
	CallOriginal(Pawn, edx);
	LogCallEnd();
}
