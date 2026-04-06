#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPawn__KillDeployables : public HookBase<
	void(__fastcall*)(ATgPawn*, void*, unsigned long),
	0x109c0870,
	TgPawn__KillDeployables> {
public:
	static void __fastcall Call(ATgPawn* Pawn, void* edx, unsigned long bAll);
	static inline void __fastcall CallOriginal(ATgPawn* Pawn, void* edx, unsigned long bAll) {
		m_original(Pawn, edx, bAll);
	};
};
