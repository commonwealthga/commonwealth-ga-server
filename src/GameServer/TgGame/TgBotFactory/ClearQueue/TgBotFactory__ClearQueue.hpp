#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgBotFactory__ClearQueue : public HookBase<
	void(__fastcall*)(ATgBotFactory*, void*),
	0x10a8c350,
	TgBotFactory__ClearQueue> {
public:
	static void __fastcall Call(ATgBotFactory* BotFactory, void* edx);
	static inline void __fastcall CallOriginal(ATgBotFactory* BotFactory, void* edx) {
		m_original(BotFactory, edx);
	};
};

