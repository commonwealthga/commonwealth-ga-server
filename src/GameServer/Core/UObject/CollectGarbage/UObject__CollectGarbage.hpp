#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class UObject__CollectGarbage : public HookBase<
	void(__cdecl*)(void*, uint32_t, void*),
	0x11325dc0,
	UObject__CollectGarbage> {
public:
	static bool bDisableGarbageCollection;
	static void __cdecl Call(void* param_1, uint32_t param_2, void* param_3);
	static inline void __fastcall CallOriginal(void* param_1, uint32_t param_2, void* param_3) {
		m_original(param_1, param_2, param_3);
	};
};

