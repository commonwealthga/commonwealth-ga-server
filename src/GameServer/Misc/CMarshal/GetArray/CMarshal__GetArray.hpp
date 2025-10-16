#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class CMarshal__GetArray : public HookBase<
	int(__fastcall*)(void*, void*, int, uint32_t*),
	0x10938090,
	CMarshal__GetArray> {
public:
	static std::map<int, uint32_t> m_values;
	static int __fastcall Call(void* CMarshal, void* edx, int Field, uint32_t* Out);
	static inline int __fastcall CallOriginal(void* CMarshal, void* edx, int Field, uint32_t* Out) {
		return m_original(CMarshal, edx, Field, Out);
	};
};

