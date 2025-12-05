#pragma once

#include "src/Utils/HookBase.hpp"

class FMallocWindows__Free : public HookBase<
	void(__fastcall*)(void*, void*, void*),
	0x1090b480,
	FMallocWindows__Free> {
public:
	static bool bLogEnabled;
	static void __fastcall Call(void* Malloc, void* edx, void* Ptr);
	static inline void __fastcall CallOriginal(void* Malloc, void* edx, void* Ptr) {
		m_original(Malloc, edx, Ptr);
	}
};

