#include "src/GameServer/TgGame/TgPawn/TrackTeamDamage/TgPawn__TrackTeamDamage.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn__TrackTeamDamage::Call(ATgPawn* Pawn, void* edx, int nDeviceModeID, int nDamage) {
	LogCallBegin();
	CallOriginal(Pawn, edx, nDeviceModeID, nDamage);
	LogCallEnd();
}
