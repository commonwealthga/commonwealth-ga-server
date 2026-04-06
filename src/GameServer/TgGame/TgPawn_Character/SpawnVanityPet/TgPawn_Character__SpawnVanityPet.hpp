#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPawn_Character__SpawnVanityPet : public HookBase<
	void(__fastcall*)(ATgPawn_Character*, void*, int),
	0x109c1370,
	TgPawn_Character__SpawnVanityPet> {
public:
	static void __fastcall Call(ATgPawn_Character* Pawn, void* edx, int nInvId);
	static inline void __fastcall CallOriginal(ATgPawn_Character* Pawn, void* edx, int nInvId) {
		m_original(Pawn, edx, nInvId);
	};
};
