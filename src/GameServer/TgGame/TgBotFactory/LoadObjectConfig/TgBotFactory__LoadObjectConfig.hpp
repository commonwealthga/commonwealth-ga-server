#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

#include <vector>

// One row from asm_data_set_bot_spawn_tables (LEFT JOIN asm_data_set_bots).
// Cached process-wide by EnsureSpawnTablesLoaded inside the TU; both
// CreateRandomSpawnQueue and PickBotFromSpawnTableGroup read from the cache.
// Exposed here so SpawnNextBot / SpawnObjectiveBot can declare local vectors
// of it for logging purposes; the cache itself is private to the .cpp.
struct SpawnTableEntry {
	int SpawnTableId;
	int SpawnGroup;
	int EnemyBotId;
	int BotCount;
	float SpawnChance;
	std::string ReferenceName;
};

// Hook for `Function TgGame.TgBotFactory.LoadObjectConfig` (stripped native).
//
// Responsibilities:
//   1. CallOriginal first so TgActorFactory parent overrides apply
//      (s_n_task_force, s_n_team_number, m_n_priority).
//   2. Apply per-actor MapObjectConfig overrides for every config-shaped UC
//      field on ATgBotFactory (n_spawn_table_id, b_auto_spawn, f_spawn_delay,
//      n_active_count, location_selection, …). See field-by-field table in
//      docs/superpowers/plans/2026-05-23-botfactory-mapobjectconfig-refactor.md
//      for the full column list.
//   3. Build the actor's m_SpawnQueue via CreateRandomSpawnQueue, and
//      populate m_SpawnGroups so per-group min/max/respawn state lives on
//      the actor where SpawnNextBot / BotDied can read it natively.
class TgBotFactory__LoadObjectConfig : public HookBase<
	void(__fastcall*)(ATgBotFactory*, void*),
	0x10a8c2d0,
	TgBotFactory__LoadObjectConfig> {
public:
	// Build a randomised spawn queue from one spawn table id at the current
	// difficulty. For each distinct spawn_group present in the table, emits
	// exactly one FSpawnQueueEntry (nGroupNumber=group, nSpawnTableId=table,
	// fSpawnTime=0, bRespawn=0). The actual bot is NOT chosen here — that
	// happens at spawn time via PickBotFromSpawnTableGroup, so each spawn
	// gets a fresh random pick.
	//
	// Used by:
	//   - TgBotFactory__LoadObjectConfig (write to actor's m_SpawnQueue)
	//   - TgBotFactory__ResetQueue and TgBotFactory__UseSpawnTable
	//   - TgMissionObjective_Bot__SpawnObjectiveBot (pick first entry to
	//     determine the (table, group) for its one-shot spawn)
	static std::vector<FSpawnQueueEntry> CreateRandomSpawnQueue(int nSpawnTableId);

	// Weighted-random pick of one bot_id from one (spawn table, spawn group)
	// at the current difficulty. Weight = spawn_chance (0.0–1.0) discretised
	// to ceil(chance * 100) slots. Returns 0 if no rows exist for that
	// (table, group) at the current difficulty.
	//
	// outBotCount (optional): if non-null, receives the picked row's bot_count
	// — the number of instances of the chosen bot to spawn for this entry.
	// SpawnNextBot uses this to honour per-group roster sizes ("spawn 4
	// guardians for this group"); SpawnObjectiveBot ignores it (always 1).
	//
	// Called by SpawnNextBot for each FSpawnQueueEntry consumed, and by
	// SpawnObjectiveBot when resolving s_nSpawnTableId.
	static int PickBotFromSpawnTableGroup(int nSpawnTableId, int nSpawnGroup, int* outBotCount = nullptr);

	static void __fastcall Call(ATgBotFactory* BotFactory, void* edx);
	static inline void __fastcall CallOriginal(ATgBotFactory* BotFactory, void* edx) {
		m_original(BotFactory, edx);
	}
};
