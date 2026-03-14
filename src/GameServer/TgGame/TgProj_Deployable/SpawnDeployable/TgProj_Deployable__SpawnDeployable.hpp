#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgProj_Deployable__SpawnDeployable : public HookBase<
	ATgDeployable*(__fastcall*)(ATgProj_Deployable*, void*, FVector, AActor*, FVector),
	0x10a19c40,
	TgProj_Deployable__SpawnDeployable> {
public:
	static ATgDeployable* __fastcall Call(ATgProj_Deployable* pThis, void* edx, FVector vLocation, AActor* TargetActor, FVector vNormal);
	static inline ATgDeployable* __fastcall CallOriginal(ATgProj_Deployable* pThis, void* edx, FVector vLocation, AActor* TargetActor, FVector vNormal) {
		return m_original(pThis, edx, vLocation, TargetActor, vNormal);
	};

	// Shared helper used by both SpawnDeployable (projectile path) and TgDeviceFire::Deploy.
	static ATgDeployable* SpawnDeployableActor(ATgPawn* pawn, int deployableId, FVector vLocation, FVector vNormal);
};
