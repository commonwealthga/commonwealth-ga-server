#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPawn__UpdateHUDScores : public HookBase<
	void(__fastcall*)(ATgPawn*, void*),
	0x109c0ab0,
	TgPawn__UpdateHUDScores> {
public:
	static void __fastcall Call(ATgPawn* Pawn, void* edx);
	static inline void __fastcall CallOriginal(ATgPawn* Pawn, void* edx) {
		m_original(Pawn, edx);
	};
};
