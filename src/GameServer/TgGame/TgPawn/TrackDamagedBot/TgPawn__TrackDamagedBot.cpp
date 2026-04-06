#include "src/GameServer/TgGame/TgPawn/TrackDamagedBot/TgPawn__TrackDamagedBot.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn__TrackDamagedBot::Call(ATgPawn* Pawn, void* edx, ATgPawn* TargetPawn, int nDeviceModeID, int nDamage) {
	LogCallBegin();
	CallOriginal(Pawn, edx, TargetPawn, nDeviceModeID, nDamage);
	LogCallEnd();
}
