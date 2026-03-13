#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class CMarshal__GetGuid : public HookBase<
	int(__fastcall*)(void*, void*, int, UUID*),
	0x109381e0,
	CMarshal__GetGuid> {
public:
	static int __fastcall Call(void* CMarshal, void* edx, int Field, UUID* Out);
	static inline int __fastcall CallOriginal(void* CMarshal, void* edx, int Field, UUID* Out) {
		return m_original(CMarshal, edx, Field, Out);
	};
};

