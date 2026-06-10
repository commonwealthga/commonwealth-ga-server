#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// Stripped server native (RET 0x8 stub @ 0x10a6f2f0) — the ONLY dead link in
// the personal-scanner chain (Sensor Boost / Visual Scanner). Called by UC
// TgEffectSensor.CalcSensorVisConfig (prop-221 effect apply): returns the
// see/display flag words for the effect's visibility config so they land in
// the beneficiary pawn's r_ScannerSettings (bNetOwner-replicated → the reveal
// is naturally per-viewer; the client's CheckStealthedCharacter scanner loop
// + SetScannerDetectedStealthLightup do the rest locally).
//
// The prop-221 effect row's base_value IS the visibility_config_id
// (e.g. 34 → see_flags=16 (bit4 stealthed), display_flags=2 (in-game)).
// Source table: asm_data_set_visibility_configs.
//
// UC: native function GetSensorVisConfig(out int nSeeFlag, out int nDisplayFlag)
class TgEffectSensor__GetSensorVisConfig : public HookBase<
	void(__fastcall*)(UTgEffectSensor*, void*, int*, int*),
	0x10a6f2f0,
	TgEffectSensor__GetSensorVisConfig> {
public:
	static void __fastcall Call(UTgEffectSensor* Effect, void* edx, int* outSeeFlag, int* outDisplayFlag);
	static inline void __fastcall CallOriginal(UTgEffectSensor* Effect, void* edx, int* outSeeFlag, int* outDisplayFlag) {
		m_original(Effect, edx, outSeeFlag, outDisplayFlag);
	};
};
