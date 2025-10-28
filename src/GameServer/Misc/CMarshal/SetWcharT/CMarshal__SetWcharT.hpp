#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class CMarshal__SetWcharT : public HookBase<
	int(__fastcall*)(void*, void*, int, wchar_t*),
	0x10938350,
	CMarshal__SetWcharT> {
public:
	static int __fastcall Call(void* CMarshal, void* edx, int Field, wchar_t* Data);
	static inline int __fastcall CallOriginal(void* CMarshal, void* edx, int Field, wchar_t* Data) {
		return m_original(CMarshal, edx, Field, Data);
	};
};

