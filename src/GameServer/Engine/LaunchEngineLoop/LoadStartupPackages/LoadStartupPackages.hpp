#pragma once

#include "src/Utils/HookBase.hpp"

class LoadStartupPackages : public HookBase<
	void(__stdcall*)(),
	0x10907A30,
	LoadStartupPackages> {
public:
    static void __stdcall Call();
	static inline void __stdcall CallOriginal() {
		return m_original();
	}
};

