#pragma once

#include "src/Utils/HookBase.hpp"

class FString__CreateFromWCharT : public HookBase<
	void*(__fastcall*)(void*, void*, short*),
	0x109eba80,
	FString__CreateFromWCharT> {
public:
	static void* __fastcall Call(void* param1, void* param2, short* param3);
	static inline void* __fastcall CallOriginal(void* param1, void* param2, short* param3) {
		return m_original(param1, param2, param3);
	}
};

