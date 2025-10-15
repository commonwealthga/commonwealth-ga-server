#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgBotFactory__SpawnBot : public HookBase<
	void(__fastcall*)(ATgBotFactory*, void*),
	0x10a8c2e0,
	TgBotFactory__SpawnBot> {
public:
	static void __fastcall Call(ATgBotFactory* BotFactory, void* edx);
	static inline void __fastcall CallOriginal(ATgBotFactory* BotFactory, void* edx) {
		m_original(BotFactory, edx);
	};
};

