#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// TgDevice::GetFireSocketName — native @ 0x10a1db40 (INTACT).
//
// UC signature: native function name GetFireSocketName();
//
// Thiscall layout (caller passes a return-slot pointer on the stack):
//   ECX        = ATgDevice* this
//   [esp+0x04] = FName* outName  (FName = { int Index; int Number; })
//   Returns outName.
//
// Stock pipeline: c_DeviceForm->c_Mesh (Tg mesh component) -> asmId ->
// CAssemblyManager::GetAssemblyMeshModel -> FX-entry query (group
// "ShotOrigin", display_mode == CurrentFireMode, display_order ==
// m_nSocketIndex, slot == r_eEquippedAt) -> socket FName; side effect:
// m_nSocketMax = max display_order, m_bSocketMaxCalculated = 1.
// On the dedicated server c_DeviceForm->c_Mesh is null for bot weapons, so
// the stock native returns 'None' and TgDevice.uc skips socket aiming.
//
// Our override: when the stock native returned 'None', rerun the SAME
// in-memory data query with asmId = Instigator->r_nBodyMeshAsmId (the FX
// rows for bot weapons are keyed by the body asm). Universal — no DB, no
// per-model tables. See decompiled/TgGame/ATgDevice/ATgDevice__GetFireSocketName/
// and .planning/2026-06-11-fire-sockets-investigation.md.
class TgDevice__GetFireSocketName : public HookBase<
    FName*(__fastcall*)(ATgDevice*, void*, FName*),
    0x10a1db40,
    TgDevice__GetFireSocketName> {
public:
    static FName* __fastcall Call(ATgDevice* Device, void* edx, FName* outName);
    static inline FName* __fastcall CallOriginal(ATgDevice* Device, void* edx, FName* outName) {
        return m_original(Device, edx, outName);
    }
};
