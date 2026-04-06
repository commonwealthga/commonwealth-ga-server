#include "src/GameServer/TgGame/TgPawn/TrackDamagedPlayer/TgPawn__TrackDamagedPlayer.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn__TrackDamagedPlayer::Call(ATgPawn* Pawn, void* edx, ATgPawn* TargetPawn, int nDeviceModeID, int nDamage) {
	LogCallBegin();
	CallOriginal(Pawn, edx, TargetPawn, nDeviceModeID, nDamage);
	LogCallEnd();
}
