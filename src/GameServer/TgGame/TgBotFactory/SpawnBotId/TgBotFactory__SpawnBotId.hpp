#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgBotFactory__SpawnBotId : public HookBase<
	void(__fastcall*)(ATgBotFactory*, void*, int),
	0x10a8c300,
	TgBotFactory__SpawnBotId> {
public:
	static void __fastcall Call(ATgBotFactory* BotFactory, void* edx, int nBotId);
	static inline void __fastcall CallOriginal(ATgBotFactory* BotFactory, void* edx, int nBotId) {
		m_original(BotFactory, edx, nBotId);
	};
};

