#pragma once

#include "src/Utils/HookBase.hpp"

class World__BeginPlay : public HookBase<
	void(__fastcall*)(void*, void*, void*, int),
	0x10bf4400,
	World__BeginPlay> {
public:
	static void __fastcall Call(void* World, void* edx, void* Url, int bResetTime);
	static inline void __fastcall CallOriginal(void* World, void* edx, void* Url, int bResetTime) {
		m_original(World, edx, Url, bResetTime);
	}
};

