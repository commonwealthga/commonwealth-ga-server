#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// UTgDeviceFire::CheckTeamPassThrough — vtable slot 102 of UTgDeviceFire
// (vtable @ 0x116f03b8 + 0x198 = 0x116f0550 → function 0x10a28110).
//
// Called from `UTgDeviceFire::GetTraceImpact` (0x10a1d180) for every hit
// returned by `UWorld::MultiLineCheck`.  Decides whether to "pass through"
// this hit (continue trace to the next actor) or "register the hit" (stop
// here).
//
// Decompile semantics:
//   - For non-Pawn / non-ForceField / non-TeamBlocker hit actors (i.e. plain
//     TgDeployable like medstation/power station): returns FALSE — "register
//     the hit, don't pass through".  This means the hit IS reported back to
//     GetTraceImpact's caller, then `IsValidTarget` runs as a second filter.
//   - For Pawn hit actors: more nuanced (m_eTargeterType + IsEnemy + range).
//   - For ForceField/TeamBlocker: bit-flag check + IsEnemy.
//
// Diagnostic-only hook.  Logs every call where the hit actor is a
// TgDeployable, so we can verify the trace actually reaches our deployables
// and what CheckTeamPassThrough decides.  Channel: `combat-trace`.
class TgDeviceFire__CheckTeamPassThrough : public HookBase<
	bool(__fastcall*)(UTgDeviceFire*, void*, AActor*),
	0x10a28110,
	TgDeviceFire__CheckTeamPassThrough> {
public:
	static bool __fastcall Call(UTgDeviceFire* FireMode, void* edx, AActor* HitActor);
	static inline bool __fastcall CallOriginal(UTgDeviceFire* FireMode, void* edx, AActor* HitActor) {
		return m_original(FireMode, edx, HitActor);
	}
};
