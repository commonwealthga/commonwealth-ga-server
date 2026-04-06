#include "src/GameServer/TgGame/TgPawn/TrackBuff/TgPawn__TrackBuff.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn__TrackBuff::Call(ATgPawn* Pawn, void* edx, int nDeviceModeID, float fValue) {
	LogCallBegin();
	CallOriginal(Pawn, edx, nDeviceModeID, fValue);
	LogCallEnd();
}
