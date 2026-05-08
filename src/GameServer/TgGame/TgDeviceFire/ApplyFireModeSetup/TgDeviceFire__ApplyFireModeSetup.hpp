#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// UTgDeviceFire vtable[0x1d4] @ 0x10a1f3b0 — TgDeviceFire setup function.
//
// Not in the UC native function list (last UC native is GetShotPowerMinCost,
// per `unrealscript/TgGame/Classes/TgDeviceFire.uc` and confirmed by the
// `TgDeviceFireexec*` strings in Ghidra). Lives in the four "extra" vtable
// slots at the very end of the table @ 0x116f058c..0x116f0598.
//
// What it does (from decompile, see analysis in conversation):
//   • Iterates the setup struct's property template array at [+0x88..+0x8c]
//   • Per entry: instantiates a UTgProperty via FUN_109a9190(TgProperty, -1)
//     and ProcessEvents `DAT_119a2ef4` (a 'CopyFrom'/'Initialize' FName) to
//     populate it from the template
//   • Appends to `param_1[0x15]` = m_Properties (TArray @ +0x54 in TgDeviceFire)
//   • Then dispatches by the new property's m_nPropertyId to set the matching
//     m_n*Index cache field — case 10 → m_nAccuracyIndex (+0xD4), case 0xfd
//     → m_nAccuracyLossOnShootIndex (+0xF0), …~30 cases total
//   • Finally copies remaining setup-struct fields into the device fire
//     (m_nId, m_nAttackType, restrict flags, projectile/pet refs, etc.)
//
// In other words: this IS the missing piece that
// `TgDevice::ApplyInventoryEquipEffects_notimplemented` was supposed to
// drive. With m_Properties populated and m_n*Index caches set, the binary's
// own GetPropertyValueById path runs natively and applies buff math via
// vtable[0x55c] (GetBuffedProperty) — which is exactly what the
// GetPropertyValueById fallback hack tries to emulate manually.
//
// This hook is purely diagnostic: log every call, dump param_1/param_2/
// param_3, then return CallOriginal. We need to know whether this function
// runs naturally during equip/spawn (in which case the cached path SHOULD
// already work and the buff problem is elsewhere), or whether it never
// runs (in which case we need to drive it ourselves from the
// TgDevice::ApplyInventoryEquipEffects reimpl).
//
// Calling convention from the disassembly: __thiscall — ECX = this
// (UTgDeviceFire*), two stack args (owner, setupStruct). Returns int.
class TgDeviceFire__ApplyFireModeSetup : public HookBase<
	int(__fastcall*)(UTgDeviceFire*, void*, void*, void*),
	0x10a1f3b0,
	TgDeviceFire__ApplyFireModeSetup> {
public:
	static int __fastcall Call(UTgDeviceFire* DeviceFire, void* edx, void* Owner, void* SetupStruct);
	static inline int __fastcall CallOriginal(UTgDeviceFire* DeviceFire, void* edx, void* Owner, void* SetupStruct) {
		return m_original(DeviceFire, edx, Owner, SetupStruct);
	}
};
