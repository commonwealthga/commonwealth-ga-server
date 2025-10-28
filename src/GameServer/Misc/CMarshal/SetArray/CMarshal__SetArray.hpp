#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class CMarshal__SetArray : public HookBase<
	int(__fastcall*)(void*, void*, int, void*),
	0x10938590,
	CMarshal__SetArray> {
public:
	static int __fastcall Call(void* CMarshal, void* edx, int Field, void* Data);
	static inline int __fastcall CallOriginal(void* CMarshal, void* edx, int Field, void* Data) {
		return m_original(CMarshal, edx, Field, Data);
	};
};

