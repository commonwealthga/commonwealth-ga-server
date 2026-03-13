#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// Hooks the C++ virtual GetEffectGroup (0x10a19970).
// Ghidra shows (param_1, param_2*) with no explicit 'this' because the stub never reads ECX,
// but vtable dispatch is __thiscall. The dummy EDX slot is required for correct stack cleanup.
// native function TgEffectGroup GetEffectGroup(int nType, out int nIndex);
class TgDeviceFire__GetEffectGroup : public HookBase<
	UTgEffectGroup*(__fastcall*)(UTgDeviceFire*, void*, int, int*),
	0x10a19970,
	TgDeviceFire__GetEffectGroup> {
public:
	static UTgEffectGroup* __fastcall Call(UTgDeviceFire* pThis, void* edx, int nType, int* nIndex);
};
