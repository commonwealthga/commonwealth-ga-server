#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// Game-side sig: __thiscall(this, int field, int* out_wchar_ptr)
// out receives a wchar_t* that points at the field's stored string (not a copy).
class CMarshal__GetWcharT : public HookBase<
	int(__fastcall*)(void*, void*, int, int*),
	0x109392d0,
	CMarshal__GetWcharT> {
public:
	static std::map<int, char[4096]> m_values; // UTF-8 converted snapshot per field id
	static bool bLogEnabled;
	static int __fastcall Call(void* CMarshal, void* edx, int Field, int* Out);
	static inline int __fastcall CallOriginal(void* CMarshal, void* edx, int Field, int* Out) {
		return m_original(CMarshal, edx, Field, Out);
	};
};
