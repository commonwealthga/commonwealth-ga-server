#pragma once

#include "src/Utils/HookBase.hpp"

class GameEngine__Tick : public HookBase<
	void(__fastcall*)(void*, void*, float),
	0x10c32dc0,
	GameEngine__Tick> {
public:
	static void __fastcall Call(void* Engine, void* edx, float DeltaSeconds);
	static inline void __fastcall CallOriginal(void* Engine, void* edx, float DeltaSeconds) {
		m_original(Engine, edx, DeltaSeconds);
	}
};
