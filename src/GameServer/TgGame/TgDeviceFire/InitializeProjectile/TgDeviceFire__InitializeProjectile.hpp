#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// UTgDeviceFire::InitializeProjectile — native @ 0x10a25310.
//
// UC declaration:
//   native function InitializeProjectile(Projectile Proj);
//
// Original applies the per-device-fire-mode asm.dat setup to the spawned
// projectile (vtable[0x378] on the projectile, passing m_pAmSetup). Does NOT
// touch Instigator/Owner/InstigatorController — those are supposed to come
// from UC's Spawn() iterator at TgDevice.uc:1839 via inheritance from the
// firing device's Instigator.
//
// PROBLEM (suspected): some projectiles are observed on the client with
// enemy team color and (server-side) ignore enemy hits. Both symptoms are
// consistent with `Projectile.Instigator == None` reaching gameplay code.
//
// This hook: (1) calls the original setup, (2) logs server-side Instigator
// state on first few firings for diagnosis, (3) defensively repairs
// Projectile.Instigator from device->Instigator if it's null, and bumps
// bNetDirty/bForceNetUpdate so the corrected Instigator gets replicated.
//
// __thiscall: ECX = this (UTgDeviceFire*). Arg via stack: Projectile*.
class TgDeviceFire__InitializeProjectile : public HookBase<
	void(__fastcall*)(UTgDeviceFire*, void*, AActor*),
	0x10a25310,
	TgDeviceFire__InitializeProjectile> {
public:
	static void __fastcall Call(UTgDeviceFire* DeviceFire, void* edx, AActor* Projectile);
	static inline void __fastcall CallOriginal(UTgDeviceFire* DeviceFire, void* edx, AActor* Projectile) {
		m_original(DeviceFire, edx, Projectile);
	}
};
