#include "src/GameServer/TgGame/TgPawn/TrackDefense/TgPawn__TrackDefense.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn__TrackDefense::Call(ATgPawn* Pawn, void* edx, int nDeviceModeID, int nDamage) {
	LogCallBegin();
	CallOriginal(Pawn, edx, nDeviceModeID, nDamage);
	LogCallEnd();
}
