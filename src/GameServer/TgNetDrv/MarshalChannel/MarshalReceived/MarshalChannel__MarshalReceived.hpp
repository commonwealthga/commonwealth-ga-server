#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// The UNetConnection for the marshal currently being dispatched. Set by
// MarshalChannel__MarshalReceived before calling CGameClient__MarshalReceived.
extern void* GCurrentMarshalConnection;

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

