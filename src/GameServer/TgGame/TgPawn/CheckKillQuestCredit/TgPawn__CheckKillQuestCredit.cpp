#include "src/GameServer/TgGame/TgPawn/CheckKillQuestCredit/TgPawn__CheckKillQuestCredit.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn__CheckKillQuestCredit::Call(ATgPawn* Pawn, void* edx, ATgRepInfo_TaskForce* tf) {
	LogCallBegin();
	CallOriginal(Pawn, edx, tf);
	LogCallEnd();
}
