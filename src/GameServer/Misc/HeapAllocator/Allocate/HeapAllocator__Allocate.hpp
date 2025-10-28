#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class HeapAllocator__Allocate : public HookBase<
	void*(__cdecl*)(SIZE_T),
	0x10934100,
	HeapAllocator__Allocate> {
public:
	static void* __cdecl Call(SIZE_T Size);
	static inline void* __cdecl CallOriginal(SIZE_T Size) {
		return m_original(Size);
	};
};

