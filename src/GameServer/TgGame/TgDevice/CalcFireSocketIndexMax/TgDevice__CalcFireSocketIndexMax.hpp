#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// TgDevice::CalcFireSocketIndexMax — native @ 0x10a1dc50.
// Computes m_nSocketMax (at +0x234 on UTgDevice), the modulus used by
// UpdateIndex to cycle m_nSocketIndex across multiple firing sockets per
// shot. UpdateIndex calls this every shot.
//
// The stock native is gated on `c_DeviceForm->c_Mesh` (UMeshComponent at
// form +0x78). On a dedicated server c_Mesh is null for bot weapons —
// UMeshComponents are visual-only and don't initialize server-side — so
// the lookup short-circuits, m_nSocketMax falls back to 1, and the cycle
// never advances past index 1. FlashFire then ships m_nSocketIndex=1 to
// clients on every shot.
//
// Result observed in-game: Boss IceShrike (two shoulder cannons) only
// ever muzzle-flashes one cannon, regardless of which weapon he uses.
// Same shape would affect any multi-socket bot weapon.
//
// Fix: after the stock native runs, if m_nSocketMax is still 1, ask
// SocketCycle (DB-backed cache built from asm_data_set_asm_mesh_fxs) for
// the cycle size keyed by (Instigator's body_asm_id, device's
// r_eEquippedAt). If a multi-socket cycle exists, overwrite m_nSocketMax.
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
