#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// TgDevice::CalcFireSocketIndexMax — native @ 0x10a1dc50 (INTACT).
// Computes m_nSocketMax (+0x234), the modulus UpdateIndex uses to cycle
// m_nSocketIndex across multiple firing sockets. UpdateIndex calls this
// every shot (after firing); FlashFire ships the index to clients, which
// drives which MuzzleFlash/Tracer FX socket plays.
//
// Stock pipeline mirrors GetFireSocketName: c_DeviceForm->c_Mesh -> asmId ->
// assembly model -> max display_order over FX entries (group "ShotOrigin",
// display_mode == CurrentFireMode, slot == r_eEquippedAt). Early-outs when
// m_bSocketMaxCalculated is set. On the dedicated server the form mesh is
// null for bot weapons, so m_nSocketMax stays 1 and multi-barrel bots only
// ever flash barrel #1.
//
// Our override: rerun the same in-memory query with asmId =
// Instigator->r_nBodyMeshAsmId when the stock result is still 1. Also the
// lazy catch-all call site for FireSockets::EnsurePopulated (SOI asset for
// the precise trace-origin path). See
// decompiled/TgGame/ATgDevice/ATgDevice__CalcFireSocketIndexMax/ and
// .planning/2026-06-11-fire-sockets-investigation.md.
class TgDevice__CalcFireSocketIndexMax : public HookBase<
    void(__fastcall*)(ATgDevice*, void*),
    0x10a1dc50,
    TgDevice__CalcFireSocketIndexMax> {
public:
    static void __fastcall Call(ATgDevice* Device, void* edx);
    static inline void __fastcall CallOriginal(ATgDevice* Device, void* edx) {
        m_original(Device, edx);
    }
};
