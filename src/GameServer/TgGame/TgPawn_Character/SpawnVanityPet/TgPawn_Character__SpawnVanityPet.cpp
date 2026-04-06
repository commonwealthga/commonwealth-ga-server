#include "src/GameServer/TgGame/TgPawn_Character/SpawnVanityPet/TgPawn_Character__SpawnVanityPet.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn_Character__SpawnVanityPet::Call(ATgPawn_Character* Pawn, void* edx, int nInvId) {
	LogCallBegin();
	CallOriginal(Pawn, edx, nInvId);
	LogCallEnd();
}
