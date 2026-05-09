#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// UTgEffect::TrackStats — UC native, stub @ 0x10a6f2d0. Stripped on our binary
// so no callers credit anything.
//
// Called by UC from TgEffectDamage::ApplyEffect line 257 and
// TgEffectHeal::ApplyEffect line 202 — the unified per-event dealer-side hook.
// Used to be the entry point for the entire stats / morale credit pipeline
// (TrackHit / TrackHealing / TrackKill etc.). We use it as the place to
// credit morale on damage dealt and heal applied.
//
// Native ABI: __thiscall(this, parms positionally on stack). The
// execTrackStats thunk at 0x10992b60 reads parms via GNatives, then pushes
// them positionally before calling vtable[0x128] (our hooked address). Total
// stack push is 0x78 bytes; callee-cleanup = 0x78. Mapped to __fastcall
// (this, edx, ...) per HookBase convention.
//
// POD wrapper for FImpactInfo's 0x60 bytes. The thunk pushes Impact INLINE
// on the stack via MOVSD.REP at 10992d0d (24 dwords). Declaring the parameter
// as the SDK's `FImpactInfo` triggers an MSVC struct-by-value path that
// mismatches the inline layout (likely because FImpactInfo nests
// FTraceHitInfo + `unsigned long bDirectHit:1` bitfield), with the function's
// RET cleanup landing at 0x1C instead of 0x78 — the 0x5C-byte mismatch lets
// the thunk's POP EDI/ESI/EBX read into the arg zone (Impact's float bytes),
// trashing the saved register state and crashing on the thunk's final RET.
// Wrapping as `uint32_t[24]` forces MSVC to pass inline.
struct FImpactInfoBytes { uint32_t dw[24]; };
static_assert(sizeof(FImpactInfoBytes) == 0x60, "FImpactInfoBytes must be 96 bytes");

class TgEffect__TrackStats : public HookBase<
	void(__fastcall*)(UTgEffect*, void*,
	                  ATgPawn*, AActor*, FImpactInfoBytes,
	                  float, int, unsigned long, float),
	0x10a6f2d0,
	TgEffect__TrackStats> {
public:
	static void __fastcall Call(UTgEffect* Effect, void* edx,
	                            ATgPawn* Instigator, AActor* Target, FImpactInfoBytes Impact,
	                            float fDamage, int iTargetDeviceModeId,
	                            unsigned long bIsEnemy, float fMissingHealth);
};
