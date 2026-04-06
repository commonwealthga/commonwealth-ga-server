#include "src/GameServer/TgGame/TgPawn/TrackKill/TgPawn__TrackKill.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn__TrackKill::Call(ATgPawn* Pawn, void* edx, ATgPawn* Killer) {
	LogCallBegin();
	CallOriginal(Pawn, edx, Killer);
	LogCallEnd();
}
