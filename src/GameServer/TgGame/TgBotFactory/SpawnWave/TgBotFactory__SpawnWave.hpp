#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgBotFactory__SpawnWave : public HookBase<
	void(__fastcall*)(ATgBotFactory*, void*, int),
	0x10a8c320,
	TgBotFactory__SpawnWave> {
public:
	static void __fastcall Call(ATgBotFactory* BotFactory, void* edx, int Num);
	static inline void __fastcall CallOriginal(ATgBotFactory* BotFactory, void* edx, int Num) {
		m_original(BotFactory, edx, Num);
	};
};

