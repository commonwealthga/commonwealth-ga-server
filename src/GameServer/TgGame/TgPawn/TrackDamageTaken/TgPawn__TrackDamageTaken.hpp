#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPawn__TrackDamageTaken : public HookBase<
	void(__fastcall*)(ATgPawn*, void*, int),
	0x109c09f0,
	TgPawn__TrackDamageTaken> {
public:
	static void __fastcall Call(ATgPawn* Pawn, void* edx, int nDamage);
	static inline void __fastcall CallOriginal(ATgPawn* Pawn, void* edx, int nDamage) {
		m_original(Pawn, edx, nDamage);
	};
};
