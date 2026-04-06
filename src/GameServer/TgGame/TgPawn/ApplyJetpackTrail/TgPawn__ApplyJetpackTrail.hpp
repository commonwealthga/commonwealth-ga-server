#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPawn__ApplyJetpackTrail : public HookBase<
	void(__fastcall*)(ATgPawn*, void*, unsigned long),
	0x109bfc30,
	TgPawn__ApplyJetpackTrail> {
public:
	static void __fastcall Call(ATgPawn* Pawn, void* edx, unsigned long bReset);
	static inline void __fastcall CallOriginal(ATgPawn* Pawn, void* edx, unsigned long bReset) {
		m_original(Pawn, edx, bReset);
	};
};
