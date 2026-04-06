#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPawn__InitSpawnPets : public HookBase<
	void(__fastcall*)(ATgPawn*, void*, int),
	0x109be400,
	TgPawn__InitSpawnPets> {
public:
	static void __fastcall Call(ATgPawn* Pawn, void* edx, int SpawnTableID);
	static inline void __fastcall CallOriginal(ATgPawn* Pawn, void* edx, int SpawnTableID) {
		m_original(Pawn, edx, SpawnTableID);
	};
};
