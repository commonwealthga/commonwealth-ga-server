#include "src/GameServer/TgGame/TgPawn_Character/VanityPetDestroyed/TgPawn_Character__VanityPetDestroyed.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn_Character__VanityPetDestroyed::Call(ATgPawn_Character* Pawn, void* edx, ATgPawn_VanityPet* Pet) {
	LogCallBegin();
	CallOriginal(Pawn, edx, Pet);
	LogCallEnd();
}
