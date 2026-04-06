#include "src/GameServer/TgGame/TgPawn/InitSpawnPets/TgPawn__InitSpawnPets.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgPawn__InitSpawnPets::Call(ATgPawn* Pawn, void* edx, int SpawnTableID) {
	LogCallBegin();
	CallOriginal(Pawn, edx, SpawnTableID);
	LogCallEnd();
}
