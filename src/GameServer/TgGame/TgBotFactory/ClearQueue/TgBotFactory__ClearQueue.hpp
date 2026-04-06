#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgBotFactory__ClearQueue : public HookBase<
	void(__fastcall*)(ATgBotFactory*, void*),
	0x10a8c350,
	TgBotFactory__ClearQueue> {
public:
	static void __fastcall Call(ATgBotFactory* Factory, void* edx);
	static inline void __fastcall CallOriginal(ATgBotFactory* Factory, void* edx) {
		m_original(Factory, edx);
	};
};
