#pragma once

#include "src/pch.hpp"

// Per-device stats for the end-of-mission "Device Stats" tab.
//
// The tab reads ATgRepInfo_Player::r_DeviceStats[9] (owner-only replication,
// already wired in GetOptimizedRepListV2). Client row builder
// (UTgUIDeviceStats vtable +0x130 → 0x11463870) walks the first 8 entries,
// hides any whose Stats[DS_ID] <= 0, resolves Stats[DS_ID] against the asm
// device table for the name + icon, and prints Stats[1..6] verbatim. So the
// server owns every number, including DPM/HPM.
//
// Nothing in the shipped binary writes r_DeviceStats — the crediting natives
// are stripped, which is why the tab is empty.
namespace DeviceStats {

// Indices into FDeviceStatInfo::Stats (TgRepInfo_Player.DEVICE_STATS).
constexpr int kId          = 0;
constexpr int kDamage      = 1;
constexpr int kHealing     = 2;
constexpr int kPlayerKills = 3;
constexpr int kBotKills    = 4;

// Add `amount` to `field` on CreditPawn's row for `deviceId`, claiming a free
// row on first use. No-ops on a null pawn/PRI, deviceId <= 0, or when all 9
// rows are already claimed by other devices.
void Credit(ATgPawn* CreditPawn, int deviceId, int field, int amount);

// asm_data_set_devices_data_set_device_modes: device_mode_id → device_id.
// Loaded once, cached. Returns 0 when unknown.
int DeviceIdFromModeId(int deviceModeId);

// Deployable → the player-equippable device that places it (directly or via
// its projectile). Loaded once, cached. Returns 0 when unknown or when more
// than one equippable device spawns the same deployable (mines: Standard Mine
// vs Archer's Folly — no way to tell them apart from the deployable alone).
int DeviceIdFromDeployableId(int deployableId);

}  // namespace DeviceStats
