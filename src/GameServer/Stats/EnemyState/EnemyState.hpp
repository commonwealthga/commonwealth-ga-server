#pragma once

#include "src/pch.hpp"

#include "lib/nlohmann/json.hpp"

// Bot-factory enemy state for MISSION_PROGRESS_SNAPSHOT / INSTANCE_RESTORE.
// The snapshot side lists every live factory bot (factory id, bot id,
// location, rotation, health) plus per-factory spawn counters. The restore
// side cannot create the map's bots itself, so it waits for this instance's
// own factories to spawn them and then matches each new bot to a carried
// one of the same factory (same bot id first, nearest spot): exact location,
// rotation and health. A factory that had already spawned in the snapshot
// and has no carried bot left gets its extra spawns removed again (killed
// enemies stay dead); respawning factories are only matched, never trimmed.
namespace EnemyState {

// {"enemies": [...], "factories": [...]} members for the feed; empty
// arrays when the map has no factory bots.
void Collect(nlohmann::json& out);

void SetCarried(const nlohmann::json& progress);

// Bot id a map factory spawn should use while a restore is pending: the
// rolled id if a carried bot of that id is still unreserved, else the id of
// another unreserved carried bot of the factory (spawn tables can roll a
// different variant on the new host). Rolled id when nothing is carried.
int ChooseBotId(int factoryId, int rolledId);

// Called every game tick; cheap when nothing is carried.
void Tick(ATgGame* Game);

}  // namespace EnemyState
