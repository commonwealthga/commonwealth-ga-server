#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgBotFactory__BotDied : public HookBase<
	void(__fastcall*)(ATgBotFactory*, void*, ATgPawn*, ATgAIController*),
	0x10a8cbf0,
	TgBotFactory__BotDied> {
public:
	static void __fastcall Call(ATgBotFactory* BotFactory, void* edx, ATgPawn* Pawn, ATgAIController* AIC);
};
