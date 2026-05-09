#include "src/GameServer/TgGame/TgPawn/GetMoraleDevice/TgPawn__GetMoraleDevice.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Reimplementation of TgPawn::GetMoraleDevice (UC native; binary stub @ 0x109bf7c0
// just `return 0`). Several call sites in our morale plumbing — and the binary's
// own AddMoralePoints validation chain — depend on this returning the actual
// equipped morale device.
//
// The pawn tracks the slot index in r_nMoraleDeviceSlot (set at equip time when
// the device's m_nDeviceType is TGDT_MORALE=476). The actual ATgDevice* lives
// in m_EquippedDevices[slot]. -1 means no morale device equipped.
ATgDevice_Morale* __fastcall TgPawn__GetMoraleDevice::Call(ATgPawn* Pawn, void* /*edx*/) {
	if (Pawn == nullptr) return nullptr;
	const int slot = Pawn->r_nMoraleDeviceSlot;
	if (slot < 0 || slot >= 0x19) return nullptr;
	ATgDevice* dev = Pawn->m_EquippedDevices[slot];
	return reinterpret_cast<ATgDevice_Morale*>(dev);
}
