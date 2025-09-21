#pragma once

#include "src/Utils/HookBase.hpp"

class ConstructCommandletObject : public HookBase<
	void(__cdecl*)(void*, int, void*, void*, int, int, void*, void*, void*),
	0x1090BE70,
	ConstructCommandletObject> {
public:
	static void __cdecl Call(void* param_1, int param_2, void* param_3, void* param_4, int param_5, int param_6, void* param_7, void* param_8, void* param_9);
	static inline void __cdecl CallOriginal(void* param_1, int param_2, void* param_3, void* param_4, int param_5, int param_6, void* param_7, void* param_8, void* param_9) {
		return m_original(param_1, param_2, param_3, param_4, param_5, param_6, param_7, param_8, param_9);
	}
};

