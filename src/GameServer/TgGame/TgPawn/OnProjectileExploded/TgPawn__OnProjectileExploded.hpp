#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPawn__OnProjectileExploded : public HookBase<
	void(__fastcall*)(ATgPawn*, void*),
	0x109bf830,
	TgPawn__OnProjectileExploded> {
public:
	static void __fastcall Call(ATgPawn* Pawn, void* edx);
	static inline void __fastcall CallOriginal(ATgPawn* Pawn, void* edx) {
		m_original(Pawn, edx);
	};
};
