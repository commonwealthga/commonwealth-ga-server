#include "src/GameServer/TgGame/TgPawn_Character/SendCharacterSkillMarshal/TgPawn_Character__SendCharacterSkillMarshal.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn_Character__SendCharacterSkillMarshal::Call(ATgPawn_Character* Pawn, void* edx) {
	LogCallBegin();
	CallOriginal(Pawn, edx);
	LogCallEnd();
}
