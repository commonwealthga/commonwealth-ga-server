#pragma once

#include "src/Utils/HookBase.hpp"

class PackageMapLevel__CanSerializeObject : public HookBase<
	bool(__fastcall*)(void*, void*, int),
	0x10C16960,
	PackageMapLevel__CanSerializeObject> {
public:
	static bool __fastcall Call(void* param_1, void* param_2, int param_3);
	static inline bool __fastcall CallOriginal(void* param_1, void* param_2, int param_3) {
		return m_original(param_1, param_2, param_3);
	}
};

