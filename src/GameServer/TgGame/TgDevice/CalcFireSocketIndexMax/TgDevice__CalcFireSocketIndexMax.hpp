#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// TgDevice::CalcFireSocketIndexMax — native @ 0x10a1dc50.
// Computes m_nSocketMax (at +0x234 on UTgDevice), the modulus used by
// UpdateIndex to cycle m_nSocketIndex across multiple firing sockets per
// shot. UpdateIndex calls this every shot.
//
// The stock native is gated on `c_DeviceForm->c_Mesh` (UMeshComponent at
// form +0x78). On a dedicated server c_Mesh is typically null for bot
// weapons because UMeshComponents are visual-only and don't initialize on
// servers — so the lookup short-circuits, m_nSocketMax falls back to 1,
// and the cycle never advances past index 1. FlashFire then ships
// m_nSocketIndex=1 to clients on every shot.
//
// Result observed in-game: Boss Shrike (two shoulder cannons) only ever
// muzzle-flashes one cannon, regardless of which weapon he uses. Same
// shape would affect any multi-socket bot weapon.
//
// Fix: after the stock native runs, if m_nSocketMax is still 1, count
// entries in Instigator->m_TgSocketOffsetInfo->m_SocketOffsets whose
// SocketName matches "ShotOrigin". If count > 1, overwrite m_nSocketMax.
// FlashFire then ships an alternating index, and the client renders the
// right cannon using its own (locally-initialized) c_DeviceForm.
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
