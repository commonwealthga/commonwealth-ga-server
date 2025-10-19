#pragma once

#include "src/Utils/HookBase.hpp"

class PackageMapLevel__Create : public HookBase<
	void*(__cdecl*)(int32_t, void*, int32_t, int32_t, int32_t, int32_t),
	0x10C198F0, // level
	// 0x10C19950, // seekfree
	PackageMapLevel__Create> {
public:
	static void* Call(int32_t param1, void* param2, int32_t param3, int32_t param4, int32_t param5, int32_t param6);
	static inline void* CallOriginal(int32_t param1, void* param2, int32_t param3, int32_t param4, int32_t param5, int32_t param6) {
		return m_original(param1, param2, param3, param4, param5, param6);
	}
};

