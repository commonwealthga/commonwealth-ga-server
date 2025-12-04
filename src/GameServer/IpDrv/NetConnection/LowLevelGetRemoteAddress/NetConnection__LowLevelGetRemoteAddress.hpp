#pragma once

#include "src/Utils/HookBase.hpp"

class NetConnection__LowLevelGetRemoteAddress : public HookBase<
	void*(__fastcall*)(UNetConnection*, void*, void*),
	0x1092dc30,
	NetConnection__LowLevelGetRemoteAddress> {
public:
	static void* __fastcall Call(UNetConnection* NetConnection, void* edx, void* Out);
	static inline void* __fastcall CallOriginal(UNetConnection* NetConnection, void* edx, void* Out) {
		LogCallOriginalBegin();
		void* result = m_original(NetConnection, edx, Out);
		LogCallOriginalEnd();
		return result;
	}
};

