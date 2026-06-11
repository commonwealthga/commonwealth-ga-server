#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// ⚠️ DECOMMISSIONED 2026-06-11 — do NOT re-install. The native @ 0x109cc520 is
// INTACT and now runs unhooked against REAL UTgSocketOffsetInfo assets
// (FireSockets::EnsurePopulated replaces the self-pointer sentinel this hook
// depended on; re-installing on top of real assets would discard the retail
// aim-interpolated socket positions, and the original native would crash if
// the sentinel ever returned). Removed from Makefile + dllmain. See
// .planning/2026-06-11-fire-sockets-investigation.md.
//
// TgPawn::GetWeaponStartTraceLocationFromSocketOffsetInfo — native @ 0x109cc520.
//
// MSVC return-struct-by-pointer convention:
//   void __thiscall(ATgPawn* this, FVector* outLoc, ATgDevice* Dev)
// outLoc is the return-value out-pointer for `Vector` UC return.
//
// UC TgPawn_Boss.GetWeaponStartTraceLocation (and the Turret/Hover/Robot/Siege
// overrides) routes through this native ONLY when:
//   - NetMode == NM_DedicatedServer (always true for us), AND
//   - this->m_TgSocketOffsetInfo != none (always FALSE on our build, because
//     the populator is stripped — m_TgSocketOffsetInfo is never written by
//     any binary code path). So this branch is dead on our server.
//
// We bring it back to life by:
//   1. Setting `pawn->m_TgSocketOffsetInfo = (UTgSocketOffsetInfo*)pawn` (a
//      self-referential sentinel, just to satisfy `!= none`) for any pawn
//      we want to handle. Done in SpawnBotById for Shrike (body_asm_id 794).
//   2. Replacing this native ENTIRELY (no CallOriginal) — the original
//      derefs m_TgSocketOffsetInfo at offsets +0x3C/+0x40/+0x4C, which would
//      crash on the sentinel. Our replacement never touches the field.
//
// The native runs INSIDE TgPawn_Boss.GetWeaponStartTraceLocation, BEFORE
// FlashFire and BEFORE UpdateIndex. So `Dev->m_nSocketIndex` here is the
// CURRENT shot's slot (not the pre-incremented next-shot value, unlike at
// InitializeProjectile time). Mapping: slot 1 → vec[0] = WSO_Origin_01 =
// bone Left → -Y; slot 2 → +Y. NATURAL mapping (no inversion).
//
// AI prediction (GetAdjustedAim, lead computation) downstream uses our
// returned StartTrace as the firing position — so leading is computed from
// the shoulder, not the body. Correct fix at the trace layer.
// CRITICAL calling convention note:
//   The original is `__thiscall` returning `Vector` by hidden out-pointer (MSVC's
//   struct-return convention). Disassembly at 0x109cc7ac confirms the function
//   loads `[EBP+0x8]` (the outparam) into EAX before RET — i.e. **EAX must equal
//   the outparam pointer at exit**. The SDK thunk that calls this via vtable
//   does `puVar2 = call(...); *puStack_8 = *puVar2`, dereferencing whatever EAX
//   holds. A void hook leaks junk in EAX → null deref → crash on bot spawn.
//   Therefore the hook MUST be declared as returning FVector* and `return outLoc;`.
class TgPawn__GetWeaponStartTraceLocationFromSocketOffsetInfo : public HookBase<
	FVector* (__fastcall*)(ATgPawn*, void*, FVector*, ATgDevice*),
	0x109cc520,
	TgPawn__GetWeaponStartTraceLocationFromSocketOffsetInfo> {
public:
	static FVector* __fastcall Call(ATgPawn* Pawn, void* edx, FVector* outLoc, ATgDevice* Dev);
	// NB: deliberately NO CallOriginal. The original derefs m_TgSocketOffsetInfo
	// internals which we've stubbed with a self-pointer sentinel; calling
	// original would crash. Our hook is the COMPLETE implementation.
};
