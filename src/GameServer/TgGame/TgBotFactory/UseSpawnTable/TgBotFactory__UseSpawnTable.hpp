#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgBotFactory__UseSpawnTable : public HookBase<
	void(__fastcall*)(ATgBotFactory*, void*),
	0x10a8c340,
	TgBotFactory__UseSpawnTable> {
public:
	static void __fastcall Call(ATgBotFactory* Factory, void* edx);
	static inline void __fastcall CallOriginal(ATgBotFactory* Factory, void* edx) {
		m_original(Factory, edx);
	};
};
