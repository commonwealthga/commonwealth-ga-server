#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPawn_Character__VanityPetDestroyed : public HookBase<
	void(__fastcall*)(ATgPawn_Character*, void*, ATgPawn_VanityPet*),
	0x109c1360,
	TgPawn_Character__VanityPetDestroyed> {
public:
	static void __fastcall Call(ATgPawn_Character* Pawn, void* edx, ATgPawn_VanityPet* Pet);
	static inline void __fastcall CallOriginal(ATgPawn_Character* Pawn, void* edx, ATgPawn_VanityPet* Pet) {
		m_original(Pawn, edx, Pet);
	};
};
