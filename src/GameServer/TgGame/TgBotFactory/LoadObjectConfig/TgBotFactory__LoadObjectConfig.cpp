#include "src/GameServer/TgGame/TgBotFactory/LoadObjectConfig/TgBotFactory__LoadObjectConfig.hpp"
#include "src/GameServer/TgGame/TgActorFactory/LoadObjectConfig/TgActorFactory__LoadObjectConfig.hpp"
#include "src/GameServer/Engine/MapObjectConfig/MapObjectConfig.hpp"
#include "src/GameServer/GameModes/SuperAgent/SuperAgent.hpp"
#include "src/GameServer/GameModes/Hardcore/Hardcore.hpp"
#include "src/GameServer/Maps/MapAdditions/MapAdditions.hpp"
#include "src/Database/Database.hpp"
#include "src/Config/Config.hpp"
#include "src/Utils/Logger/Logger.hpp"

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <map>
#include <random>
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
	// Hardcore Security (5000) is custom the same way.
	// skal: generalized for any game mode
	const int requestedDifficulty = primaryDifficulty;
	const bool isCustomTier = (requestedDifficulty > 1471);
	if (isCustomTier) {
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
	if (isCustomTier) {
		cascade.insert(cascade.begin(), requestedDifficulty);
	}

	return cascade;
}

