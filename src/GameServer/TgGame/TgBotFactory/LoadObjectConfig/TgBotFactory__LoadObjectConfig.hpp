#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"
#include "src/Config/DifficultyScalar.hpp"

#include <vector>

// One row from asm_data_set_bot_spawn_tables (LEFT JOIN asm_data_set_bots).
// Cached process-wide by EnsureSpawnTablesLoaded inside the TU.
struct SpawnTableEntry {
	int SpawnTableId;
	int SpawnGroup;
	int EnemyBotId;
	int BotCount;
	float SpawnChance;
	DifficultyScalar BBM;
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

	SpawnTableEntry (int nSpawnTableId,int nSpawnGroup,int nEnemyBotId,int nBotCount,float nSpawnChance,
					DifficultyScalar nBBM,const std::string& nReferenceName,int nGroupMin,int nGroupMax,int nRespawnSec)
		:SpawnTableId (nSpawnTableId)
		,SpawnGroup (nSpawnGroup)
		,EnemyBotId (nEnemyBotId)
		,BotCount (nBotCount)
		,SpawnChance (nSpawnChance)
		,BBM (nBBM)
		,ReferenceName (nReferenceName)
		,GroupMin (nGroupMin)
		,GroupMax (nGroupMax)
		,RespawnSec (nRespawnSec)
	{
	}
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
	int RolledBotId;      // The bot the group roll picked (bot_count groups
	                      // only; 0 for roster-style groups, gmax>0).
	bool PerEntryRoll;    // True when the group's bot should be re-rolled for
	                      // EVERY spawn instead of pinned to RolledBotId:
	                      // roster-style groups, and bot_count groups whose rows
	                      // all share one bot_count (the count then carries no
	                      // composition, so a multi-row group — 111 shipped
	                      // groups, e.g. table 28 group 2 = 2x one of 4 elites —
	                      // must produce a mixed pack). When the rows disagree
	                      // on bot_count the count IS the composition ("8
	                      // Dismantlers" OR "2 Elite Assassins") and the group
	                      // stays pinned to the rolled row.
	DifficultyScalar BBM;	// skal: add spawn-table bbm
	FSpawnGroupDetail Detail; // nMin/nMaxCount + nRespawnSeconds seeded;
	                          // nCurrentCount = 0 (caller preserves alive)
};

// skal: struct to carry around a {botid,bbm}
struct BotIdEntry {
	int BotId;
	DifficultyScalar BBM;
	BotIdEntry ():BotId (0),BBM () {}
	//BotIdEntry (int nBotId,float fBBM_HP,float fBBM_Dmg):BotId (nBotId),BBM (fBBM_HP,fBBM_Dmg) {}
	BotIdEntry (int nBotId,DifficultyScalar nBBM):BotId (nBotId),BBM (nBBM) {}
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
	//
	// rngKey/rngActivation address the roll inside the match seed
	// (GameRng::Purpose::SpawnPlan): pass FactoryRngKey(factory) and the
	// factory's activation counter so the roster reproduces exactly under a
	// recorded seed no matter when the activation happened.
	static std::vector<SpawnGroupPlan> RollSpawnPlan(int nSpawnTableId,
	                                                 uint64_t rngKey = 0,
	                                                 uint64_t rngActivation = 0);

	// Stable per-actor RNG key: map_object_id when the actor carries one
	// (stable per map placement), else the FName (Index<<32|Number) so
	// dynamically spawned factories still get a unique, deterministic key.
	static uint64_t FactoryRngKey(ATgBotFactory* factory);

	// Table's spawn_group value for a group INDEX (sorted order), or -1.
	// Queue entries and m_nFactorySpawnGroup carry indices; bot picks need
	// the group value.
	static int GetGroupNumberByIndex(int nSpawnTableId, int nGroupIndex);

	// Weighted-random pick of one bot_id from one (spawn table, spawn group
	// VALUE) at the current difficulty. Weight = spawn_chance. Returns 0 if
	// no rows exist at the current difficulty. Used per consumed queue entry
	// (fresh roll each spawn) and by SpawnObjectiveBot.
	//
	// Fresh roll each spawn for groups with PerEntryRoll (RollSpawnPlan leaves
	// those unpinned so a multi-row group produces a mixed pack instead of N
	// copies of one row).
	// skal: return value was botid, replace with {botid,bbm} the previous 'zero' return is simply moved to the entry.botid
	// rngKey/rngIndex address the roll inside the match seed
	// (GameRng::Purpose::SpawnEntryType) — normally
	// FactoryRngKey(factory) + the factory's per-spawn counter.
	static BotIdEntry PickBotFromSpawnTableGroup(int nSpawnTableId, int nSpawnGroup,
	                                            int* outBotCount = nullptr,
	                                            uint64_t rngKey = 0,
	                                            uint64_t rngIndex = 0);

	// Per-factory storage of the group rolls from the last ResetQueue, keyed
	// by the factory's FName (collision-safe across actor address reuse).
	// rolls[i] = RolledBotId for group index i (0 = roster group, roll per
	// entry). SpawnNextBot consults this so a group's whole roster — including
	// BotDied replacement entries — spawns the SAME rolled bot.
	// skal: used to be a vector<int> for botid, replaced with vector<BotIdEntry>
	// and the Get() return BotIdEntry instead of direct botid
	static void SetFactoryGroupRolls(ATgBotFactory* Factory, const std::vector<BotIdEntry>& rolls);
	static BotIdEntry  GetFactoryGroupRoll(ATgBotFactory* Factory, int nGroupIndex);

	// Optional per-factory override of group INDEX -> group VALUE resolution.
	// Escape-wave queues (SuperAgent OwnedSetQueue) repeat the rolled plan
	// several times over, so their group indices exceed the table's group
	// count and GetGroupNumberByIndex can't resolve them; the queue builder
	// stores the values here. -1 = no override (fall back to the table).
	static void SetFactoryGroupValues(ATgBotFactory* Factory, const std::vector<int>& values);
	static void ClearFactoryGroupValues(ATgBotFactory* Factory);
	static int  GetFactoryGroupValue(ATgBotFactory* Factory, int nGroupIndex);

	static void __fastcall Call(ATgBotFactory* BotFactory, void* edx);
	static inline void __fastcall CallOriginal(ATgBotFactory* BotFactory, void* edx) {
		m_original(BotFactory, edx);
	}
};
