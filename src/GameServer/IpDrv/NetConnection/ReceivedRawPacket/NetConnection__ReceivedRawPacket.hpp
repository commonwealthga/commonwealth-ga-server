#pragma once

#include "src/Utils/HookBase.hpp"

class NetConnection__ReceivedRawPacket : public HookBase<
	void(__fastcall*)(UNetConnection*, void*, void*, int),
	0x10C1C940,
	NetConnection__ReceivedRawPacket> {
public:
	static void __fastcall Call(UNetConnection* NetConnection, void* edx, void* buffer, int bytesRead);
	static inline void __fastcall CallOriginal(UNetConnection* NetConnection, void* edx, void* buffer, int bytesRead) {
		m_original(NetConnection, edx, buffer, bytesRead);
	}
};

