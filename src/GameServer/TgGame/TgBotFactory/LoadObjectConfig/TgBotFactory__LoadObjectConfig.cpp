#include "src/GameServer/TgGame/TgBotFactory/LoadObjectConfig/TgBotFactory__LoadObjectConfig.hpp"
#include "src/GameServer/TgGame/TgActorFactory/LoadObjectConfig/TgActorFactory__LoadObjectConfig.hpp"
#include "src/GameServer/Engine/MapObjectConfig/MapObjectConfig.hpp"
#include "src/Database/Database.hpp"
#include "src/Config/Config.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <cstdlib>
#include <ctime>
#include <map>
#include <vector>

namespace {

// Preloaded cache, populated once per process (or whenever difficulty
// changes). [bot_spawn_table_id][spawn_group] -> rows at that difficulty.
// Anonymous namespace = private to this TU; callers go through
// CreateRandomSpawnQueue / PickBotFromSpawnTableGroup, not the cache directly.
std::map<int, std::map<int, std::vector<SpawnTableEntry>>> g_spawnTables;
int g_loadedDifficultyValueId = -1;

// Medium Security (valid_value group 116). Of 211 distinct spawn tables in
// asm_data_set_bot_spawn_tables, only 41 ship rows at Ultra-Max Security
// (1471) — 136 of the 170 missing tables exist exclusively at 1029. Use 1029
// as the per-table fallback so PVE maps work at difficulties the boss/factory
// roster was never authored for, without polluting the verbatim asm_* data.
const int kFallbackDifficultyValueId = 1029;

// Load one difficulty pass into g_spawnTables. If skipExisting is true, rows
// whose bot_spawn_table_id is already present in the cache are dropped (used
// by the Medium-Security fallback pass so we don't overwrite the primary
// difficulty's roster). Returns the number of rows actually inserted and,
// via outTablesAdded, the number of new bot_spawn_table_ids introduced.
int LoadSpawnTableRows(sqlite3* db, int difficulty, bool skipExisting,
                       int* outTablesAdded) {
	if (outTablesAdded) *outTablesAdded = 0;

	sqlite3_stmt* stmt = nullptr;
	// COALESCE(bbm,1.0) > 0 — asm_data_set_bots.bot_balance_multiplier=0 is the
	// "never spawn this bot" sentinel (player pets, decoys, turrets, intentional
	// boss skips like Vulcan); orphan rows where the LEFT JOIN misses keep the
	// row by defaulting to 1.0.
	int rc = sqlite3_prepare_v2(db,
		"SELECT bot_spawn_table_id, spawn_group, enemy_bot_id, bot_count, "
		"       spawn_chance, COALESCE(reference_name, '') "
		"FROM asm_data_set_bot_spawn_tables "
		"LEFT JOIN asm_data_set_bots "
		"       ON asm_data_set_bot_spawn_tables.enemy_bot_id = asm_data_set_bots.bot_id "
		"WHERE difficulty_value_id = ? "
		"  AND COALESCE(asm_data_set_bots.bot_balance_multiplier, 1.0) > 0",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("tgbotfactory",
			"  LoadSpawnTableRows prepare failed (difficulty=%d): %s\n",
			difficulty, sqlite3_errmsg(db));
		return 0;
	}
	sqlite3_bind_int(stmt, 1, difficulty);

	int rowsInserted = 0;
	while (sqlite3_step(stmt) == SQLITE_ROW) {
		const int tableId = sqlite3_column_int(stmt, 0);
		if (skipExisting && g_spawnTables.find(tableId) != g_spawnTables.end()) continue;
		const int group   = sqlite3_column_int(stmt, 1);
		const int botId   = sqlite3_column_int(stmt, 2);
		const int count   = sqlite3_column_int(stmt, 3);
		const float chance = static_cast<float>(sqlite3_column_double(stmt, 4));
		const unsigned char* refNameRaw = sqlite3_column_text(stmt, 5);
		const std::string refName(refNameRaw ? reinterpret_cast<const char*>(refNameRaw) : "");

		auto& groupMap = g_spawnTables[tableId];
		const bool isNewTable = groupMap.empty();
		groupMap[group].push_back(SpawnTableEntry{
			tableId, group, botId, count, chance, refName
		});
		rowsInserted++;
		if (isNewTable && outTablesAdded) (*outTablesAdded)++;
	}
	sqlite3_finalize(stmt);
	return rowsInserted;
}

void EnsureSpawnTablesLoaded() {
	const int difficulty = Config::GetDifficultyValueId();
	if (g_loadedDifficultyValueId == difficulty && !g_spawnTables.empty()) return;
	g_spawnTables.clear();
	g_loadedDifficultyValueId = difficulty;

	sqlite3* db = Database::GetConnection();
	if (!db) {
		Logger::Log("tgbotfactory", "  EnsureSpawnTablesLoaded: no DB connection\n");
		return;
	}

	int primaryTables = 0;
	const int primaryRows = LoadSpawnTableRows(db, difficulty,
	                                           /*skipExisting=*/false,
	                                           &primaryTables);

	int fallbackTables = 0;
	int fallbackRows = 0;
	if (difficulty != kFallbackDifficultyValueId) {
		fallbackRows = LoadSpawnTableRows(db, kFallbackDifficultyValueId,
		                                  /*skipExisting=*/true,
		                                  &fallbackTables);
	}

	Logger::Log("tgbotfactory",
		"  EnsureSpawnTablesLoaded: difficulty=%d primary=%d rows / %d tables, "
		"fallback(%d)=%d rows / %d tables, total tables=%zu\n",
		difficulty, primaryRows, primaryTables,
		kFallbackDifficultyValueId, fallbackRows, fallbackTables,
		g_spawnTables.size());
}

// Seed once per process; rand() is fine for spawn randomisation.
void EnsureRandSeeded() {
	static bool seeded = false;
	if (!seeded) { srand(static_cast<unsigned>(time(nullptr))); seeded = true; }
}

// Apply MapObjectConfig overrides for every config-shaped field declared on
// ATgBotFactory (NOT the parent class — those are handled by
// TgActorFactory__LoadObjectConfig via CallOriginal).
void ApplyFactoryFieldOverrides(ATgBotFactory* f) {
	const int mid = f->m_nMapObjectId;

	Logger::Log("tgbotfactory", "ApplyFactoryFieldOverrides mapObjectId=%d, nSpawnTableId=%d", mid, f->nSpawnTableId);

	f->s_nTaskForce  = (unsigned char)MapObjectConfig::GetInt(
		mid, "s_n_task_force", f->s_nTaskForce);
	f->s_nTeamNumber = MapObjectConfig::GetInt(
		mid, "s_n_team_number", f->s_nTeamNumber);

	// Spawn-table id — headline knob. Everything else is meaningless without it.
	f->nSpawnTableId = MapObjectConfig::GetInt(mid, "n_spawn_table_id", f->nSpawnTableId);

	Logger::Log("tgbotfactory", "override n_spawn_table_id=%d\n", f->nSpawnTableId);

	// Booleans (1-bit bitfields in SDK; cast through bool to avoid signed
	// truncation surprises).
	f->bAutoSpawn                = (bool)MapObjectConfig::GetInt(mid, "b_auto_spawn",                  f->bAutoSpawn);
	f->bRespawn                  = (bool)MapObjectConfig::GetInt(mid, "b_respawn",                     f->bRespawn);
	f->bBulkSpawn                = (bool)MapObjectConfig::GetInt(mid, "b_bulk_spawn",                  f->bBulkSpawn);
	f->bAlwaysPatrol             = (bool)MapObjectConfig::GetInt(mid, "b_always_patrol",               f->bAlwaysPatrol);
	f->bSpawnOnAlarm             = (bool)MapObjectConfig::GetInt(mid, "b_spawn_on_alarm",              f->bSpawnOnAlarm);
	f->bPatrolLoop               = (bool)MapObjectConfig::GetInt(mid, "b_patrol_loop",                 f->bPatrolLoop);
	f->m_bIgnoreCollisionOnSpawn = (bool)MapObjectConfig::GetInt(mid, "m_b_ignore_collision_on_spawn", f->m_bIgnoreCollisionOnSpawn);

	// Counters
	f->nBotCount      = MapObjectConfig::GetInt(mid, "n_bot_count",       f->nBotCount);
	f->nActiveCount   = MapObjectConfig::GetInt(mid, "n_active_count",    f->nActiveCount);
	f->nGlobalAlarmId = MapObjectConfig::GetInt(mid, "n_global_alarm_id", f->nGlobalAlarmId);

	// Priority (game phase, NOT spawn priority — TgGame.s_nCurrentPriority is
	// the global state; this is which phase this factory belongs to)
	f->nPriority     = MapObjectConfig::GetInt(mid, "n_priority",      f->nPriority);
	f->nPrevPriority = MapObjectConfig::GetInt(mid, "n_prev_priority", f->nPrevPriority);

	// Timings / multipliers
	f->fSpawnDelay   = MapObjectConfig::GetFloat(mid, "f_spawn_delay",   f->fSpawnDelay);
	f->fRespawnDelay = MapObjectConfig::GetFloat(mid, "f_respawn_delay", f->fRespawnDelay);
	f->fBalance      = MapObjectConfig::GetFloat(mid, "f_balance",      f->fBalance);

	// Selection mode enums (eBotSelection: 0=BS_RANDOM, 1=BS_SEQUENTIAL)
	f->LocationSelection = (unsigned char)MapObjectConfig::GetInt(mid, "location_selection", f->LocationSelection);
	f->TypeSelection     = (unsigned char)MapObjectConfig::GetInt(mid, "type_selection",     f->TypeSelection);
}

// Populate the actor's m_SpawnGroups one entry per distinct group present in
// the configured spawn table. Native consumers (the intact BotDied at
// 0x10a8cbf0; SpawnNextBot regardless) read nCurrentCount + nMaxCount from
// this.
//
// At this point we haven't yet weighted-picked which row each group will
// use, so we can't know the target roster size — leave nMinCount/nMaxCount
// at 0 (= no per-group cap). SpawnNextBot writes them when it processes
// each group for the first time, setting both to the picked row's bot_count
// so BotDied + respawn refill back up to that level.
//
// nActiveCount remains the factory-wide hard cap, independent of per-group.
//
// m_SpawnGroups order is the same as the queue order (both come from the
// same std::map iteration), so m_SpawnGroups[i] corresponds to the group at
// m_SpawnQueue[i] — SpawnNextBot relies on this alignment.
void PopulateSpawnGroups(ATgBotFactory* f) {
	f->m_SpawnGroups.Clear();

	auto tableIt = g_spawnTables.find(f->nSpawnTableId);
	if (tableIt == g_spawnTables.end()) {
		Logger::Log("tgbotfactory",
			"  PopulateSpawnGroups: no rows for spawn_table_id=%d at current difficulty\n",
			f->nSpawnTableId);
		return;
	}

	const int respawnSecs = static_cast<int>(f->fRespawnDelay > 0.0f ? f->fRespawnDelay : 0.0f);

	for (const auto& groupKV : tableIt->second) {
		(void)groupKV;
		FSpawnGroupDetail gd;
		gd.nMinCount       = 0;   // set by SpawnNextBot on first pick
		gd.nMaxCount       = 0;   // 0 = no per-group cap until first pick
		gd.nCurrentCount   = 0;
		gd.nRespawnSeconds = respawnSecs;
		f->m_SpawnGroups.Add(gd);
	}
}

}  // namespace

