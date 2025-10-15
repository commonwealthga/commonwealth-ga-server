#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgBotFactory__UseSpawnTable : public HookBase<
	void(__fastcall*)(ATgBotFactory*, void*),
	0x10a8c340,
	TgBotFactory__UseSpawnTable> {
public:
	static void __fastcall Call(ATgBotFactory* BotFactory, void* edx);
	static inline void __fastcall CallOriginal(ATgBotFactory* BotFactory, void* edx) {
		m_original(BotFactory, edx);
	};
};

