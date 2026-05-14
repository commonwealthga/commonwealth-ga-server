#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// Native @ 0x109668d0 — TgPlayerController::TraceWeaponFire.
// UC signature: `native function Vector TraceWeaponFire(TgDevice Dev,
//                Vector StartTrace, Vector EndTrace, optional Actor TraceActor)`
//
// Thiscall layout per the decompile (Ghidra address from user 2026-05-14):
//   ECX        = ATgPlayerController* this
//   [esp+0x04] = float* out_HitLocation         (caller-supplied output ptr)
//   [esp+0x08] = ATgDevice* Dev
//   [esp+0x0C..0x14] = FVector StartTrace       (3× float = 12 bytes)
//   [esp+0x18..0x20] = FVector EndTrace         (3× float = 12 bytes)
//   [esp+0x24] = AActor* TraceActor             (optional, often null)
//
// The function returns its `param_1` (out_HitLocation). When the trace finds
// no valid hit (all candidates filtered out by the bDeleteMe/blocking check
// in the loop), it copies EndTrace.X/Y/Z to *out_HitLocation as the fallback
// — i.e. "max-range, nothing hit" returns the trace endpoint.
//
// Diagnostic purpose: TgPawn.GetBaseAimRotation computes the projectile spawn
// rotation as `Rotator(HitLocation - WeaponLoc)`. We've observed it producing
// (0, 32764, 0) — Vector(-1, +ε, 0) — during flight, which means HitLocation
// is landing at a position whose Z exactly matches WeaponLoc.Z. This hook
// captures the trace inputs/output so we can see whether (a) Z exact-match
// comes from EndTrace itself (geometry of the bogus stale viewLoc trace) or
// (b) TraceWeaponFire's hit-list filtering is matching something with
// HitLocation.Z hardcoded to WeaponLoc.Z.
class TgPlayerController__TraceWeaponFire : public HookBase<
	float*(__fastcall*)(void*, void*, float*, void*, float, float, float, float, float, float, void*),
	0x109668d0,
	TgPlayerController__TraceWeaponFire> {
public:
	static float* __fastcall Call(
		void* This, void* edx, float* outHit,
		void* Dev,
		float StartX, float StartY, float StartZ,
		float EndX, float EndY, float EndZ,
		void* TraceActor);

	static inline float* __fastcall CallOriginal(
		void* This, void* edx, float* outHit,
		void* Dev,
		float StartX, float StartY, float StartZ,
		float EndX, float EndY, float EndZ,
		void* TraceActor)
	{
		return m_original(This, edx, outHit, Dev, StartX, StartY, StartZ, EndX, EndY, EndZ, TraceActor);
	}
};
