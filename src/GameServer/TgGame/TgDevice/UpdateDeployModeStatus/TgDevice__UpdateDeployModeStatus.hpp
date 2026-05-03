#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// TgDevice::UpdateDeployModeStatus — per-tick validator for deploy-mode
// placement preview. Ghidra address 0x10a273b0. Identified as the crash
// site during PvE where AI-spawned pets/deployables trigger a bogus 0x1d5
// pointer dereference.
//
// Hook is diagnostic-only: logs key state to the "deploy_validate" crash
// channel, then calls original. The crash ring buffer will preserve the
// last 100 validations so the next crash dump pinpoints exactly which
// device triggered it.
class TgDevice__UpdateDeployModeStatus : public HookBase<
    void(__fastcall*)(ATgDevice*, void*),
    0x10a273b0,
    TgDevice__UpdateDeployModeStatus> {
public:
    static void __fastcall Call(ATgDevice* Device, void* edx);
    static inline void __fastcall CallOriginal(ATgDevice* Device, void* edx) {
        m_original(Device, edx);
    }
};
