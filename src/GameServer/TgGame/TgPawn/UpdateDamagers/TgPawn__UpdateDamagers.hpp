#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPawn__UpdateDamagers : public HookBase<
	void(__fastcall*)(ATgPawn*, void*, ATgPawn*),
	0x109c0a10,
	TgPawn__UpdateDamagers> {
public:
	static void __fastcall Call(ATgPawn* Pawn, void* edx, ATgPawn* Damager);
	static inline void __fastcall CallOriginal(ATgPawn* Pawn, void* edx, ATgPawn* Damager) {
		m_original(Pawn, edx, Damager);
	};
};
