#include "src/GameServer/TgGame/TgDevice/UpdateDeployModeStatus/TgDevice__UpdateDeployModeStatus.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Fields on ATgDevice (from Ghidra decompile of UpdateDeployModeStatus):
//   +0x21c  r_nDeviceId                        (already named in SDK)
//   +0x229  byte:   active placement index     (used as ArrayNum[+0x238] idx)
//   +0x22a  byte:   state (1=valid, 2=invalid, 3=partial)
//   +0x22c  dword:  flags  (bit 0x400 gates the whole validator)
//   +0x238  ptr:    placement-slot array (on device instance)
//   +0x23c  int:    placement-slot array count
//
// We read these before calling original so if a fault occurs inside the
// original call, the last ring entries identify which device was being
// validated and its key state.

void __fastcall TgDevice__UpdateDeployModeStatus::Call(ATgDevice* Device, void* edx) {
    // Log entry FIRST with the raw pointer, before any field reads — if a
    // chained dereference (like Device->Class->GetFullName()) faults, we
    // still get a ring entry proving the hook ran.
    Logger::Log("deploy_validate", "enter: Device=%p\n", (void*)Device);

    // Only snapshot richer state if we safely can.
    uint8_t  descIdx  = 0;
    uint8_t  state    = 0;
    uint32_t flags    = 0;
    int      deviceId = 0;
    APawn*   instigator = nullptr;
    const char* devClassName = "<null>";
    const char* instClassName = "<null>";
    if (Device) {
        descIdx    = *(uint8_t*) ((char*)Device + 0x229);
        state      = *(uint8_t*) ((char*)Device + 0x22a);
        flags      = *(uint32_t*)((char*)Device + 0x22c);
        deviceId   = Device->r_nDeviceId;
        instigator = Device->Instigator;
        // GetFullName is risky if Class is a garbage non-null pointer; guard
        // by requiring the first dword (vtable) to at least look like a
        // module-range address (0x10000000-0x13ffffff covers GlobalAgenda +
        // loaded DLLs in our server process).
        auto classLooksReal = [](UObject* cls) -> bool {
            if (!cls) return false;
            uintptr_t vt = *(uintptr_t*)cls;
            return vt >= 0x00400000u && vt < 0x80000000u;
        };
        if (classLooksReal((UObject*)Device->Class))
            devClassName = Device->Class->GetFullName();
        if (instigator && classLooksReal((UObject*)instigator->Class))
            instClassName = instigator->Class->GetFullName();
    }

    Logger::Log("deploy_validate",
        "  dev=%p cls=%s devId=%d descIdx=%u state=%u flags=0x%x  inst=%p cls=%s\n",
        (void*)Device, devClassName,
        deviceId, (unsigned)descIdx, (unsigned)state, (unsigned)flags,
        (void*)instigator, instClassName);

    CallOriginal(Device, edx);
}
