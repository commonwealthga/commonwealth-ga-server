#pragma once

#include "src/Utils/HookBase.hpp"

class CMarshal__GetString2 : public HookBase<
	int(__fastcall*)(void*, void*, uint32_t, wchar_t*, uint32_t*),
	0x10937f10,
	CMarshal__GetString2> {
public:
	static std::map<int, char[4096]> m_values;
	static int __fastcall Call(void* CMarshal, void* edx, uint32_t a2, wchar_t* a3, uint32_t* a4);
	static inline int __fastcall CallOriginal(void* CMarshal, void* edx, uint32_t a2, wchar_t* a3, uint32_t* a4) {
		return m_original(CMarshal, edx, a2, a3, a4);
	};
};

