#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// Hooks the C++ virtual CustomFire (0x10a19930) — server-side stub (void, no-op).
// Called by TgDevice::CustomFire() when Role==Authority and m_nFireType==2.
// Dispatches to Deploy() or SpawnPet() based on m_nAttackType.
// native function CustomFire();
class TgDeviceFire__CustomFire : public HookBase<
	void(__fastcall*)(UTgDeviceFire*, void*),
	0x10a19930,
	TgDeviceFire__CustomFire> {
public:
	static void __fastcall Call(UTgDeviceFire* pThis, void* edx);
};
