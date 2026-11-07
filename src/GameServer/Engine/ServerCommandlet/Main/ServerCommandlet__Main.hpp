#pragma once

#include "src/Utils/HookBase.hpp"

class ServerCommandlet__Main : public HookBase<
	int(__fastcall*)(void*, void*, void*),
	0x10B0D810,
	ServerCommandlet__Main> {
public:
	// UCommandlet::Main(const FString& Params) is __thiscall (original does ret 0x4).
	static int __fastcall Call(void* This, void* edx, void* Params);
	static inline int __fastcall CallOriginal(void* This, void* Params) {
		LogCallOriginalBegin();
		int result = m_original(This, nullptr, Params);
		LogCallOriginalEnd();
		return result;
	}
};

