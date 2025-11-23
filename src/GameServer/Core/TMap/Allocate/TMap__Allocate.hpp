#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TMap__Allocate : public HookBase<
	void*(__fastcall*)(void*),
	0x10ea6290,
	TMap__Allocate> {
public:
	static void* __fastcall Call(void* Pointer);
	static inline void* __fastcall CallOriginal(void* Pointer) {
		LogCallOriginalBegin();
		void* result = m_original(Pointer);
		LogCallOriginalEnd();
		return result;
	};
};

