#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgGame_Arena__LoadGameConfig : public HookBase<
	void(__fastcall*)(ATgGame_Arena*, void*),
	0x10AD9DA0,
	TgGame_Arena__LoadGameConfig> {
public:
	static void __fastcall* Call(ATgGame_Arena* Game, void* edx);
	static inline void __fastcall* CallOriginal(ATgGame_Arena* Game, void* edx) {
		m_original(Game, edx);
	}
};

