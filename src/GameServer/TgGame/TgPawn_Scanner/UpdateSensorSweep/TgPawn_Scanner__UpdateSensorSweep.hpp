#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// Wrap of the INTACT TgPawn_Scanner::UpdateSensorSweep (client half: beam FX
// + hit processing, all NetMode-gated to non-dedicated). We add the missing
// SERVER half: detect enemy players inside the beam fan envelope and fire
// the intact EnemyDetectedTimer (phase 3 alert). Assumes the engine
// dispatches this on the server too — the entry log line on the `alarm`
// channel verifies that assumption on first capture.
class TgPawn_Scanner__UpdateSensorSweep : public HookBase<
	void(__fastcall*)(ATgPawn_Scanner*, void*, float),
	0x109d4750,
	TgPawn_Scanner__UpdateSensorSweep> {
public:
	static void __fastcall Call(ATgPawn_Scanner* Scanner, void* edx, float DeltaSeconds);
	static inline void __fastcall CallOriginal(ATgPawn_Scanner* Scanner, void* edx, float DeltaSeconds) {
		m_original(Scanner, edx, DeltaSeconds);
	};
};
