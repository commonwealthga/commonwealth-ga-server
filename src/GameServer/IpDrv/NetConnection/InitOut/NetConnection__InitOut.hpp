#pragma once

#include "src/Utils/HookBase.hpp"

class NetConnection__InitOut : public HookBase<
	void*(__fastcall*)(UNetConnection*),
	0x10C1C870,
	NetConnection__InitOut> {
public:
    static void* __fastcall Call(UNetConnection* NetConnection);
	static inline void* __fastcall CallOriginal(UNetConnection* NetConnection) {
		return m_original(NetConnection);
	}
};

// typedef void(__fastcall* tUNetConnectionInitOut)(int);
// tUNetConnectionInitOut pOriginalUNetConnectionInitOut = (tUNetConnectionInitOut)0x10C1C870;

