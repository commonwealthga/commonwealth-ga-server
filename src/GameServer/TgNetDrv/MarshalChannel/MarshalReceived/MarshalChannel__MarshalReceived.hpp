#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class MarshalChannel__MarshalReceived : public HookBase<
	void(__fastcall*)(void*, void*, void*),
	0x109f9970,
	MarshalChannel__MarshalReceived> {
public:
	static void __fastcall Call(UMarshalChannel* MarshalChannel, void* edx, void* InBunch);
	static inline void __fastcall CallOriginal(UMarshalChannel* MarshalChannel, void* edx, void* InBunch) {
		m_original(MarshalChannel, edx, InBunch);
	};
};

