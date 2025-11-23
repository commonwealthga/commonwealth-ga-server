#pragma once

#include "src/Utils/HookBase.hpp"

class GameEngine__SpawnServerActors : public HookBase<
	void(__fastcall*)(void*),
	0x10BA0410,
	GameEngine__SpawnServerActors> {
public:
    static void __fastcall Call(void* GameEngine);
	static inline void __fastcall CallOriginal(void* GameEngine) {
		LogCallOriginalBegin();
		m_original(GameEngine);
		LogCallOriginalEnd();
	}
};
