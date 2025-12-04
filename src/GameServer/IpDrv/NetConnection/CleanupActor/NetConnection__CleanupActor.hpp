#pragma once

#include "src/Utils/HookBase.hpp"

class NetConnection__CleanupActor : public HookBase<
	void(__fastcall*)(UNetConnection*),
	0x10c199b0,
	NetConnection__CleanupActor> {
public:
    static void __fastcall Call(UNetConnection* NetConnection);
	static inline void __fastcall CallOriginal(UNetConnection* NetConnection) {
		LogCallOriginalBegin();
		m_original(NetConnection);
		LogCallOriginalEnd();
	}
};

