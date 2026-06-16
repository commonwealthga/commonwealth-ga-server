#include "src/GameServer/TgGame/TgBotFactory/LoadObjectConfig/TgBotFactory__LoadObjectConfig.hpp"
#include "src/GameServer/TgGame/TgActorFactory/LoadObjectConfig/TgActorFactory__LoadObjectConfig.hpp"
#include "src/GameServer/Engine/MapObjectConfig/MapObjectConfig.hpp"
#include "src/GameServer/GameModes/SuperAgent/SuperAgent.hpp"
#include "src/Database/Database.hpp"
#include "src/Config/Config.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <cstdlib>
#include <cstring>
#include <ctime>
#include <map>
#include <set>
#include <vector>

namespace {

// Preloaded cache, populated once per process (or whenever difficulty
// changes). [bot_spawn_table_id][spawn_group] -> rows at that difficulty.
// Anonymous namespace = private to this TU; callers go through
// CreateRandomSpawnQueue / PickBotFromSpawnTableGroup, not the cache directly.
std::map<int, std::map<int, std::vector<SpawnTableEntry>>> g_spawnTables;
int g_loadedDifficultyValueId = -1;

// Build the cascade order: every difficulty in valid_value_group 116 with
// sort_order <= primary's sort_order, walked from primary down to Novice.
// Skip-existing semantics mean each tier only fills in spawn-table ids the
// higher tiers didn't ship. Returns the primary tier first, then descending.
//
// Group 116 ladder (sort_order : value_id):
//   1=Novice 1467, 2=Low Security 1028, 3=Adept 1468, 4=Medium Security 1029,
//   5=Advanced 1469, 6=High Security 1030, 7=Expert 1470,
//   8=Maximum Security 1259, 9=Ultra-Max Security 1471, 10=Double Agent 1260.
// Double Agent (1260) sits above 1471; the cascade is strictly "<= current",
// so 1260 never participates unless it's the primary.
std::vector<int> GetDifficultyCascade(sqlite3* db, int primaryDifficulty) {
	std::vector<int> cascade;

	// Super Agent (10000) is a custom difficulty: it MAY ship its own spawn-table
	// rows (which must take precedence) but otherwise inherits Ultra-Max Security
	// (1471). Build the cascade from 1471's group-116 ladder, then put 10000 back
	// at the FRONT as the primary tier so any 10000 rows win and 1471-and-below
	// fill the gaps. (Gap tiers correctly load full multi-row tables — see the
	// pre-existing-table snapshot in LoadSpawnTableRows.)
	const int requestedDifficulty = primaryDifficulty;
	if (primaryDifficulty == 10000) {
		primaryDifficulty = 1471;
	}

	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"SELECT value_id FROM asm_data_set_valid_values "
		"WHERE valid_value_group_id = 116 "
		"  AND sort_order <= (SELECT sort_order FROM asm_data_set_valid_values "
		"                     WHERE valid_value_group_id = 116 AND value_id = ?) "
		"ORDER BY sort_order DESC",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("tgbotfactory",
			"  GetDifficultyCascade prepare failed: %s — falling back to primary only\n",
			sqlite3_errmsg(db));
		cascade.push_back(primaryDifficulty);
		return cascade;
	}
	sqlite3_bind_int(stmt, 1, primaryDifficulty);

	while (sqlite3_step(stmt) == SQLITE_ROW) {
		cascade.push_back(sqlite3_column_int(stmt, 0));
	}
	sqlite3_finalize(stmt);

	// Defensive: if the lookup returned nothing (e.g. primary isn't in group
	// 116), at least try the primary directly so we don't degrade to "no
	// spawn tables at all".
	if (cascade.empty()) {
		cascade.push_back(primaryDifficulty);
	}

	// Custom difficulty leads the cascade as the primary tier, so its own rows
	// (if any) win over the inherited Ultra-Max roster.
	if (requestedDifficulty == 10000) {
		cascade.insert(cascade.begin(), requestedDifficulty);
	}

	return cascade;
}

