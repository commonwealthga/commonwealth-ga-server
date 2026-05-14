#pragma once

#include "src/Utils/HookBase.hpp"

// CMarshal::set_float @ 0x10938380.
// Stores a float at the given key on a CMarshal.
class CMarshal__SetFloat : public HookBase<
	int(__fastcall*)(void*, void*, int, float),
	0x10938380,
	CMarshal__SetFloat> {
public:
	static int __fastcall Call(void* Marshal, void* edx, int Key, float Value);
	static inline int __fastcall CallOriginal(void* Marshal, void* edx, int Key, float Value) {
		return m_original(Marshal, edx, Key, Value);
	};
};