int TgBotFactory__LoadObjectConfig::PickBotFromSpawnTableGroup(int nSpawnTableId, int nSpawnGroup, int* outBotCount) {
	EnsureSpawnTablesLoaded();
	EnsureRandSeeded();

	if (outBotCount) *outBotCount = 1;

	auto tableIt = g_spawnTables.find(nSpawnTableId);
	if (tableIt == g_spawnTables.end()) return 0;
	auto groupIt = tableIt->second.find(nSpawnGroup);
	if (groupIt == tableIt->second.end() || groupIt->second.empty()) return 0;

	// Weight each row by spawn_chance, but remember the row index so we can
	// recover the picked row's bot_count after the random pick.
	std::vector<int> weightedIdx;
	for (size_t i = 0; i < groupIt->second.size(); ++i) {
		const int slots = static_cast<int>(groupIt->second[i].SpawnChance * 100.0f);
		for (int j = 0; j < slots; ++j) weightedIdx.push_back(static_cast<int>(i));
	}
	if (weightedIdx.empty()) {
		Logger::Log("tgbotfactory",
			"  PickBotFromSpawnTableGroup: table=%d group=%d has rows but all "
			"spawn_chance rounded to zero\n", nSpawnTableId, nSpawnGroup);
		return 0;
	}
	const SpawnTableEntry& picked = groupIt->second[weightedIdx[rand() % weightedIdx.size()]];
	if (outBotCount) *outBotCount = picked.BotCount > 0 ? picked.BotCount : 1;
	return picked.EnemyBotId;
}

