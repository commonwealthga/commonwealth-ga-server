#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class CMarshalObject__Create : public HookBase<
	void*(__fastcall*)(void*),
	0x10935320,
	CMarshalObject__Create> {
public:
	static inline void** CMarshal_vftable = (void**)0x1168e364;
	static void* __fastcall Call(void* Marshal);
	static inline void* __fastcall CallOriginal(void* Marshal) {
		return m_original(Marshal);
	};
};

