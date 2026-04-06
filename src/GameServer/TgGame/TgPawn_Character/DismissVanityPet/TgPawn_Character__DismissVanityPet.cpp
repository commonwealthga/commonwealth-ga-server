#include "src/GameServer/TgGame/TgPawn_Character/DismissVanityPet/TgPawn_Character__DismissVanityPet.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn_Character__DismissVanityPet::Call(ATgPawn_Character* Pawn, void* edx) {
	LogCallBegin();
	CallOriginal(Pawn, edx);
	LogCallEnd();
}