std::vector<FSpawnQueueEntry> TgBotFactory__LoadObjectConfig::CreateRandomSpawnQueue(int nSpawnTableId) {
	EnsureSpawnTablesLoaded();

	std::vector<FSpawnQueueEntry> result;
	auto tableIt = g_spawnTables.find(nSpawnTableId);
	if (tableIt == g_spawnTables.end()) {
		Logger::Log("tgbotfactory",
			"  CreateRandomSpawnQueue: no rows for spawn_table_id=%d at current difficulty\n",
			nSpawnTableId);
		return result;
	}

	// One queue entry per distinct group. Bot id is NOT chosen here — picked
	// fresh per spawn via PickBotFromSpawnTableGroup so consecutive spawns
	// from the same group can roll different bots from the weighted pool.
	for (const auto& groupKV : tableIt->second) {
		const int spawnGroup = groupKV.first;
		FSpawnQueueEntry entry;
		entry.nGroupNumber  = spawnGroup;
		entry.fSpawnTime    = 0.0f;
		entry.bRespawn      = 0;
		entry.nSpawnTableId = nSpawnTableId;
		result.push_back(entry);
	}

	Logger::Log("tgbotfactory",
		"  CreateRandomSpawnQueue: built %zu entries for spawn_table_id=%d\n",
		result.size(), nSpawnTableId);
	return result;
}

