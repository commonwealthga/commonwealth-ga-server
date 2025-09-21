#pragma once

#include "src/Utils/HookBase.hpp"

class PackageMapLevel__Initialize : public HookBase<
	void*(__fastcall*)(void*),
	0x10C19400,
	PackageMapLevel__Initialize> {
public:
	static void* Call(void* param1);
	static inline void* CallOriginal(void* param1) {
		return m_original(param1);
	}
};

