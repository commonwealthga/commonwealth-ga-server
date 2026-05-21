#pragma once

#include "src/Utils/HookBase.hpp"

// FMallocWindows::Malloc — vtable slot 1 on the FMallocWindows instance whose
// vtable lives at 0x1168927c. Direct binary entry point at 0x1090dc70.
//
// Signature (decompiled from binary):
//   void* __thiscall Malloc(this, DWORD Size, DWORD Alignment)
//
// On this build the only supported Alignment is DEFAULT_ALIGNMENT (=8); the
// function asserts otherwise (FMallocWindows.h:0x99). Size > 0x8000 falls
// through to VirtualAlloc; smaller goes through the pool bucket at
// `this + 0x288 + Size*4`.
//
// HookBase pattern with __fastcall declaration mirrors how Free is wrapped —
// __thiscall and __fastcall agree on ECX=this, with __fastcall additionally
// claiming EDX which the native ignores (we pass nullptr through it).
//
// **Not installed by default.** This class exists so callers can invoke the
// native via `FMallocWindows__Malloc::CallOriginal(...)` without redefining
// the address+signature themselves. If you ever need to intercept, call
// `Install()` from dllmain.
class FMallocWindows__Malloc : public HookBase<
	void*(__fastcall*)(void* /*this*/, void* /*edx*/, unsigned int /*Size*/, int /*Alignment*/),
	0x1090dc70,
	FMallocWindows__Malloc> {
public:
	static void* __fastcall Call(void* This, void* edx, unsigned int Size, int Alignment);
	static inline void* __fastcall CallOriginal(void* This, void* edx, unsigned int Size, int Alignment) {
		return m_original(This, edx, Size, Alignment);
	}
};
