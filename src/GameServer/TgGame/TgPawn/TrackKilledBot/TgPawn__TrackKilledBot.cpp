#include "src/GameServer/TgGame/TgPawn/TrackKilledBot/TgPawn__TrackKilledBot.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn__TrackKilledBot::Call(ATgPawn* Pawn, void* edx, int nDeviceModeID) {
	LogCallBegin();
	CallOriginal(Pawn, edx, nDeviceModeID);
	LogCallEnd();
}
