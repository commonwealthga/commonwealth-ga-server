#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgBotFactory__BotDied : public HookBase<
	void(__fastcall*)(ATgBotFactory*, void*, ATgPawn*, ATgAIController*),
	0x10a8cbf0,
	TgBotFactory__BotDied> {
public:
	static void __fastcall Call(ATgBotFactory* BotFactory, void* edx, ATgPawn* Pawn, ATgAIController* AIC);
	// BotDied (0x10a8cbf0) is INTACT — CallOriginal really runs retail's body.
	static inline void __fastcall CallOriginal(ATgBotFactory* BotFactory, void* edx, ATgPawn* Pawn, ATgAIController* AIC) {
		m_original(BotFactory, edx, Pawn, AIC);
	}
};
