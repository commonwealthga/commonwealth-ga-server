#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPawn__SetPhase : public HookBase<
	void(__fastcall*)(ATgPawn*, void*, int),
	0x109c8280,
	TgPawn__SetPhase> {
public:
	static void __fastcall Call(ATgPawn* Pawn, void* edx, int nNewPhase);
	static inline void __fastcall CallOriginal(ATgPawn* Pawn, void* edx, int nNewPhase) {
		m_original(Pawn, edx, nNewPhase);
	};
};
