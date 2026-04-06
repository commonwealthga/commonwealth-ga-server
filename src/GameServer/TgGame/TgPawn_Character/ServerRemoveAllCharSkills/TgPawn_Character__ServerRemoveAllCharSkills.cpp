#include "src/GameServer/TgGame/TgPawn_Character/ServerRemoveAllCharSkills/TgPawn_Character__ServerRemoveAllCharSkills.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn_Character__ServerRemoveAllCharSkills::Call(ATgPawn_Character* Pawn, void* edx) {
	LogCallBegin();
	CallOriginal(Pawn, edx);
	LogCallEnd();
}
