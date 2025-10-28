#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class CMarshalRowSet__InsertRow : public HookBase<
	int(__fastcall*)(void*, void*, void*),
	0x109355e0,
	CMarshalRowSet__InsertRow> {
public:
	static int __fastcall Call(void* CMarshal, void* edx, void* Row);
	static inline int __fastcall CallOriginal(void* CMarshal, void* edx, void* Row) {
		return m_original(CMarshal, edx, Row);
	};
};


