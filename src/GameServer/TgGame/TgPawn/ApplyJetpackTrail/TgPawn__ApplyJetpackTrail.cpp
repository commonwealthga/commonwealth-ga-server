#include "src/GameServer/TgGame/TgPawn/ApplyJetpackTrail/TgPawn__ApplyJetpackTrail.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn__ApplyJetpackTrail::Call(ATgPawn* Pawn, void* edx, unsigned long bReset) {
	LogCallBegin();
	CallOriginal(Pawn, edx, bReset);
	LogCallEnd();
}
