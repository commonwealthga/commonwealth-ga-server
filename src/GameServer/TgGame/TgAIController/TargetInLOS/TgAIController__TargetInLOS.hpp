#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// Hook on `TgAIController::TargetInLOS` (FUN_10a85c60).
// Line traces between two AI pawns always fail, so opted-in bot factories
// (bot_sight_radius) could never shoot each other; for those pairs the radius
// stands in for line of sight (BotSight::TargetInRadius). Everything else
// keeps the original answer.
class TgAIController__TargetInLOS : public HookBase<
	int(__fastcall*)(ATgAIController*, void*),
	0x10a85c60,
	TgAIController__TargetInLOS> {
public:
	static int __fastcall Call(ATgAIController* aic, void* edx);
	static inline int __fastcall CallOriginal(ATgAIController* aic, void* edx) {
		return m_original(aic, edx);
	};
};
