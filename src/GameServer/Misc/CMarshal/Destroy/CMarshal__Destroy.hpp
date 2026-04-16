#pragma once

#include "src/Utils/HookBase.hpp"

// CMarshal destructor @ 0x10936a00.
// Frees the marshal's internal heap-allocated members (field table, blob buffers, etc).
// Does NOT free the marshal struct itself — caller owns the outer allocation.
class CMarshal__Destroy : public HookBase<
	void(__fastcall*)(void*),
	0x10936a00,
	CMarshal__Destroy> {
public:
	static void __fastcall Call(void* Marshal);
	static inline void __fastcall CallOriginal(void* Marshal) {
		m_original(Marshal);
	};
};
