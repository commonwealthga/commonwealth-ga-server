#pragma once

#include "src/Utils/HookBase.hpp"

class FString__CreateFromWCharT : public HookBase<
	void*(__fastcall*)(void*, void*, void*),
	0x109eba80,
	FString__CreateFromWCharT> {
public:
	static void* Call(void* param1, void* param2, void* param3);
	static inline void* CallOriginal(void* param1, void* param2, void* param3) {
		return m_original(param1, param2, param3);
	}
};

