#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// Intact native at 0x109c98e0. The binary only fires DoClientSideAlarmEffects
// via ProcessEvent — no server-side alarm activation. We hook to invoke
// Game::ActivateAlarm ourselves after CallOriginal so factories with
// bSpawnOnAlarm + matching nGlobalAlarmId actually wake up.
class TgPawn_Scanner__DoAlarm : public HookBase<
	void(__fastcall*)(ATgPawn_Scanner*, void*),
	0x109c98e0,
	TgPawn_Scanner__DoAlarm> {
public:
	static void __fastcall Call(ATgPawn_Scanner* Scanner, void* edx);
	static inline void __fastcall CallOriginal(ATgPawn_Scanner* Scanner, void* edx) {
		m_original(Scanner, edx);
	}
};
