#pragma once

#include "src/Utils/HookBase.hpp"

// CMarshal::set_bool @ 0x10938440.
// Stores a boolean/flag at the given key on a CMarshal. Internally allocates
// a flag cell and writes the byte value (0/1).
class CMarshal__SetBool : public HookBase<
	int(__fastcall*)(void*, void*, int, int),
	0x10938440,
	CMarshal__SetBool> {
public:
	static int __fastcall Call(void* Marshal, void* edx, int Key, int Value);
	static inline int __fastcall CallOriginal(void* Marshal, void* edx, int Key, int Value) {
		return m_original(Marshal, edx, Key, Value);
	};
};
