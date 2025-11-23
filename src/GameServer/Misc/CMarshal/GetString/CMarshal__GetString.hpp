#pragma once

#include "src/Utils/HookBase.hpp"

class CMarshal__GetString : public HookBase<
	int(__fastcall*)(void*, void*, int, wchar_t*),
	0x10937ED0,
	CMarshal__GetString> {
public:
	static int __fastcall Call(void* CMarshal, void* edx, int a2, wchar_t* a3);
	static inline int __fastcall CallOriginal(void* CMarshal, void* edx, int a2, wchar_t* a3) {
		return m_original(CMarshal, edx, a2, a3);
	};
};

