#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

#include <vector>

// One row from asm_data_set_bot_spawn_tables (LEFT JOIN asm_data_set_bots).
// Cached process-wide by EnsureSpawnTablesLoaded inside the TU.
struct SpawnTableEntry {
	int SpawnTableId;
	int SpawnGroup;
	int EnemyBotId;
	int BotCount;
	float SpawnChance;
	std::string ReferenceName;
	// spawn_group_min/max — when max>0, the group's roster is rand[min..max]
	// entries (Tick/Wasp Incubator tables use this with bot_count=1); when 0,
	// the roster is the chance-picked row's bot_count.
	int GroupMin;
	int GroupMax;
	// spawn_group_respawn_sec — per-group BotDied respawn delay; seeds
	// FSpawnGroupDetail::nRespawnSeconds (the intact BotDied reads it when
	// scheduling the replacement entry's fSpawnTime).
	int RespawnSec;
};

// ResetQueue/SpawnWave build product: one plan per group INDEX (the position
// of the group in sorted spawn_group order — the intact BotDied indexes
// m_SpawnGroups positionally via aic->m_nFactorySpawnGroup, and queue
// entries' nGroupNumber carries the same index, NOT the table's spawn_group
// value).
struct SpawnGroupPlan {
	int GroupNumber;      // the table's spawn_group value (for bot picks)
	int EntryCount;       // rolled roster: rand[gmin..gmax], or picked row's
	                      // bot_count, or 0 (chance roll landed in the empty
	                      // remainder when the group's chances sum < 1.0)
	int RolledBotId;      // bot_count-style groups (gmax==0): the ONE bot the
	                      // group roll picked — every entry of the group spawns
	                      // this bot (vanilla: per-group roll, not per-entry).
	                      // 0 for roster-style groups (gmax>0, incubators) —
	                      // those keep a fresh per-entry roll (mixed brood).
	FSpawnGroupDetail Detail; // nMin/nMaxCount + nRespawnSeconds seeded;
	                          // nCurrentCount = 0 (caller preserves alive)
};

// Hook for `Function TgGame.TgBotFactory.LoadObjectConfig` (stripped native).
//
// Responsibilities (queue building moved to ResetQueue — retail's UC
// PostBeginPlay calls ResetQueue() right after this):
//   1. Parent hook first so TgActorFactory overrides apply
//      (s_n_task_force, s_n_team_number, m_n_priority).
//   2. Apply per-actor MapObjectConfig overrides for every config-shaped UC
//      field on ATgBotFactory (n_spawn_table_id, b_auto_spawn, f_spawn_delay,
//      n_active_count, b_respawn, location_selection, …).
class TgBotFactory__LoadObjectConfig : public HookBase<
	void(__fastcall*)(ATgBotFactory*, void*),
	0x10a8c2d0,
	TgBotFactory__LoadObjectConfig> {
public:
	// Roll the spawn plan for one table at the current difficulty: one
	// SpawnGroupPlan per group index (sorted spawn_group order). Each group
	// rolls its chance-weighted row once — a below-1.0 chance sum can yield
	// EntryCount=0 ("group spawns nothing this activation").
	static std::vector<SpawnGroupPlan> RollSpawnPlan(int nSpawnTableId);

	// Table's spawn_group value for a group INDEX (sorted order), or -1.
	// Queue entries and m_nFactorySpawnGroup carry indices; bot picks need
	// the group value.
	static int GetGroupNumberByIndex(int nSpawnTableId, int nGroupIndex);

	// Weighted-random pick of one bot_id from one (spawn table, spawn group
	// VALUE) at the current difficulty. Weight = spawn_chance. Returns 0 if
	// no rows exist at the current difficulty. Used per consumed queue entry
	// of roster-style groups (fresh roll each spawn) and by SpawnObjectiveBot.
	static int PickBotFromSpawnTableGroup(int nSpawnTableId, int nSpawnGroup, int* outBotCount = nullptr);

	// Per-factory storage of the group rolls from the last ResetQueue, keyed
	// by the factory's FName (collision-safe across actor address reuse).
	// rolls[i] = RolledBotId for group index i (0 = roster group, roll per
	// entry). SpawnNextBot consults this so a group's whole roster — including
	// BotDied replacement entries — spawns the SAME rolled bot.
	static void SetFactoryGroupRolls(ATgBotFactory* Factory, const std::vector<int>& rolls);
	static int  GetFactoryGroupRoll(ATgBotFactory* Factory, int nGroupIndex);

	static void __fastcall Call(ATgBotFactory* BotFactory, void* edx);
	static inline void __fastcall CallOriginal(ATgBotFactory* BotFactory, void* edx) {
		m_original(BotFactory, edx);
	}
};
