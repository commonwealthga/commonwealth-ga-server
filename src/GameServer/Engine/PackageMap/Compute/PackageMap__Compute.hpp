#pragma once

#include "src/Utils/HookBase.hpp"

class PackageMap__Compute : public HookBase<
	void(__fastcall*)(void*),
	0x11366000,
	PackageMap__Compute> {
public:
	static void __fastcall Call(void* PackageMap);
	static inline void __fastcall CallOriginal(void* PackageMap) {
		return m_original(PackageMap);
	}
};

