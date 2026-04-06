#include "src/GameServer/TgGame/TgPawn/SetJetpackTrailId/TgPawn__SetJetpackTrailId.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn__SetJetpackTrailId::Call(ATgPawn* Pawn, void* edx, int nId) {
	LogCallBegin();
	CallOriginal(Pawn, edx, nId);
	LogCallEnd();
}
