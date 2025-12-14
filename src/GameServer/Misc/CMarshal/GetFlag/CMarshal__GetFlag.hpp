#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class CMarshal__GetFlag : public HookBase<
	int(__fastcall*)(void*, void*, int, unsigned char*),
	0x10938050,
	CMarshal__GetFlag> {
public:
	static std::map<int, unsigned char> m_values;
	static bool bLogEnabled;
	static int __fastcall Call(void* CMarshal, void* edx, int Field, unsigned char* Out);
	static inline int __fastcall CallOriginal(void* CMarshal, void* edx, int Field, unsigned char* Out) {
		return m_original(CMarshal, edx, Field, Out);
	};
};

