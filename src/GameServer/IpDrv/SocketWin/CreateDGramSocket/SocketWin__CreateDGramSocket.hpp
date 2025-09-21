#pragma once

#include "src/Utils/HookBase.hpp"

class SocketWin__CreateDGramSocket : public HookBase<
	void*(__cdecl*)(void),
	0x10ADB520,
	SocketWin__CreateDGramSocket> {
public:
    static void* __fastcall Call();
	static inline void* __fastcall CallOriginal() {
		return m_original();
	}
};

