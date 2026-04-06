#include "src/GameServer/TgGame/TgPawn_Character/ReapplyCharacterSkillTree/TgPawn_Character__ReapplyCharacterSkillTree.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn_Character__ReapplyCharacterSkillTree::Call(ATgPawn_Character* Pawn, void* edx) {
	LogCallBegin();
	CallOriginal(Pawn, edx);
	LogCallEnd();
}
