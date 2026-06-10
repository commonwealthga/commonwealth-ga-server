#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// INTACT native at 0x109c9890 — the single funnel for scanner detection:
// both the beam path (TgPawn_Scanner__UpdateSensorSweep calls it after
// setting r_Target) and the damage path (UC TakeDamage sets r_Target then
// calls it) land here. The binary body does SetPhase(3) + EnemyDetected +
// clears the sweep flag. We append the mission-wide red
// "@@player_name@@ has been detected." alert after CallOriginal.
class TgPawn_Scanner__EnemyDetectedTimer : public HookBase<
	void(__fastcall*)(ATgPawn_Scanner*, void*),
	0x109c9890,
	TgPawn_Scanner__EnemyDetectedTimer> {
public:
	static void __fastcall Call(ATgPawn_Scanner* Scanner, void* edx);
	static inline void __fastcall CallOriginal(ATgPawn_Scanner* Scanner, void* edx) {
		m_original(Scanner, edx);
	}
};
