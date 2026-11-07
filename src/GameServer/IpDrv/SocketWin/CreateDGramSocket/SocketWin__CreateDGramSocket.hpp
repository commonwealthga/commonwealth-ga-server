#pragma once

#include "src/Utils/HookBase.hpp"

class SocketWin__CreateDGramSocket : public HookBase<
	void*(__fastcall*)(void*, void*, int),
	0x10ADB520,
	SocketWin__CreateDGramSocket> {
public:
    static void* __fastcall Call(void* This, void* edx, int bBroadcast);
	// Original is __thiscall with one stack arg (ret 0x4); ECX is unused.
	static inline void* __fastcall CallOriginal() {
		return m_original(nullptr, nullptr, 0);
	}
};

