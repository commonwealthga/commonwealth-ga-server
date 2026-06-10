#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// native final function SpawnPets();
// STUB in the binary (0x10a7e8a0, returns void). Called by UC
// TgAIController.ExecuteAction case 1516 ("Spawn Pets") every AI think while
// a spawn action is selected — e.g. Tick/Wasp Incubator phase-1 behaviors.
// The companion TgPawn::InitSpawnPets (INTACT on the spawner classes) has
// already built the pawn's personal TgBotFactorySpawnable by the time this
// runs; our job is to drive that factory's spawn chain.
class TgAIController__SpawnPets : public HookBase<
	void(__fastcall*)(ATgAIController*, void*),
	0x10a7e8a0,
	TgAIController__SpawnPets> {
public:
	static void __fastcall Call(ATgAIController* AIC, void* edx);
	static inline void __fastcall CallOriginal(ATgAIController* AIC, void* edx) {
		m_original(AIC, edx);
	};
};
