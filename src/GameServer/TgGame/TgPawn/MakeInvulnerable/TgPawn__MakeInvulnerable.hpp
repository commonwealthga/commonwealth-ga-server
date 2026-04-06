#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPawn__MakeInvulnerable : public HookBase<
	void(__fastcall*)(ATgPawn*, void*, float),
	0x109bfcc0,
	TgPawn__MakeInvulnerable> {
public:
	static void __fastcall Call(ATgPawn* Pawn, void* edx, float fLength);
	static inline void __fastcall CallOriginal(ATgPawn* Pawn, void* edx, float fLength) {
		m_original(Pawn, edx, fLength);
	};
};
