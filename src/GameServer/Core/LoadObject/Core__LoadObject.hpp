#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// UE3 global UObject::StaticLoadObject wrapper at 0x112e4960.
// Signature (from decompile at 0x112e4960):
//   void* __cdecl LoadObject(
//     void*    ObjectClass,      // UClass* of what to load
//     wchar_t* InOuter,          // optional outer (nullptr common)
//     wchar_t* InName,           // name/path string
//     wchar_t* InFilename,       // optional package file (nullptr common)
//     unsigned LoadFlags,
//     int*     Sandbox,
//     int      IsVerifyImport);
//
// Returns null when the package/object can't be resolved — which causes
// the deliberate CrashProgram("newObj", ...) in TgAssemblyMisc.cpp:253.
//
// This hook logs only NULL returns — i.e. every failed load — to keep the
// log volume low (LoadObject is called ubiquitously across the engine).
class Core__LoadObject : public HookBase<
	void*(__cdecl*)(void*, wchar_t*, wchar_t*, wchar_t*, unsigned, int*, int),
	0x112e4960,
	Core__LoadObject> {
public:
	static void* __cdecl Call(void* ObjectClass, wchar_t* InOuter, wchar_t* InName,
		wchar_t* InFilename, unsigned LoadFlags, int* Sandbox, int IsVerifyImport);
	static inline void* __cdecl CallOriginal(void* ObjectClass, wchar_t* InOuter, wchar_t* InName,
		wchar_t* InFilename, unsigned LoadFlags, int* Sandbox, int IsVerifyImport) {
		return m_original(ObjectClass, InOuter, InName, InFilename, LoadFlags, Sandbox, IsVerifyImport);
	}
};
