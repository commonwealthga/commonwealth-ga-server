#pragma once

#include "src/Utils/HookBase.hpp"

class GameEngine__Init : public HookBase<
	void(__fastcall*)(void*),
	0x10BA5CB0,
	GameEngine__Init> {
public:
	static bool bInitTcpServer;
    static void __fastcall Call(void* GameEngine);
	static inline void __fastcall CallOriginal(void* GameEngine) {
		LogCallOriginalBegin();
		m_original(GameEngine);
		LogCallOriginalEnd();
	}
private:
	static void FixGlobals();
};

