#pragma once

#include "src/Utils/HookBase.hpp"

class ServerCommandlet__Main : public HookBase<
	int(__stdcall*)(),
	0x10B0D810,
	ServerCommandlet__Main> {
public:
	static int __stdcall Call();
	static inline int __stdcall CallOriginal() {
		return m_original();
	}
};

