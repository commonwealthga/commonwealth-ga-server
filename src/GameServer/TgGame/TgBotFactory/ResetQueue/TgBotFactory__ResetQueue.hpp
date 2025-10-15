#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgBotFactory__ResetQueue : public HookBase<
	void(__fastcall*)(ATgBotFactory*, void*, int),
	0x10a8c360,
	TgBotFactory__ResetQueue> {
public:
	static void __fastcall Call(ATgBotFactory* BotFactory, void* edx, int nOverrideSpawnTableId = 0);
	static inline void __fastcall CallOriginal(ATgBotFactory* BotFactory, void* edx, int nOverrideSpawnTableId = 0) {
		m_original(BotFactory, edx, nOverrideSpawnTableId);
	};
};

