#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgGame_Defense__CacheKismetConfiguration : public HookBase<
	void(__fastcall*)(ATgGame_Defense*, void*),
	0x10ad9eb0,
	TgGame_Defense__CacheKismetConfiguration> {
public:
	static void __fastcall Call(ATgGame_Defense* Game, void* edx);
	static inline void __fastcall CallOriginal(ATgGame_Defense* Game, void* edx) {
		m_original(Game, edx);
	};
};
