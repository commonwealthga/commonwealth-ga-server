#pragma once
#include "src/pch.hpp"

// Server-side fire rules from the
// patch notes that the final game data does not carry as flags:
//   1.36 "Consumables: Removed from PvP"          -> no Consume-slot fire in a PvP match
//   1.36 "Personal Vandal Boost disabled in Open Zone" -> no fire in an open world zone
// A refused ServerStartFire never reaches the device (nothing spent, morale
// bar kept); the player gets one short center-screen note.
namespace FireRules {
	// true = refuse this ServerStartFire (caller skips CallOriginal).
	bool RefuseStartFire(ATgDevice* Device);
}
