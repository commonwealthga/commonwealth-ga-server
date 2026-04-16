#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// Instrumentation-only trampoline on TgPawn::DisplayDamageInfo @ 0x109dd3a0.
// Disassembly ends with `RET 0x10` (4 stack dwords) — likely Canvas* + FVector
// (3 floats) as ScreenLoc. We only care whether the call fires and for which
// pawn, so log generically.
class TgPawn__DisplayDamageInfo : public HookBase<
	void(__fastcall*)(void*, void*, void*, float, float, float),
	0x109dd3a0,
	TgPawn__DisplayDamageInfo> {
public:
	static void __fastcall Call(void* Pawn, void* edx, void* Canvas, float x, float y, float z);
	static inline void __fastcall CallOriginal(void* Pawn, void* edx, void* Canvas, float x, float y, float z) {
		m_original(Pawn, edx, Canvas, x, y, z);
	}
};
