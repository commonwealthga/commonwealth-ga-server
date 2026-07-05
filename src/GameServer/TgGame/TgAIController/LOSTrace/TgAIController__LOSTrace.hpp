#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// Hook on the recursive LOS trace (FUN_10a806e0, __thiscall).
// Called from every AI LOS path: TargetInLOS (fire maintenance),
// FindBestEnemy (target acquisition), LineOfSightTo UC native, etc.
// ForceField domes are disabled at the top-level entry so the trace sees
// through all of them. Depth tracking prevents re-disabling on recursive
// self-calls (which all pass through this hook too).
class TgAIController__LOSTrace : public HookBase<
	int(__fastcall*)(int*, void*, int, int, int, int, int, int, int, int, int),
	0x10a806e0,
	TgAIController__LOSTrace> {
public:
	static int __fastcall Call(int* param_1, void* edx,
		int p2, int p3, int p4, int p5, int p6, int p7, int p8, int p9, int p10);
	static inline int __fastcall CallOriginal(int* param_1, void* edx,
		int p2, int p3, int p4, int p5, int p6, int p7, int p8, int p9, int p10) {
		return m_original(param_1, edx, p2, p3, p4, p5, p6, p7, p8, p9, p10);
	}
};
