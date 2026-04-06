#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPawn_Character__SetCurrentItemProfile : public HookBase<
	void(__fastcall*)(ATgPawn_Character*, void*, int),
	0x109c0680,
	TgPawn_Character__SetCurrentItemProfile> {
public:
	static void __fastcall Call(ATgPawn_Character* Pawn, void* edx, int nItemProfileId);
	static inline void __fastcall CallOriginal(ATgPawn_Character* Pawn, void* edx, int nItemProfileId) {
		m_original(Pawn, edx, nItemProfileId);
	};
};