void __fastcall TgBotFactory__LoadObjectConfig::Call(ATgBotFactory* BotFactory, void* edx) {
	if (BotFactory == nullptr) return;
	const int mid = BotFactory->m_nMapObjectId;

	// TgBotFactory.LoadObjectConfig native is a STUB — CallOriginal here is a
	// no-op. Invoke the parent (TgActorFactory) hook directly so its
	// s_n_task_force / s_n_team_number / m_n_priority overrides land before
	// we layer the bot-factory-specific knobs on top.
	TgActorFactory__LoadObjectConfig::Call((ATgActorFactory*)BotFactory, edx);

	Logger::Log("tgbotfactory",
		"[%s] %s LoadObjectConfig mapObjectId=%d\n",
		Logger::GetTime(), BotFactory->GetName(), mid);

	ApplyFactoryFieldOverrides(BotFactory);

	if (BotFactory->nSpawnTableId <= 0) {
		Logger::Log("tgbotfactory",
			"  mapObjectId=%d: no n_spawn_table_id configured — empty queue, no bots\n", mid);
		BotFactory->m_SpawnQueue.Clear();
		BotFactory->m_SpawnGroups.Clear();
		return;
	}

	// Build queue (one entry per group) and per-group state. Native consumers
	// (SpawnNextBot, the intact BotDied at 0x10a8cbf0) read both off the
	// actor directly — no side maps.
	BotFactory->m_SpawnQueue.Clear();
	const auto queue = CreateRandomSpawnQueue(BotFactory->nSpawnTableId);
	for (const auto& e : queue) BotFactory->m_SpawnQueue.Add(e);
	PopulateSpawnGroups(BotFactory);

	Logger::Log("tgbotfactory",
		"  mapObjectId=%d: spawn_table=%d  groups=%d  queueEntries=%d  "
		"autoSpawn=%d nPriority=%d taskForce=%d nActiveCount=%d nBotCount=%d "
		"locSel=%d typeSel=%d\n",
		mid, BotFactory->nSpawnTableId,
		BotFactory->m_SpawnGroups.Num(), BotFactory->m_SpawnQueue.Num(),
		(int)BotFactory->bAutoSpawn, BotFactory->nPriority, BotFactory->s_nTaskForce,
		BotFactory->nActiveCount, BotFactory->nBotCount,
		(int)BotFactory->LocationSelection, (int)BotFactory->TypeSelection);
}
