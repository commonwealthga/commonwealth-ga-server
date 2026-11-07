#pragma once

#include "src/pch.hpp"

#include "lib/nlohmann/json.hpp"

// Player-owned pets and turrets (device-spawned bots: turrets, drones, pack
// pets, decoys) for MISSION_PROGRESS_SNAPSHOT / INSTANCE_RESTORE. The snapshot
// side lists each live bot a player's device spawned (owner user, bot id, the
// device and fire mode that spawned it, location, rotation, health, turret
// deploy progress, time left, current player target). The restore side
// re-spawns each one through the normal pet spawn path once its owner is alive
// on this instance with that device equipped, then sets the exact spot,
// health, deploy progress, time left and target. Vanity pets are re-summoned
// by the player spawn itself and are not listed.
namespace PetState {

// "pets" member for the feed (omitted when there are none).
void Collect(nlohmann::json& out);

void SetCarried(const nlohmann::json& progress);

// Called every game tick; cheap when nothing is carried.
void Tick(ATgGame* Game);

}  // namespace PetState
