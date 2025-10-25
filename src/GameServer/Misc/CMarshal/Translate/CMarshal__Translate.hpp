#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class CMarshal__Translate : public HookBase<
	wchar_t*(__fastcall*)(void*, void*, int, void*),
	0x1093adb0,
	CMarshal__Translate> {
public:
	static std::map<int, char[4096]> m_values;
	static wchar_t* __fastcall Call(void* CMarshal, void* edx, uint32_t a2, void* a3);
	static inline wchar_t* __fastcall CallOriginal(void* CMarshal, void* edx, uint32_t a2, void* a3) {
		return m_original(CMarshal, edx, a2, a3);
	};
};

