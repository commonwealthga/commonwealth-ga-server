#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// TgPawn::TickMakeVisibleCalculation @ 0x109C07D0 — original is a stripped
// native (`RET 0x4`, one-byte body). This is the per-tick ramp that should
// drive `m_fMakeVisibleCurrent` (pawn+0xEFC) toward 1.0 when `r_bIsStealthed`
// is set (bit 0x40 at pawn+0x3D0) and back to 0.0 when it isn't. Without it,
// prop 124 flips the stealth bit but no visual fade happens on the client,
// and `IsStealthed()` (which requires `m_fMakeVisibleCurrent != 0`) returns
// false server-side — so stealth is effectively dormant.
//
// __thiscall: ECX = this (ATgPawn*), stack = float DeltaTime. Matches the
// `RET 0x4` original-body epilogue (1 stack arg, 4 bytes, caller is callee's
// problem in __thiscall — RET N pops N bytes after return).
class TgPawn__TickMakeVisibleCalculation : public HookBase<
	void(__fastcall*)(ATgPawn*, void*, float),
	0x109C07D0,
	TgPawn__TickMakeVisibleCalculation> {
public:
	static void __fastcall Call(ATgPawn* Pawn, void* edx, float DeltaTime);
};
