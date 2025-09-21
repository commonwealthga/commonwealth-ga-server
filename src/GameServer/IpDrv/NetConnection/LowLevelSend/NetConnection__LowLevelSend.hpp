#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class NetConnection__LowLevelSend : public HookBase<
	void(__fastcall*)(),
	0x1092EEC0,
	NetConnection__LowLevelSend> {
public:
	static void __fastcall Call(UNetConnection* Connection, void* edx, void* Buffer, int Size);
	static inline void __fastcall CallOriginal() {
		return m_original();
	}
};

