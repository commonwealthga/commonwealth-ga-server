#include "src/GameServer/TgGame/TgPawn/ReapplyCharacterSkillTree/TgPawn__ReapplyCharacterSkillTree.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn__ReapplyCharacterSkillTree::Call(ATgPawn* Pawn, void* edx) {
	LogCallBegin();
	CallOriginal(Pawn, edx);
	LogCallEnd();
}