// Super Agent composite tables: REBUILD each target table by concatenating the
// groups of its source tables (renumbered into a fresh 0..N sequence). Runs
// after the cascade so the source tables are already loaded. Difficulty-gated to
// the custom mode so normal play is untouched.
//
// Every source is read from a SNAPSHOT of the base tables taken before any
// rewrite, so a source ALWAYS expands to that table's ORIGINAL rows — even when
// it is itself a composite target (e.g. `{33,{33,102,102}}` + `{102,{33,102}}`,
// where 33 and 102 reference each other). No recursion and no dependence on the
// order targets happen to be processed in: a composite is always
// "original(srcA) + original(srcB) + …". That's the point — keep the map's
// baked spawns (a target listing ITSELF keeps its original groups) and ADD to
// them. A source that is a target expands to the ORIGINAL, never the other
// composite, so `{33,…102…}` gets base-102, not 102's Super-Agent wave.
// Hardcore Security uses the same mechanism with its own list.
void ApplyCompositeTables() {
	const std::map<int, std::vector<int>>* composites =
		SuperAgent::IsActive() ? &SuperAgent::SpawnTableComposites()
		: Hardcore::IsActive() ? &Hardcore::SpawnTableComposites()
		: nullptr;
	if (!composites) return;
	const auto base = g_spawnTables;   // snapshot of the un-composited tables
	for (const auto& comp : *composites) {
		const int target = comp.first;
		std::map<int, std::vector<SpawnTableEntry>> combined;
		int nextGroup = 0;
		for (const int src : comp.second) {
			const auto it = base.find(src);   // ALWAYS the original rows
			if (it == base.end()) {
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

	// Load one difficulty pass into g_spawnTables
	// loadedTables is used to track what's already loaded at a higher difficulty
	// returns the number of entries actually loadeded and the number of new tables
	//
	// Sources both asm_data_set_bot_spawn_tables (verbatim game data, read-only)
	// and mod_data_set_bot_spawn_tables (our custom replacements/additions)
	// mod_data_set_bot_spawn_tables has precendence in that if a table is defined in it,
	// related asm_data_set_bot_spawn_tables entries are NOT loadded

	struct LoadStats {
		int NbEntry;
		int NbStockTable;
		int NbOverridenTable;
		LoadStats ():NbEntry (0),NbStockTable (0),NbOverridenTable (0) {}
	};

	std::set<int> loadedTables;

	const auto LoadSpawnTableRows=[&](int Diff) -> LoadStats {

		// COALESCE(bbm,1.0) > 0 — asm_data_set_bots.bot_balance_multiplier=0 is the
		// "never spawn this bot" sentinel (player pets, decoys, turrets, intentional
		// boss skips like Vulcan); orphan rows where the LEFT JOIN misses keep the
		// row by defaulting to 1.0.

		// Step 1: find out which spawn_table_id are defined for this difficulty
		// tldr; we do select all spawn_table_id that are involved with the given difficulty and whose bots are not disabled
		// and build a set with those

		std::set<int> DiffTables;

		{
			static const char* const QueryString=
				"SELECT DISTINCT(u.bot_spawn_table_id) "
				"FROM ( "
				"  SELECT bot_spawn_table_id, difficulty_value_id, enemy_bot_id "
				"  FROM asm_data_set_bot_spawn_tables "
				"  UNION ALL "
				"  SELECT bot_spawn_table_id, difficulty_value_id, enemy_bot_id "
				"  FROM mod_data_set_bot_spawn_tables "
				") AS u "
				"LEFT JOIN asm_data_set_bots b ON u.enemy_bot_id = b.bot_id "
				"WHERE u.difficulty_value_id = ? "
				"  AND COALESCE(b.bot_balance_multiplier, 1.0) > 0";

			sqlite3_stmt* stmt = nullptr;
			const int rc = sqlite3_prepare_v2(db, QueryString, -1, &stmt, nullptr);
			if (rc != SQLITE_OK || !stmt) {
				Logger::Log("tgbotfactory",
					"  LoadSpawnTableRows(step1) prepare failed (difficulty=%d): %s\n",
					Diff, sqlite3_errmsg(db));
				return LoadStats ();
			}
			sqlite3_bind_int(stmt, 1, Diff);
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				const int tableId = sqlite3_column_int(stmt, 0);
				if (loadedTables.count(tableId)) continue;
				DiffTables.insert(tableId);
			}
			sqlite3_finalize(stmt);
		}

		// Step 2: load the actual spawn tables
		// walk the set to load each spawn table by id
		// this is itself a 2-steps process:
		// 	load from mod_data_set_bot_spawn_tables (our custom overrides)
		// 	if that didn't load anything, load from asm_data_set_bot_spawn_tables (the stock client-extracted data)

		static const char* const QueryString1=
			"SELECT u.spawn_group, u.enemy_bot_id, u.bot_count, "
			"       u.spawn_chance, u.bot_balance_multiplier, COALESCE(b.reference_name, ''), "
			"       u.spawn_group_min, u.spawn_group_max, u.spawn_group_respawn_sec "
			"FROM ( "
			"  SELECT bot_spawn_table_id, difficulty_value_id, spawn_group, "
			"         enemy_bot_id, bot_count, spawn_chance, COALESCE (bot_balance_multiplier, 1.0) as bot_balance_multiplier, "
			"         spawn_group_min, spawn_group_max, spawn_group_respawn_sec "
			"  FROM mod_data_set_bot_spawn_tables "
			") AS u "
			"LEFT JOIN asm_data_set_bots b ON u.enemy_bot_id = b.bot_id "
			"WHERE u.bot_spawn_table_id = ? "
			"  AND u.difficulty_value_id = ? "
			"  AND COALESCE(b.bot_balance_multiplier, 1.0) > 0";

		static const char* const QueryString2=
			"SELECT u.spawn_group, u.enemy_bot_id, u.bot_count, "
			"       u.spawn_chance, u.bot_balance_multiplier, COALESCE(b.reference_name, ''), "
			"       u.spawn_group_min, u.spawn_group_max, u.spawn_group_respawn_sec "
			"FROM ( "
			"  SELECT bot_spawn_table_id, difficulty_value_id, spawn_group, "
			"         enemy_bot_id, bot_count, spawn_chance, COALESCE (bot_balance_multiplier, 1.0) as bot_balance_multiplier, "
			"         spawn_group_min, spawn_group_max, spawn_group_respawn_sec "
			"  FROM asm_data_set_bot_spawn_tables "
			") AS u "
			"LEFT JOIN asm_data_set_bots b ON u.enemy_bot_id = b.bot_id "
			"WHERE u.bot_spawn_table_id = ? "
			"  AND u.difficulty_value_id = ? "
			"  AND COALESCE(b.bot_balance_multiplier, 1.0) > 0";

		LoadStats Stats;

		for (const int& tableId : DiffTables) {

			auto& groupMap = g_spawnTables[tableId];
			const bool isNewTable = groupMap.empty();
			enum class eRC { Error, Empty, NotEmpty };
			const auto query=[&](const char* QueryString,int& NbNewTable) -> eRC {
				sqlite3_stmt* stmt = nullptr;
				const int rc = sqlite3_prepare_v2(db, QueryString, -1, &stmt, nullptr);
				if (rc != SQLITE_OK || !stmt) {
					Logger::Log("tgbotfactory",
						"  LoadSpawnTableRows(step2) prepare failed (difficulty=%d): %s\n",
						Diff, sqlite3_errmsg(db));
					return eRC::Error;
				}
				sqlite3_bind_int(stmt, 1, tableId);
				sqlite3_bind_int(stmt, 2, Diff);
				int NbEntry = 0;
				while (sqlite3_step(stmt) == SQLITE_ROW) {
					const int group   = sqlite3_column_int(stmt, 0);
					const int botId   = sqlite3_column_int(stmt, 1);
					const int count   = sqlite3_column_int(stmt, 2);
					const float chance = static_cast<float>(sqlite3_column_double(stmt, 3));
					const float bbm = static_cast<float>(sqlite3_column_double(stmt, 4));
					const unsigned char* refNameRaw = sqlite3_column_text(stmt, 5);
					const std::string refName(refNameRaw ? reinterpret_cast<const char*>(refNameRaw) : "");
					const int groupMin   = sqlite3_column_int(stmt, 6);
					const int groupMax   = sqlite3_column_int(stmt, 7);
					const int respawnSec = sqlite3_column_int(stmt, 8);
					groupMap[group].push_back(SpawnTableEntry{
						tableId, group, botId, count, chance, bbm, refName, groupMin, groupMax, respawnSec
					});
					++NbEntry;
				}
				sqlite3_finalize(stmt);
				if (NbEntry) {
					Stats.NbEntry+=NbEntry;
					if (isNewTable) ++NbNewTable;
					return eRC::NotEmpty;
				}
				else {
					return eRC::Empty;
				}
			};
			if (query (QueryString1,Stats.NbOverridenTable) == eRC::Empty) {
				query (QueryString2,Stats.NbStockTable);
			}
		}
		return Stats;
	};

	const std::vector<int> cascade = GetDifficultyCascade(db, difficulty);

	const auto LoadCascade=[&](int Index,const char* const Tail) {
		const int tier = cascade[Index];
		const LoadStats Stats=LoadSpawnTableRows(tier);
		Logger::Log("tgbotfactory",
			"  cascade tier %zu difficulty=%d -> %d entries / %d stock tables / %d overriden tables%s\n",
			Index, tier, Stats.NbEntry, Stats.NbStockTable, Stats.NbOverridenTable, Tail);
	};

	LoadCascade (0, " (primary)");

	for (size_t i = 1; i < cascade.size(); ++i) {
		for (const auto& kv : g_spawnTables) loadedTables.insert(kv.first);
		LoadCascade (i, "");
	}

	// Rebuild composite tables from the now-loaded raw tables (Super Agent only).
	ApplyCompositeTables();

	Logger::Log("tgbotfactory",
		"  EnsureSpawnTablesLoaded: difficulty=%d cascade depth=%zu total tables=%zu\n",
		difficulty, cascade.size(), g_spawnTables.size());
}

// Private RNG for spawn rolls. Do NOT use CRT rand(): the DLL shares
// msvcrt's rand state with the game binary, which can re-seed srand()
// deterministically during map load — observed as the SAME boss rolled from
// an 80/20 table on 7 consecutive Ultra-Max runs. mt19937 state is ours alone.
std::mt19937& SpawnRng() {
	static std::mt19937 gen(
		static_cast<unsigned>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
	return gen;
}
float SpawnRollFloat(float span) {
	return std::uniform_real_distribution<float>(0.0f, span)(SpawnRng());
}
int SpawnRollInt(int lo, int hi) {  // inclusive
	return std::uniform_int_distribution<int>(lo, hi)(SpawnRng());
}
void EnsureRandSeeded() {}  // superseded by SpawnRng; kept for call sites

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
	float roll = SpawnRollFloat(span);
	for (const auto& r : rows) {
		if (roll < r.SpawnChance) return &r;
		roll -= r.SpawnChance;
	}
	return nullptr;  // landed in the empty remainder
}

}  // namespace

// skal: replace simple int return (botid) by {botid,bbm}
// previous empty (zero) return moved to {0,0.0f}
BotIdEntry TgBotFactory__LoadObjectConfig::PickBotFromSpawnTableGroup(int nSpawnTableId, int nSpawnGroup, int* outBotCount) {
	EnsureSpawnTablesLoaded();
	EnsureRandSeeded();

	if (outBotCount) *outBotCount = 1;

	auto tableIt = g_spawnTables.find(nSpawnTableId);
	if (tableIt == g_spawnTables.end()) return BotIdEntry();
	auto groupIt = tableIt->second.find(nSpawnGroup);
	if (groupIt == tableIt->second.end() || groupIt->second.empty()) return BotIdEntry();

	// Spawn-time pick: the group's existence was already decided at queue
	// build (RollSpawnPlan), so re-roll among the rows WITHOUT the empty
	// outcome — normalise by the chance total.
	float total = 0.0f;
	for (const auto& r : groupIt->second) total += r.SpawnChance;
	if (total <= 0.0f) return BotIdEntry();
	float roll = SpawnRollFloat(total);
	const SpawnTableEntry* picked = &groupIt->second.back();
	for (const auto& r : groupIt->second) {
		if (roll < r.SpawnChance) { picked = &r; break; }
		roll -= r.SpawnChance;
	}
	if (outBotCount) *outBotCount = picked->BotCount > 0 ? picked->BotCount : 1;
	return BotIdEntry (picked->EnemyBotId,picked->BBM);
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
		gp.BBM = 0.0f;
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
				gp.EntryCount = (hi > lo) ? SpawnRollInt(lo, hi) : lo;
				gp.Detail.nMinCount = lo;
				gp.Detail.nMaxCount = hi;
			} else {
				// bot_count group: ONE roll fields the whole group — every
				// entry spawns the rolled bot (vanilla per-group semantics).
				gp.EntryCount  = row->BotCount > 0 ? row->BotCount : 1;
				gp.RolledBotId = row->EnemyBotId;
				gp.BBM         = row->BBM;
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
// skal: replace botid with {botid,bbm}
static std::map<uint64_t, std::vector<BotIdEntry>> g_factoryGroupRolls;

static uint64_t FactoryKey(ATgBotFactory* f) {
	uint32_t number = 0;
	std::memcpy(&number, ((const char*)&f->Name) + 4, sizeof(number));
	return (static_cast<uint64_t>(static_cast<uint32_t>(f->Name.Index)) << 32) | number;
}

void TgBotFactory__LoadObjectConfig::SetFactoryGroupRolls(
		ATgBotFactory* Factory, const std::vector<BotIdEntry>& rolls) {
	if (Factory == nullptr) return;
	g_factoryGroupRolls[FactoryKey(Factory)] = rolls;
}

BotIdEntry TgBotFactory__LoadObjectConfig::GetFactoryGroupRoll(
		ATgBotFactory* Factory, int nGroupIndex) {
	if (Factory == nullptr || nGroupIndex < 0) return BotIdEntry();
	auto it = g_factoryGroupRolls.find(FactoryKey(Factory));
	if (it == g_factoryGroupRolls.end()) return BotIdEntry();
	if (nGroupIndex >= static_cast<int>(it->second.size())) return BotIdEntry();
	return it->second[nGroupIndex];
}

// Escape-wave combined plans repeat group indices past the table's group
// count — this map carries their index->group-VALUE resolution per factory.
static std::map<uint64_t, std::vector<int>> g_factoryGroupValues;

void TgBotFactory__LoadObjectConfig::SetFactoryGroupValues(
		ATgBotFactory* Factory, const std::vector<int>& values) {
	if (Factory == nullptr) return;
	g_factoryGroupValues[FactoryKey(Factory)] = values;
}

void TgBotFactory__LoadObjectConfig::ClearFactoryGroupValues(ATgBotFactory* Factory) {
	if (Factory == nullptr) return;
	g_factoryGroupValues.erase(FactoryKey(Factory));
}

int TgBotFactory__LoadObjectConfig::GetFactoryGroupValue(
		ATgBotFactory* Factory, int nGroupIndex) {
	if (Factory == nullptr || nGroupIndex < 0) return -1;
	auto it = g_factoryGroupValues.find(FactoryKey(Factory));
	if (it == g_factoryGroupValues.end()) return -1;
	if (nGroupIndex >= static_cast<int>(it->second.size())) return -1;
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

	// Hand-authored factory (MapAdditions) — spec first, DB overrides on top.
	MapAdditions::ApplyPendingBotFactory(BotFactory);

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
