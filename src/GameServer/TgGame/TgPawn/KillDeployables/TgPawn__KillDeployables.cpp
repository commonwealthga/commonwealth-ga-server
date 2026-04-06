#include "src/GameServer/TgGame/TgPawn/KillDeployables/TgPawn__KillDeployables.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn__KillDeployables::Call(ATgPawn* Pawn, void* edx, unsigned long bAll) {
	LogCallBegin();
	CallOriginal(Pawn, edx, bAll);
	LogCallEnd();
}
