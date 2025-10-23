#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class World__GetWorldInfo : public HookBase<
	AWorldInfo*(__fastcall*)(UWorld*, void*, int),
	0x10bef8c0,
	World__GetWorldInfo> {
public:
	static AWorldInfo* __fastcall Call(UWorld* World, void* edx, int Param1);
	static inline AWorldInfo* __fastcall CallOriginal(UWorld* World, void* edx, int Param1) {
		return m_original(World, edx, Param1);
	}
};

