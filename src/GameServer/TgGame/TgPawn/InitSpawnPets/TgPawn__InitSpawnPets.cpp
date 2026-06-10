#include "src/GameServer/TgGame/TgPawn/InitSpawnPets/TgPawn__InitSpawnPets.hpp"
#include "src/Utils/Logger/Logger.hpp"

// The BASE TgPawn::InitSpawnPets is a stub (0x109be400), but the subclass
// overrides (RecursiveSpawner 0x109dcde0 / Dismantler 0x109dc8e0 /
// WaspSpawner 0x109d94b0 / AttackTransport 0x109dc3e0) survived as
// byte-identical, class-agnostic copies — delegate to the RecursiveSpawner
// copy rather than reconstructing it.
void __fastcall TgPawn__InitSpawnPets::Call(ATgPawn* Pawn, void* edx, int SpawnTableID) {
	LogCallBegin();
	if (Pawn) {
		Logger::Log("pet_spawn",
			"TgPawn::InitSpawnPets(base): pawn=0x%p table=%d — delegating to intact subclass template\n",
			(void*)Pawn, SpawnTableID);
		using InitSpawnPetsFn = void(__fastcall*)(ATgPawn*, void*, int);
		((InitSpawnPetsFn)0x109dcde0)(Pawn, nullptr, SpawnTableID);
	}
	LogCallEnd();
}
