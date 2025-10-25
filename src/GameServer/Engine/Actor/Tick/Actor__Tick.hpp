#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class Actor__Tick : public HookBase<
	void*(__fastcall*)(void*, void*, float, int),
	0x10c2ab80,
	Actor__Tick> {
public:
	static void* __fastcall Call(void* a1, void* edx, float a2, int a3);
	static inline void* __fastcall CallOriginal(void* a1, void* edx, float a2, int a3) {
		return m_original(a1, edx, a2, a3);
	};
};

