#include "src/GameServer/TgGame/TgPawn/GiveKillXp/TgPawn__GiveKillXp.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn__GiveKillXp::Call(ATgPawn* Pawn, void* edx, ATgRepInfo_TaskForce* tf) {
	LogCallBegin();
	CallOriginal(Pawn, edx, tf);
	LogCallEnd();
}
