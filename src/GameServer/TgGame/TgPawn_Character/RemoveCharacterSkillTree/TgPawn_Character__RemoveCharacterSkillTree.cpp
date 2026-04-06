#include "src/GameServer/TgGame/TgPawn_Character/RemoveCharacterSkillTree/TgPawn_Character__RemoveCharacterSkillTree.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn_Character__RemoveCharacterSkillTree::Call(ATgPawn_Character* Pawn, void* edx) {
	LogCallBegin();
	CallOriginal(Pawn, edx);
	LogCallEnd();
}
