#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// Diagnostics: the engine's assertion reporter (appFailAssert, e.g. the TArray
// bounds check "Array.h line 560") goes straight to appError, and the crash
// handler's EBP walk is useless under FPO. This logs a raw stack scan of
// return-address-looking values (GlobalAgenda.exe .text + our DLL) on the
// "assert" channel first, then calls the original. Fatal path only.
class AssertTrace : public HookBase<
	void(__cdecl*)(void*, void*, int, void*),
	0x112b1890,
	AssertTrace> {
public:
	static void __cdecl Call(void* a, void* b, int line, void* d);
	// Last actor handed to ActorChannel::ReplicateActor (named in the log).
	static inline void* s_lastRepActor = nullptr;
	// First-chance access-violation logger (first 3 AVs, "assert" channel).
	static void InstallFirstChance();
};
