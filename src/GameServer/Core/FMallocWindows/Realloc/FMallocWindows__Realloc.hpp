#pragma once

#include "src/Utils/HookBase.hpp"

// FMallocWindows::Realloc — vtable slot 2 on the FMallocWindows instance whose
// vtable lives at 0x1168927c. Direct binary entry point at 0x1090b880.
//
// Signature (decompiled from binary):
//   void* __thiscall Realloc(this, void* Ptr, DWORD NewSize, DWORD Alignment)
//
// Branches:
//   Ptr == NULL              → delegates to vtable[1] (Malloc)
//   Ptr != NULL, NewSize==0  → delegates to vtable[3] (Free), returns NULL
//   Ptr != NULL, NewSize>0   → resize-in-place if pool bucket fits, else
//                              Malloc + memcpy + Free
//
// Same Alignment==DEFAULT_ALIGNMENT (=8) assertion as Malloc
// (FMallocWindows.h:0xfc).
//
// **Not installed by default.** Available for direct CallOriginal invocation.
class FMallocWindows__Realloc : public HookBase<
	void*(__fastcall*)(void* /*this*/, void* /*edx*/, void* /*Ptr*/, unsigned int /*NewSize*/, int /*Alignment*/),
	0x1090b880,
	FMallocWindows__Realloc> {
public:
	static void* __fastcall Call(void* This, void* edx, void* Ptr,
	                             unsigned int NewSize, int Alignment);
	static inline void* __fastcall CallOriginal(void* This, void* edx, void* Ptr,
	                                            unsigned int NewSize, int Alignment) {
		return m_original(This, edx, Ptr, NewSize, Alignment);
	}
};
