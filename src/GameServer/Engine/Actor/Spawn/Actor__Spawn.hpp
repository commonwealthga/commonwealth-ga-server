#pragma once

#include "src/Utils/HookBase.hpp"

class Actor__Spawn : public HookBase<
	AActor*(__fastcall*)(UWorld*, void*, UClass*, uint64_t, FVector*, FRotator*, AActor*, bool, bool, AActor*, bool, ULevel*),
	0x10CC1510,
	Actor__Spawn> {
public:
	static AActor* __fastcall Call(UWorld* pThis, void* edx, UClass* Class, uint64_t InNameStub, FVector* Location, FRotator* Rotation, AActor* Template, bool bNoCollisionFail, bool bRemoteOwned, AActor* Owner, bool bNoFail, ULevel* OverrideLevel);
	static inline AActor* __fastcall CallOriginal(UWorld* pThis, void* edx, UClass* Class, uint64_t InNameStub, FVector* Location, FRotator* Rotation, AActor* Template, bool bNoCollisionFail, bool bRemoteOwned, AActor* Owner, bool bNoFail, ULevel* OverrideLevel) {
		return m_original(pThis, edx, Class, InNameStub, Location, Rotation, Template, bNoCollisionFail, bRemoteOwned, Owner, bNoFail, OverrideLevel);
	}
};