// Load one difficulty pass into g_spawnTables. If skipExisting is true, rows
// whose bot_spawn_table_id is already present in the cache are dropped (so
// each cascade tier only fills the gaps left by the tiers above it). Returns
// the number of rows actually inserted and, via outTablesAdded, the number of
// new bot_spawn_table_ids introduced.
//
// Sources both asm_data_set_bot_spawn_tables (verbatim game data, read-only)
// and mod_data_set_bot_spawn_tables (our custom additions) via UNION ALL.
// Rows from either table participate equally in the cascade.
int LoadSpawnTableRows(sqlite3* db, int difficulty, bool skipExisting,
                       int* outTablesAdded) {
	if (outTablesAdded) *outTablesAdded = 0;

	sqlite3_stmt* stmt = nullptr;
	// COALESCE(bbm,1.0) > 0 — asm_data_set_bots.bot_balance_multiplier=0 is the
	// "never spawn this bot" sentinel (player pets, decoys, turrets, intentional
	// boss skips like Vulcan); orphan rows where the LEFT JOIN misses keep the
	// row by defaulting to 1.0.
	int rc = sqlite3_prepare_v2(db,
		"SELECT u.bot_spawn_table_id, u.spawn_group, u.enemy_bot_id, u.bot_count, "
		"       u.spawn_chance, COALESCE(b.reference_name, ''), "
		"       u.spawn_group_min, u.spawn_group_max, u.spawn_group_respawn_sec "
		"FROM ( "
		"  SELECT bot_spawn_table_id, difficulty_value_id, spawn_group, "
		"         enemy_bot_id, bot_count, spawn_chance, "
		"         spawn_group_min, spawn_group_max, spawn_group_respawn_sec "
		"  FROM asm_data_set_bot_spawn_tables "
		"  UNION ALL "
		"  SELECT bot_spawn_table_id, difficulty_value_id, spawn_group, "
		"         enemy_bot_id, bot_count, spawn_chance, "
		"         spawn_group_min, spawn_group_max, spawn_group_respawn_sec "
		"  FROM mod_data_set_bot_spawn_tables "
		") AS u "
		"LEFT JOIN asm_data_set_bots b ON u.enemy_bot_id = b.bot_id "
		"WHERE u.difficulty_value_id = ? "
		"  AND COALESCE(b.bot_balance_multiplier, 1.0) > 0",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("tgbotfactory",
			"  LoadSpawnTableRows prepare failed (difficulty=%d): %s\n",
			difficulty, sqlite3_errmsg(db));
		return 0;
	}
	sqlite3_bind_int(stmt, 1, difficulty);

	// Snapshot the tables shipped by HIGHER-priority tiers. skipExisting must
	// drop only tables a PREVIOUS tier already shipped — never a table THIS tier
	// is introducing. Testing the live g_spawnTables instead would skip every row
	// after the first of each new table (the cache gains the table on row 1),
	// truncating every gap-filled table to a single row.
	std::set<int> preexistingTables;
	if (skipExisting) {
		for (const auto& kv : g_spawnTables) preexistingTables.insert(kv.first);
	}

	int rowsInserted = 0;
	while (sqlite3_step(stmt) == SQLITE_ROW) {
		const int tableId = sqlite3_column_int(stmt, 0);
		if (skipExisting && preexistingTables.count(tableId)) continue;
		const int group   = sqlite3_column_int(stmt, 1);
		const int botId   = sqlite3_column_int(stmt, 2);
		const int count   = sqlite3_column_int(stmt, 3);
		const float chance = static_cast<float>(sqlite3_column_double(stmt, 4));
		const unsigned char* refNameRaw = sqlite3_column_text(stmt, 5);
		const std::string refName(refNameRaw ? reinterpret_cast<const char*>(refNameRaw) : "");
		const int groupMin   = sqlite3_column_int(stmt, 6);
		const int groupMax   = sqlite3_column_int(stmt, 7);
		const int respawnSec = sqlite3_column_int(stmt, 8);

		auto& groupMap = g_spawnTables[tableId];
		const bool isNewTable = groupMap.empty();
		groupMap[group].push_back(SpawnTableEntry{
			tableId, group, botId, count, chance, refName, groupMin, groupMax, respawnSec
		});
		rowsInserted++;
		if (isNewTable && outTablesAdded) (*outTablesAdded)++;
	}
	sqlite3_finalize(stmt);
	return rowsInserted;
}

