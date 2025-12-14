#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class CMarshal__GetIntEnum : public HookBase<
	int(__fastcall*)(void*, void*, int, int*),
	0x109382e0,
	CMarshal__GetIntEnum> {
public:
	static std::map<int, int> m_values;
	static bool bLogEnabled;
	static int __fastcall Call(void* CMarshal, void* edx, int Field, int* Out);
	static inline int __fastcall CallOriginal(void* CMarshal, void* edx, int Field, int* Out) {
		return m_original(CMarshal, edx, Field, Out);
	};
};

