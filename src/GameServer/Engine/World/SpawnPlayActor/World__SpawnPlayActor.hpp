#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class World__SpawnPlayActor : public HookBase<
	ATgPlayerController*(__fastcall*)(void*, void*, ULocalPlayer*, int, FURL*, FString*, int),
	0x10CBC8C0,
	World__SpawnPlayActor> {
public:
	static ATgPlayerController* __fastcall Call(UWorld* World, void* edx, ULocalPlayer* Connection, int RemoteRole, FURL* RequestURL, FString* Error, int param_7);
	static inline ATgPlayerController* __fastcall CallOriginal(UWorld* World, void* edx, ULocalPlayer* Connection, int RemoteRole, FURL* RequestURL, FString* Error, int param_7) {
		return m_original(World, edx, Connection, RemoteRole, RequestURL, Error, param_7);
	}
};