// Super Agent composite tables: REBUILD each target table by concatenating the
// groups of its source tables (renumbered into a fresh 0..N sequence). Runs
// after the cascade so the source tables are already loaded. Difficulty-gated to
// the custom mode so normal play is untouched. A target listed as its own source
// is read before the overwrite, so it can keep its original groups.
void ApplyCompositeTables() {
	if (!SuperAgent::IsActive()) return;
	for (const auto& comp : SuperAgent::SpawnTableComposites()) {
		const int target = comp.first;
		std::map<int, std::vector<SpawnTableEntry>> combined;
		int nextGroup = 0;
		for (const int src : comp.second) {
			const auto it = g_spawnTables.find(src);
			if (it == g_spawnTables.end()) {
				Logger::Log("tgbotfactory",
					"  composite %d: source table %d not loaded at this difficulty — skipped\n",
					target, src);
				continue;
			}
			for (const auto& grp : it->second) {           // ascending group key
				std::vector<SpawnTableEntry> rows = grp.second;   // copy
				for (auto& e : rows) { e.SpawnTableId = target; e.SpawnGroup = nextGroup; }
				combined[nextGroup] = std::move(rows);
				nextGroup++;
			}
		}
		if (!combined.empty()) {
			g_spawnTables[target] = std::move(combined);
			Logger::Log("tgbotfactory",
				"  composite table %d = %d group(s) from %zu source(s)\n",
				target, nextGroup, comp.second.size());
		}
	}
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

	const std::vector<int> cascade = GetDifficultyCascade(db, difficulty);
	for (size_t i = 0; i < cascade.size(); ++i) {
		const int tier = cascade[i];
		const bool isPrimary = (i == 0);
		int tablesAdded = 0;
		const int rows = LoadSpawnTableRows(db, tier,
		                                    /*skipExisting=*/!isPrimary,
		                                    &tablesAdded);
		Logger::Log("tgbotfactory",
			"  cascade tier %zu difficulty=%d -> %d rows / %d new tables%s\n",
			i, tier, rows, tablesAdded, isPrimary ? " (primary)" : "");
	}

	// Rebuild composite tables from the now-loaded raw tables (Super Agent only).
	ApplyCompositeTables();

	Logger::Log("tgbotfactory",
		"  EnsureSpawnTablesLoaded: difficulty=%d cascade depth=%zu total tables=%zu\n",
		difficulty, cascade.size(), g_spawnTables.size());
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

// Roll one group's chance-weighted row. Chances within a (difficulty, group)
// usually sum to ~1.0 (pick which bot the group fields); a sum below 1.0
// leaves an "empty" remainder — the group spawns nothing this roll (e.g.
// table 28 group 3: single row at 0.75 = 75% chance the group exists).
// Returns nullptr for the empty outcome.
const SpawnTableEntry* RollGroupRow(const std::vector<SpawnTableEntry>& rows) {
	float total = 0.0f;
	for (const auto& r : rows) total += r.SpawnChance;
	if (total <= 0.0f) return nullptr;

	const float span = total > 1.0f ? total : 1.0f;
	float roll = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * span;
	for (const auto& r : rows) {
		if (roll < r.SpawnChance) return &r;
		roll -= r.SpawnChance;
	}
	return nullptr;  // landed in the empty remainder
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

	// Spawn-time pick: the group's existence was already decided at queue
	// build (RollSpawnPlan), so re-roll among the rows WITHOUT the empty
	// outcome — normalise by the chance total.
	float total = 0.0f;
	for (const auto& r : groupIt->second) total += r.SpawnChance;
	if (total <= 0.0f) return 0;
	float roll = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * total;
	const SpawnTableEntry* picked = &groupIt->second.back();
	for (const auto& r : groupIt->second) {
		if (roll < r.SpawnChance) { picked = &r; break; }
		roll -= r.SpawnChance;
	}
	if (outBotCount) *outBotCount = picked->BotCount > 0 ? picked->BotCount : 1;
	return picked->EnemyBotId;
}

int TgBotFactory__LoadObjectConfig::GetGroupNumberByIndex(int nSpawnTableId, int nGroupIndex) {
	EnsureSpawnTablesLoaded();
	auto tableIt = g_spawnTables.find(nSpawnTableId);
	if (tableIt == g_spawnTables.end() || nGroupIndex < 0) return -1;
	int i = 0;
	for (const auto& groupKV : tableIt->second) {
		if (i++ == nGroupIndex) return groupKV.first;
	}
	return -1;
}

std::vector<SpawnGroupPlan> TgBotFactory__LoadObjectConfig::RollSpawnPlan(int nSpawnTableId) {
	EnsureSpawnTablesLoaded();
	EnsureRandSeeded();

	std::vector<SpawnGroupPlan> plan;
	auto tableIt = g_spawnTables.find(nSpawnTableId);
	if (tableIt == g_spawnTables.end()) {
		Logger::Log("tgbotfactory",
			"  RollSpawnPlan: no rows for spawn_table_id=%d at current difficulty\n",
			nSpawnTableId);
		return plan;
	}

	for (const auto& groupKV : tableIt->second) {
		SpawnGroupPlan gp;
		gp.GroupNumber  = groupKV.first;
		gp.EntryCount   = 0;
		gp.RolledBotId  = 0;
		gp.Detail.nMinCount       = 0;
		gp.Detail.nMaxCount       = 0;
		gp.Detail.nCurrentCount   = 0;
		gp.Detail.nRespawnSeconds = 0;

		const SpawnTableEntry* row = RollGroupRow(groupKV.second);
		if (row != nullptr) {
			if (row->GroupMax > 0) {
				// Random roster: rand[gmin..gmax] entries (incubator-style),
				// each entry re-rolls its bot at spawn (mixed brood).
				const int lo = row->GroupMin > 0 ? row->GroupMin : row->GroupMax;
				const int hi = row->GroupMax >= lo ? row->GroupMax : lo;
				gp.EntryCount = lo + (hi > lo ? rand() % (hi - lo + 1) : 0);
				gp.Detail.nMinCount = lo;
				gp.Detail.nMaxCount = hi;
			} else {
				// bot_count group: ONE roll fields the whole group — every
				// entry spawns the rolled bot (vanilla per-group semantics).
				gp.EntryCount  = row->BotCount > 0 ? row->BotCount : 1;
				gp.RolledBotId = row->EnemyBotId;
				gp.Detail.nMinCount = gp.EntryCount;
				gp.Detail.nMaxCount = gp.EntryCount;
			}
			gp.Detail.nRespawnSeconds = row->RespawnSec;
		}
		plan.push_back(gp);
	}
	return plan;
}

// Last ResetQueue's per-group rolled bots, keyed by factory FName
// (Index<<32|Number — immune to actor address reuse). Bounded by the number
// of factory instances in the map process.
static std::map<uint64_t, std::vector<int>> g_factoryGroupRolls;

static uint64_t FactoryKey(ATgBotFactory* f) {
	uint32_t number = 0;
	std::memcpy(&number, ((const char*)&f->Name) + 4, sizeof(number));
	return (static_cast<uint64_t>(static_cast<uint32_t>(f->Name.Index)) << 32) | number;
}

void TgBotFactory__LoadObjectConfig::SetFactoryGroupRolls(
		ATgBotFactory* Factory, const std::vector<int>& rolls) {
	if (Factory == nullptr) return;
	g_factoryGroupRolls[FactoryKey(Factory)] = rolls;
}

int TgBotFactory__LoadObjectConfig::GetFactoryGroupRoll(
		ATgBotFactory* Factory, int nGroupIndex) {
	if (Factory == nullptr || nGroupIndex < 0) return 0;
	auto it = g_factoryGroupRolls.find(FactoryKey(Factory));
	if (it == g_factoryGroupRolls.end()) return 0;
	if (nGroupIndex >= static_cast<int>(it->second.size())) return 0;
	return it->second[nGroupIndex];
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

	// Queue + group building belongs to ResetQueue — UC PostBeginPlay calls
	// it right after this returns (TgBotFactory.uc:133).
	Logger::Log("tgbotfactory",
		"  mapObjectId=%d: spawn_table=%d autoSpawn=%d respawn=%d alarm=%d "
		"nPriority=%d taskForce=%d nActiveCount=%d locSel=%d typeSel=%d\n",
		mid, BotFactory->nSpawnTableId,
		(int)BotFactory->bAutoSpawn, (int)BotFactory->bRespawn,
		(int)BotFactory->bSpawnOnAlarm, BotFactory->nPriority,
		BotFactory->s_nTaskForce, BotFactory->nActiveCount,
		(int)BotFactory->LocationSelection, (int)BotFactory->TypeSelection);
}
