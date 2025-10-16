#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgAIController__InitBehavior : public HookBase<
	void(__fastcall*)(ATgAIController*, void*, void*, void*),
	0x10a87930,
	TgAIController__InitBehavior> {
public:
	static void __fastcall Call(ATgAIController* Controller, void* edx, void* Bot, void* BotFactory);
	static inline void __fastcall CallOriginal(ATgAIController* Controller, void* edx, void* Bot, void* BotFactory) {
		m_original(Controller, edx, Bot, BotFactory);
	};
};


