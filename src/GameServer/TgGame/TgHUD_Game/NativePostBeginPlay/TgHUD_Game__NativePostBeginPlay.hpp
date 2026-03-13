#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgHUD_Game__NativePostBeginPlay : public HookBase<
	void(__fastcall*)(void*, void*),
	0x113a0860,
	TgHUD_Game__NativePostBeginPlay> {
public:
	static void __fastcall Call(void* HUD, void* edx);
	static inline void __fastcall CallOriginal(void* HUD, void* edx) {
		m_original(HUD, edx);
	}
};
