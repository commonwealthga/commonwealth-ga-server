#pragma once
#include "src/pch.hpp"

// Consumables (equip point 12, device slot 1482 "Consumable") are
// single-use bag items. The binary's RemoveConsumableFromOwnerInventory never
// runs for them (in_hand_device_flag = 0, same as the beacon) and its server
// half only synced the HUD, so the bag row was never spent.
namespace ConsumableUse {
	// Call after DeviceFiring.EndState on the server. No-op unless Device is a
	// player's equip-point-12 device with a real inventory row.
	void OnDeviceFired(ATgDevice* Device);
}
