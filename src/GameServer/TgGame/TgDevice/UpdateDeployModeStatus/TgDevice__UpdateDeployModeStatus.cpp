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

    CallOriginal(Device, edx);
}
