#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class ClientConnection__SendMarshal : public HookBase<
	void(__fastcall*)(UNetConnection*, void*, void*, int),
	0x1092d6b0,
	ClientConnection__SendMarshal> {
public:
	static void __fastcall Call(UNetConnection* Connection, void* edx, void* Marshal, int bAltQueue);
	// bAltQueue: byte flag selecting the second send queue; native callers pass 0.
	static inline void __fastcall CallOriginal(UNetConnection* MarshalChannel, void* edx, void* Marshal, int bAltQueue = 0) {
		m_original(MarshalChannel, edx, Marshal, bAltQueue);
	};
};

