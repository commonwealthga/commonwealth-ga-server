#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPawn__UpdateDebuffer : public HookBase<
	void(__fastcall*)(ATgPawn*, void*, ATgPawn*),
	0x109c0a40,
	TgPawn__UpdateDebuffer> {
public:
	static void __fastcall Call(ATgPawn* Pawn, void* edx, ATgPawn* Debuffer);
	static inline void __fastcall CallOriginal(ATgPawn* Pawn, void* edx, ATgPawn* Debuffer) {
		m_original(Pawn, edx, Debuffer);
	};
};
