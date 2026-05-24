#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// Intact native at 0x109e8570. Per the Ghidra decompile:
//   1. target != null
//   2. self->vtable[0x140](target) != 0     ← Actor::IsEnemy (game-info route)
//   3. (target.flags & 0x2000) → must pass a UC ProcessEvent check
//   4. target NOT in state Dying/Dead
//   → returns true (detected) only if all four pass.
//
// Diagnostic-only hook: logs which leg fails per call so we can localise the
// "scanner ignores me" bug. CallOriginal runs first; we read its return.
class TgPawn_Scanner__DidSensorBeamDetectEnemy : public HookBase<
	int(__fastcall*)(ATgPawn_Scanner*, void*, AActor*),
	0x109e8570,
	TgPawn_Scanner__DidSensorBeamDetectEnemy> {
public:
	static int __fastcall Call(ATgPawn_Scanner* Scanner, void* edx, AActor* target);
	static inline int __fastcall CallOriginal(ATgPawn_Scanner* Scanner, void* edx, AActor* target) {
		return m_original(Scanner, edx, target);
	}
};
