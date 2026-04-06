#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPawn__UpdateHealer : public HookBase<
	void(__fastcall*)(ATgPawn*, void*, ATgPawn*),
	0x109c0a20,
	TgPawn__UpdateHealer> {
public:
	static void __fastcall Call(ATgPawn* Pawn, void* edx, ATgPawn* Healer);
	static inline void __fastcall CallOriginal(ATgPawn* Pawn, void* edx, ATgPawn* Healer) {
		m_original(Pawn, edx, Healer);
	};
};
