#include "src/GameServer/TgGame/TgPawn/CheckUseQuestCredit/TgPawn__CheckUseQuestCredit.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn__CheckUseQuestCredit::Call(ATgPawn* Pawn, void* edx) {
	LogCallBegin();
	CallOriginal(Pawn, edx);
	LogCallEnd();
}
