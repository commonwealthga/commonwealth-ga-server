#pragma once

#include "src/pch.hpp"

#include "lib/nlohmann/json.hpp"

// Per-player pawn state for MISSION_PROGRESS_SNAPSHOT / INSTANCE_RESTORE:
// location, rotation, health, power pool, running device cooldowns, timed
// effects and the equipped loadout (+ the device held in hand) of
// each live player (GA devices have no ammo/clip state - power is the pool). The
// snapshot side adds a "pawn" object to each "players" entry; the restore
// side keeps the carried states per user and applies each one once, when
// that user's pawn is alive on this instance (set exact values, short
// invulnerability so nobody dies during the reload).
namespace PlayerState {

nlohmann::json Describe(ATgPawn* Pawn);

void SetCarried(const nlohmann::json& players);

// Called every game tick; cheap when nothing is carried.
void Tick(ATgGame* Game);

}  // namespace PlayerState
