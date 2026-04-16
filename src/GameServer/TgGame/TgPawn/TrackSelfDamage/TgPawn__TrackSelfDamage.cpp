#include "src/GameServer/TgGame/TgPawn/TrackSelfDamage/TgPawn__TrackSelfDamage.hpp"

void __fastcall TgPawn__TrackSelfDamage::Call(ATgPawn* Pawn, void* edx, int nDeviceModeID, int nDamage) {
	LogCallBegin();
	CallOriginal(Pawn, edx, nDeviceModeID, nDamage);
	LogCallEnd();
}
