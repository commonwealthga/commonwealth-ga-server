#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// Hooks the C++ virtual Deploy (0x10a19960) — server-side stub (void, no-op).
// Called by CustomFire() for deployable attack types:
//   TGTT_ATTACK_PLACE_DEPLOYABLE (209), TGTT_ATTACK_INSTANT_DEPLOYABLE (342),
//   TGTT_ATTACK_SELF_DEPLOYABLE (876).
// native function Deploy();
class TgDeviceFire__Deploy : public HookBase<
	void(__fastcall*)(UTgDeviceFire*, void*),
	0x10a19960,
	TgDeviceFire__Deploy> {
public:
	static void __fastcall Call(UTgDeviceFire* pThis, void* edx);
};
