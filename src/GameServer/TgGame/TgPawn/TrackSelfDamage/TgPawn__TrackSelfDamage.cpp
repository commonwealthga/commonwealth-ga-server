#include "src/GameServer/TgGame/TgPawn/TrackSelfDamage/TgPawn__TrackSelfDamage.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn__TrackSelfDamage::Call(ATgPawn* Pawn, void* edx, int nDeviceModeID, int nDamage) {
	LogCallBegin();
	CallOriginal(Pawn, edx, nDeviceModeID, nDamage);
	LogCallEnd();
}
