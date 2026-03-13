#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// Hooks the C++ virtual SpawnPet (0x10a19950) — server-side stub (void, no-op).
// Called by CustomFire() for pet/bot attack types:
//   TGTT_ATTACK_SPAWN_PET (306)  → bPet=true  (controllable companion)
//   TGTT_ATTACK_INSTANT_BOT (817) → bPet=false (decoy bot)
// native function SpawnPet(bool bPet);
class TgDeviceFire__SpawnPet : public HookBase<
	void(__fastcall*)(UTgDeviceFire*, void*, BOOL),
	0x10a19950,
	TgDeviceFire__SpawnPet> {
public:
	static void __fastcall Call(UTgDeviceFire* pThis, void* edx, BOOL bPet);
};
