#include "src/GameServer/TgGame/TgPawn/SpawnLoot/TgPawn__SpawnLoot.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn__SpawnLoot::Call(ATgPawn* Pawn, void* edx, ATgRepInfo_TaskForce* tf) {
	LogCallBegin();
	CallOriginal(Pawn, edx, tf);
	LogCallEnd();
}
