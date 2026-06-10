#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// native function TgPawn SpawnBot(Vector vLocation);
// STUB in the binary (returns 0). Called by UC TgProj_Bot.SpawnTheBot when a
// thrown bot-grenade settles (HitWall with bBounce=false) — e.g. Spider
// Grenades. Spawns the projectile-configured bot as a pet of the thrower.
class TgProj_Bot__SpawnBot : public HookBase<
	ATgPawn*(__fastcall*)(ATgProj_Bot*, void*, FVector),
	0x10a19c10,
	TgProj_Bot__SpawnBot> {
public:
	static ATgPawn* __fastcall Call(ATgProj_Bot* pThis, void* edx, FVector vLocation);
	static inline ATgPawn* __fastcall CallOriginal(ATgProj_Bot* pThis, void* edx, FVector vLocation) {
		return m_original(pThis, edx, vLocation);
	};
};
