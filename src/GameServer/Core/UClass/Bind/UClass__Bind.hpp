#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// UClass::Bind at 0x112f4f00 (UnClass.cpp). The binary asserts
// `GIsEditor || GetSuperClass() || this==UObject::StaticClass()` and crashes
// without naming the class. This hook re-checks that condition on entry and
// shows the failing class's full name in a message box before the crash.
class UClass__Bind : public HookBase<
	void(__fastcall*)(UClass*, void*),
	0x112F4F00,
	UClass__Bind> {
public:
	static void __fastcall Call(UClass* Class, void* edx);
	static inline void __fastcall CallOriginal(UClass* Class, void* edx) {
		m_original(Class, edx);
	};
};
