#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPawn__OnPetSpawned : public HookBase<
	void(__fastcall*)(ATgPawn*, void*, ATgPawn*),
	0x109be3f0,
	TgPawn__OnPetSpawned> {
public:
	static void __fastcall Call(ATgPawn* Pawn, void* edx, ATgPawn* Pet);
	static inline void __fastcall CallOriginal(ATgPawn* Pawn, void* edx, ATgPawn* Pet) {
		m_original(Pawn, edx, Pet);
	};
};
