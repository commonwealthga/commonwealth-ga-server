#pragma once

#include "src/Utils/HookBase.hpp"

class FString__CreateFromWCharT : public HookBase<
	void*(__fastcall*)(void*, void*, void*),
	0x109eba80,
	FString__CreateFromWCharT> {
public:
	static void* __fastcall Call(void* param1, void* param2, void* param3);
	static inline void* __fastcall CallOriginal(void* param1, void* param2, void* param3) {
		LogCallOriginalBegin();
		void* result = m_original(param1, param2, param3);
		LogCallOriginalEnd();
		return result;
	}
};

