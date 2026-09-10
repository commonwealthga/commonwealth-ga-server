#pragma once

#include "src/pch.hpp"

#include <vector>

// Hand-authored actors added to maps at load (things the map never baked).
// Apply() runs once per map from TgGame::InitGameRepInfo; its body is a plain
// per-map list of Add* calls.
namespace MapAdditions {

struct NavSpot {
	FVector Location;   // floor level — SpawnNextBot adds the bot's half-height
	int     Yaw = 0;    // bot facing at spawn (UE units, 65536 = 360°)
};

// Mirrors the map_object_config bot-factory columns. map_object_config rows
// keyed by nMapObjectId (+ map_name) still apply on top of these values.
struct BotFactorySpec {
	int  nMapObjectId      = 0;      // synthetic, unique per map (use 900000+)
	int  nSpawnTableId     = 0;
	bool bSpawnOnAlarm     = false;  // alarm responder
	int  nPriority         = 0;      // alarm responders: -1 (0 is never alarm-eligible)
	int  nGlobalAlarmId    = 0;      // non-zero retargets the responder's table on alarm
	bool bAutoSpawn        = true;   // spawns at load only if !bSpawnOnAlarm && nPriority==0
	bool bRespawn          = false;
	bool bBulkSpawn        = false;
	bool bPatrolLoop       = false;
	bool bAlwaysPatrol     = false;
	int  nActiveCount      = 0;
	float fSpawnDelay      = 0.2f;
	float fRespawnDelay    = 0.0f;
	unsigned char LocationSelection = 0;  // 0=random, 1=sequential
	unsigned char TypeSelection     = 0;
	unsigned char nTaskForce = 2;
	int  nTeamNumber        = 2;
	std::vector<NavSpot> SpawnPoints;     // required — one PathNode each
	std::vector<NavSpot> PatrolPoints;
};

// Boss-driven reinforcements: while the objective boss has a target, fire its
// spawn table on one of the listed factories every fCooldown seconds.
struct BossAlarmSpec {
	int  nObjectiveMapObjectId = 0;   // boss TgMissionObjective_Bot; 0 = the map's only one
	int  nSpawnTableId         = 0;
	std::vector<int> FactoryIds;      // factory map object ids (ours or baked); one picked per fire
	std::vector<int> Difficulties;    // allowed difficulty value ids; empty = all
	int   nMinPlayers       = 1;      // PlayerControllers in the match, checked every fire
	float fFirstDelay       = 12.0f;  // seconds of continuous target before the first fire
	float fCooldown         = 45.0f;
	float fWipeDespawnDelay = 10.0f;  // boss targetless this long -> despawn the factories' bots
};

void Apply(ATgGame* Game);

// Called from GameEngine::Tick; self-throttles to 1Hz, no-op without boss alarms.
void Tick();

// TgBotFactory::LoadObjectConfig hook entry: applies the pending spec for this
// factory's map object id (fields + LocationList/PatrolPath). No-op otherwise.
void ApplyPendingBotFactory(ATgBotFactory* Factory);

}  // namespace MapAdditions
