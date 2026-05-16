#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// TgDevice::GetFireSocketName — native @ 0x10a1db40.
//
// UC signature: native function name GetFireSocketName();
//
// Thiscall layout (caller passes a return-slot pointer on the stack):
//   ECX        = ATgDevice* this
//   [esp+0x04] = FName* outName  (FName = { int Index; int Number; })
//   Returns outName.
//
// Stock implementation reads c_DeviceForm->c_Mesh and looks up the socket
// name keyed by m_nSocketIndex via the AssemblyManager. On a dedicated
// server c_DeviceForm->c_Mesh is null for bot weapons, so it falls through
// and leaves outName = (0, 0), i.e. FName 'None'. TgDevice.uc:1813 then
// skips the socket-aim branch and spawns projectiles at the pawn body.
//
// Our override: if the stock native returned 'None' AND the pawn has a
// registered (asm_id, equip_point) socket cycle in SocketCycle, write the
// indexed FName ourselves. m_nSocketIndex is 1-based; we map it to the
// 0-based vector slot. Falls through silently for any non-bot pawn or any
// device whose slot has no cycle entry, preserving the stock behavior for
// player weapons.
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
