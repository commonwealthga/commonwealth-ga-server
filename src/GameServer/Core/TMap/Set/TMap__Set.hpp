#pragma once

#include "src/pch.hpp"

// Wrapper for the binary's TMap::Set helper at 0x109bdd50:
//
//   int* __thiscall TMap_Set(TMap* this, uint key, uint value);
//
// Sets the (key → value) pair, replacing the value if the key already
// exists. Auto-bootstraps the TMap on first call (TSparseArray + hash table
// allocate themselves on the first insert). Returns a pointer to the value
// field as stored in the pair.
//
// We don't subclass HookBase because we never want to *intercept* this — we
// just need to invoke it with the right thiscall convention. Direct fastcall
// thunk with a dummy edx argument is the canonical pattern in this codebase
// for `__thiscall` natives.
namespace TMap_Set {
	typedef int* (__fastcall *Fn)(void* /*this*/, void* /*edx*/, unsigned int /*key*/, unsigned int /*value*/);
	inline int* Call(void* map, unsigned int key, unsigned int value) {
		return ((Fn)0x109bdd50)(map, nullptr, key, value);
	}
}
