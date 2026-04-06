#include "src/GameServer/TgGame/TgPawn/OnPetSpawned/TgPawn__OnPetSpawned.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn__OnPetSpawned::Call(ATgPawn* Pawn, void* edx, ATgPawn* Pet) {
	LogCallBegin();
	CallOriginal(Pawn, edx, Pet);
	LogCallEnd();
}
