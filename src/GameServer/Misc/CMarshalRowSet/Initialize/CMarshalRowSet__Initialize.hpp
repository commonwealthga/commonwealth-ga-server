#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class CMarshalRowSet__Initialize : public HookBase<
	void*(__fastcall*)(void*),
	0x10935530,
	CMarshalRowSet__Initialize> {
public:
	static void* __fastcall Call(void* CMarshal);
	static inline void* __fastcall CallOriginal(void* CMarshal) {
		return m_original(CMarshal);
	};
};


