#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class CMarshal__GetFloat : public HookBase<
	int(__fastcall*)(void*, void*, int, float*),
	0x10937f50,
	CMarshal__GetFloat> {
public:
	static std::map<int, float> m_values;
	static int __fastcall Call(void* CMarshal, void* edx, int Field, float* Out);
	static inline int __fastcall CallOriginal(void* CMarshal, void* edx, int Field, float* Out) {
		return m_original(CMarshal, edx, Field, Out);
	};
};

