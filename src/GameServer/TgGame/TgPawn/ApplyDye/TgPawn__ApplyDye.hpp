#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPawn__ApplyDye : public HookBase<
	void(__fastcall*)(ATgPawn*, void*, unsigned long),
	0x109bfc00,
	TgPawn__ApplyDye> {
public:
	static void __fastcall Call(ATgPawn* Pawn, void* edx, unsigned long bResetDyeMIC);
	static inline void __fastcall CallOriginal(ATgPawn* Pawn, void* edx, unsigned long bResetDyeMIC) {
		m_original(Pawn, edx, bResetDyeMIC);
	};
};
