#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// UTgDeviceFire::GetPropertyValueById — original native @ 0x10a27d40.
//
// Original signature (UC):
//   native function float GetPropertyValueById(int nPropertyId, int nPropertyIndex);
//
// The original walks `m_Properties[nPropertyIndex]` (TArray @ +0x54) and reads
// the property's m_fRaw, optionally applying buff-modifier adjustments via a
// chain of vtable calls (CheckEffectBuffModifier etc.).
//
// PROBLEM: the per-fire-mode `m_Properties` array is supposed to be populated
// by `TgDevice::ApplyInventoryEquipEffects` — which is a STRIPPED stub (empty
// function @ 0x10a19a70). Without it:
//   • `m_Properties` stays empty
//   • every cached index field (`m_nAccuracyIndex`, `m_nAccuracyLossOnShootIndex`,
//     `m_nCoolDownTimeIndex`, …) stays at -1 (UC default)
//   • original `GetPropertyValueById(propId, -1)` short-circuits to 0.0f
//   • → accuracy / cooldown / range / power-cost math all silently degrade
//
// This hook replaces the broken cache-based lookup with a fallback that
// resolves by `nPropertyId` directly against the pawn's `s_Properties`.
// Slower than a cached index dereference, but correct.
//
// __thiscall: ECX = this (UTgDeviceFire*). Two args via stack: propId, index.
// __fastcall convention adds the dummy edx.
//
// Returns float in ST(0) (cdecl float convention) — represented as float here;
// the binary uses x87 float10 but a 32-bit float fits all observed values.
class TgDeviceFire__GetPropertyValueById : public HookBase<
	float(__fastcall*)(UTgDeviceFire*, void*, int, int),
	0x10a27d40,
	TgDeviceFire__GetPropertyValueById> {
public:
	static float __fastcall Call(UTgDeviceFire* DeviceFire, void* edx, int nPropertyId, int nPropertyIndex);
	static inline float __fastcall CallOriginal(UTgDeviceFire* DeviceFire, void* edx, int nPropertyId, int nPropertyIndex) {
		return m_original(DeviceFire, edx, nPropertyId, nPropertyIndex);
	}
};
