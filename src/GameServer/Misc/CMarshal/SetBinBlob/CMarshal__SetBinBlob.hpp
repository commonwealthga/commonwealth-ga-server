#pragma once

#include <cstddef>
#include "src/Utils/HookBase.hpp"

// CMarshal::set_bin_blob @ 0x10938500.
// Stores a binary blob at the given integer key on a CMarshal.
class CMarshal__SetBinBlob : public HookBase<
	void*(__fastcall*)(void*, void*, int, const void*, std::size_t),
	0x10938500,
	CMarshal__SetBinBlob> {
public:
	static void* __fastcall Call(void* Marshal, void* edx, int Key, const void* Data, std::size_t Size);
	static inline void* __fastcall CallOriginal(void* Marshal, void* edx, int Key, const void* Data, std::size_t Size) {
		return m_original(Marshal, edx, Key, Data, Size);
	};
};
