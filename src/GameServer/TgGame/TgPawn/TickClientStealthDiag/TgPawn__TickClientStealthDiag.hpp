#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// CLIENT-SIDE DIAGNOSTIC — charts the make-visible decay over time. Hooks the
// native TgPawn tick augmentation @ 0x109cb910 (its tail decays
// m_fMakeVisibleCurrent by r_fMakeVisibleFadeRate*dt and clamps to [0,1]).
// Logs m / incr / fadeRate / c_nTickCheckingState per stealth-relevant pawn,
// throttled to ~1 line/s. Install ONLY in dllmainclient.
class TgPawn__TickClientStealthDiag : public HookBase<
	void(__fastcall*)(ATgPawn*, void*, float),
	0x109cb910,
	TgPawn__TickClientStealthDiag> {
public:
	static void __fastcall Call(ATgPawn* Pawn, void* edx, float DeltaTime);
	static inline void __fastcall CallOriginal(ATgPawn* Pawn, void* edx, float DeltaTime) {
		m_original(Pawn, edx, DeltaTime);
	};
};
