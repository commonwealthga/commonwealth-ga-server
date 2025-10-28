#pragma once

#include "src/Utils/HookBase.hpp"

class NetConnection__FlushNet : public HookBase<
	void(__fastcall*)(UNetConnection*),
	0x10c1cab0,
	NetConnection__FlushNet> {
public:
    static void __fastcall Call(UNetConnection* NetConnection);
	static inline void __fastcall CallOriginal(UNetConnection* NetConnection) {
		m_original(NetConnection);
	}
};

