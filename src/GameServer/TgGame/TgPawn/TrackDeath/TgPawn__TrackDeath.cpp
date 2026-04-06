#include "src/GameServer/TgGame/TgPawn/TrackDeath/TgPawn__TrackDeath.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn__TrackDeath::Call(ATgPawn* Pawn, void* edx) {
	LogCallBegin();
	CallOriginal(Pawn, edx);
	LogCallEnd();
}
