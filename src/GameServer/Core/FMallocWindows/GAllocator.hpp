#pragma once

#include <cstddef>

// UE3 global allocator pointer — at 0x134237e8, with 13K+ references throughout
// the binary. Initialized by `InitGAllocatorInstance` (@ 0x10905710) which:
//   1. malloc(0x202b0) for an FMallocWindows
//   2. construct it via FUN_1090b1e0 (writes vtable 0x1168927c at offset 0)
//   3. if ValidateHeap() returns 0 (= not natively thread-safe), wrap with a
//      FMallocThreadSafeProxy (vtable 0x116897d4) that holds the FMallocWindows
//      at +0x4 and a CRITICAL_SECTION at +0xC
//
// Either way `*GAllocatorInstance` is an FMalloc-derived object whose vtable
// follows the standard layout: slot 1=Malloc, slot 2=Realloc, slot 3=Free.
// Routing TArray::Add / Clear through this pointer guarantees the engine's
// UProperty TArray destructor (which calls vtable[3] Free on Data) sees a
// pointer it knows how to release — eliminating the libc↔GMalloc cross-
// allocator crash documented in
// `src/SDK/SdkHeaders.h::TArray::Add`.
namespace GAllocator {

	inline void** const InstanceAddr = (void**)0x134237e8;

	// Raw access to the current FMalloc-derived instance pointer. Stable
	// after engine init; we never need to refresh it.
	inline void* Get() { return *InstanceAddr; }

	// __thiscall vtable invocation. MinGW lays out __thiscall identically to
	// __fastcall except it doesn't claim EDX — passing nullptr for the edx
	// slot is harmless (the native ignores it). Mirrors how Free's HookBase
	// declares its signature.
	inline void* Malloc(std::size_t Size, int Alignment = 8) {
		void*  inst = *InstanceAddr;
		void** vtbl = *(void***)inst;
		typedef void* (__fastcall* Fn)(void* /*this*/, void* /*edx*/,
		                               unsigned int /*Size*/, int /*Alignment*/);
		return ((Fn)vtbl[1])(inst, nullptr, (unsigned int)Size, Alignment);
	}

	inline void* Realloc(void* Ptr, std::size_t NewSize, int Alignment = 8) {
		void*  inst = *InstanceAddr;
		void** vtbl = *(void***)inst;
		typedef void* (__fastcall* Fn)(void* /*this*/, void* /*edx*/, void* /*Ptr*/,
		                               unsigned int /*NewSize*/, int /*Alignment*/);
		return ((Fn)vtbl[2])(inst, nullptr, Ptr, (unsigned int)NewSize, Alignment);
	}

	inline void Free(void* Ptr) {
		if (!Ptr) return;
		void*  inst = *InstanceAddr;
		void** vtbl = *(void***)inst;
		typedef void (__fastcall* Fn)(void* /*this*/, void* /*edx*/, void* /*Ptr*/);
		((Fn)vtbl[3])(inst, nullptr, Ptr);
	}

}  // namespace GAllocator
