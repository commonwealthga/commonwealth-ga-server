#pragma once

#include "src/Utils/HookBase.hpp"

class NetConnection__Cleanup : public HookBase<
	void(__fastcall*)(UNetConnection*),
	0x10C1AF50,
	NetConnection__Cleanup> {
public:
    static void __fastcall Call(UNetConnection* NetConnection);
	static inline void __fastcall CallOriginal(UNetConnection* NetConnection) {
		m_original(NetConnection);
	}
};

