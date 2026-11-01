#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class NetConnection__LowLevelSend : public HookBase<
	void(__fastcall*)(UNetConnection*, void*, void*, int),
	0x1092EEC0,
	NetConnection__LowLevelSend> {
public:
	static void __fastcall Call(UNetConnection* Connection, void* edx, void* Buffer, int Size);
	static inline void __fastcall CallOriginal(UNetConnection* Connection, void* Buffer, int Size) {
		return m_original(Connection, nullptr, Buffer, Size);
	}
};

