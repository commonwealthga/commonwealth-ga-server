#pragma once

#include "src/pch.hpp"

#include "lib/nlohmann/json.hpp"

// Player-placed deployables (stations, force fields, mines, sensors, bombs)
// for MISSION_PROGRESS_SNAPSHOT / INSTANCE_RESTORE. The snapshot side lists
// each live deployable a player placed (owner user, deployable id, the device
// and fire mode that placed it, location, rotation, health, deploy progress,
// life timers). The restore side re-spawns each one through the normal player
// deploy path once its owner is alive on this instance with that device
// equipped, then sets the exact spot, rotation, health, deploy progress and
// time left. Map-factory deployables come back with the map itself; spawn
// beacons are not carried.
namespace DeployableState {

// "deployables" member for the feed (omitted when there are none).
void Collect(nlohmann::json& out);

void SetCarried(const nlohmann::json& progress);

// Called every game tick; cheap when nothing is carried.
void Tick(ATgGame* Game);

}  // namespace DeployableState
