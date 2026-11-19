#include "src/ControlServer/Database/Database.hpp"
#include "src/ControlServer/Database/Migrations.hpp"
#include "src/ControlServer/Logger.hpp"
#include "sqlite3.h"
#include <string>
#include <vector>
#include <map>
#include <atomic>
#include <ctime>
#include <algorithm>
#include <chrono>
#include <random>
#include <utility>
#include <set>

// Bumped on every ga_friends mutation; starts at 1 so a fresh ChatSession
// cache (epoch 0) always loads on first use.
static std::atomic<uint64_t> g_friends_epoch{1};

sqlite3* Database::connection = nullptr;
std::string Database::db_path_ = "server.db";
static std::string g_base_db = "dbbase/base.db";

void Database::SetDbPath(const std::string& path) { db_path_ = path; }

void Database::SetBaseDb(const std::string& base_db) {
    if (!base_db.empty()) g_base_db = base_db;
}

sqlite3* Database::GetConnection() {
	if (Database::connection == nullptr) {
		int result = sqlite3_open(db_path_.c_str(), &connection);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to open database: %s\n", sqlite3_errmsg(connection));
			return nullptr;
		}
		sqlite3_exec(connection, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
		sqlite3_exec(connection, "PRAGMA busy_timeout=5000;", nullptr, nullptr, nullptr);
		sqlite3_exec(connection, "PRAGMA synchronous=NORMAL;", nullptr, nullptr, nullptr);
		// Cap WAL post-checkpoint size to 64 MiB — without this, the file is
		// only ever reset to the start, never shrunk, so its size monotonically
		// grows to the historical high-water mark (the 301 MB prod symptom).
		sqlite3_exec(connection, "PRAGMA journal_size_limit=67108864;", nullptr, nullptr, nullptr);

		// Query-rate meter, channel "dbperf" (off unless enabled in
		// control-server.json). One trace hook counts every statement, and the
		// count is dumped once per second — cheaper and more honest than
		// instrumenting call sites, and it covers code we didn't write.
		sqlite3_trace_v2(connection, SQLITE_TRACE_STMT,
			[](unsigned, void*, void*, void*) -> int {
				static std::atomic<long> count{0};
				static std::atomic<long long> window_start{0};
				const long long now = (long long)time(nullptr);
				long long start = window_start.load();
				if (start == 0) { window_start.store(now); start = now; }
				count.fetch_add(1);
				if (now > start) {
					Logger::Log("dbperf", "[DB] %ld statements in %llds\n",
						count.exchange(0), (long long)(now - start));
					window_start.store(now);
				}
				return 0;
			}, nullptr);
	}

	return Database::connection;
}

void Database::CloseConnection() {
	if (Database::connection != nullptr) {
		sqlite3_exec(Database::connection, "PRAGMA wal_checkpoint(TRUNCATE);", nullptr, nullptr, nullptr);
	}
	sqlite3_close(Database::connection);
	Database::connection = nullptr;
}

int Database::Callback(void* data, int argc, char** argv, char** azColName) {

	std::vector<std::map<std::string, std::string>>* dataMap = static_cast<std::vector<std::map<std::string, std::string>>*>(data);

	std::map<std::string, std::string> row;

	for (int i = 0; i < argc; i++) {
		row[azColName[i]] = argv[i] ? argv[i] : "";
	}

	dataMap->push_back(row);

	return 0;
}

void Database::Init() {
	sqlite3* db = GetConnection();
	if (!db) {
		Logger::Log("db", "Database::Init skipped: no DB connection\n");
		return;
	}

	// Schema bootstrap. This process is the only one that migrates a database:
	// a database with no schema is replaced by the baseline, then forward .sql
	// migrations are applied. The ~3,000-line version-gated ladder that used to
	// live here is baked into dbbase/base.db -- every block in it describes a
	// change that baseline already contains.
	const auto boot = Migrations::Run(db, db_path_, g_base_db);
	if (!boot.ok) {
		Logger::Log("db", "[Database::Init] schema bootstrap failed: %s\n",
			boot.error.c_str());
		return;
	}
	if (boot.installed_base) {
		Logger::Log("db", "[Database::Init] installed baseline, now at version %d\n",
			boot.to_version);
	} else if (boot.applied > 0) {
		Logger::Log("db", "[Database::Init] applied %d migration(s): %d -> %d\n",
			boot.applied, boot.from_version, boot.to_version);
	} else {
		Logger::Log("db", "[Database::Init] schema at version %d\n", boot.to_version);
	}

	// NOTE: PlayerSessionStore::Init() is called separately from main.cpp -- not here.
}

std::string Database::GetQuestStatus(int64_t character_id, int quest_id) {
	sqlite3* db = GetConnection();
	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"SELECT status FROM ga_character_quests WHERE character_id = ? AND quest_id = ?",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[Quest] GetQuestStatus prepare failed: %s\n", sqlite3_errmsg(db));
		return "";
	}
	sqlite3_bind_int64(stmt, 1, character_id);
	sqlite3_bind_int(stmt, 2, quest_id);

	std::string status;
	if (sqlite3_step(stmt) == SQLITE_ROW) {
		const char* s = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
		if (s) status = s;
	}
	sqlite3_finalize(stmt);
	return status;
}

void Database::AcceptQuest(int64_t character_id, int quest_id) {
	sqlite3* db = GetConnection();
	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"INSERT OR IGNORE INTO ga_character_quests (character_id, quest_id, status) VALUES (?, ?, 'active')",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[Quest] AcceptQuest prepare failed: %s\n", sqlite3_errmsg(db));
		return;
	}
	sqlite3_bind_int64(stmt, 1, character_id);
	sqlite3_bind_int(stmt, 2, quest_id);
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	Logger::Log("db", "[Quest] Accepted quest %d for character %lld\n", quest_id, character_id);
}

void Database::CompleteQuest(int64_t character_id, int quest_id) {
	sqlite3* db = GetConnection();
	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"UPDATE ga_character_quests SET status = 'complete', completed_at = strftime('%s','now') "
		"WHERE character_id = ? AND quest_id = ?",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[Quest] CompleteQuest prepare failed: %s\n", sqlite3_errmsg(db));
		return;
	}
	sqlite3_bind_int64(stmt, 1, character_id);
	sqlite3_bind_int(stmt, 2, quest_id);
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	Logger::Log("db", "[Quest] Completed quest %d for character %lld\n", quest_id, character_id);
}

void Database::AbandonQuest(int64_t character_id, int quest_id) {
	sqlite3* db = GetConnection();
	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"DELETE FROM ga_character_quests WHERE character_id = ? AND quest_id = ?",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[Quest] AbandonQuest prepare failed: %s\n", sqlite3_errmsg(db));
		return;
	}
	sqlite3_bind_int64(stmt, 1, character_id);
	sqlite3_bind_int(stmt, 2, quest_id);
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	Logger::Log("db", "[Quest] Abandoned quest %d for character %lld\n", quest_id, character_id);
}

// Kill (1428) and Interact with Volume (1431) both keep their progress in
// ga_character_quest_progress and differ only in which target column of
// asm_data_set_quest_requirements the event matches on. One body, two entry
// points — so a fix to the counter logic can't reach only half of them.
//
// `target_column` is a fixed literal chosen by the two callers below, never
// anything that came off the wire.
static std::vector<Database::QuestProgress> CreditStoredRequirements(
		int64_t character_id, int requirement_type, const char* target_column,
		int target_id, const char* label) {
	std::vector<Database::QuestProgress> moved;
	if (character_id <= 0 || target_id <= 0) return moved;

	sqlite3* db = Database::GetConnection();
	if (!db) return moved;

	// Candidate requirements: matching type + target, on a quest this character
	// has ACTIVE, and not already satisfied. The LEFT JOIN supplies 0 for
	// requirements with no counter row yet.
	const std::string sql =
		"SELECT r.quest_id, r.quest_requirement_id, r.count, "
		"       COALESCE(p.count, 0) "
		"FROM asm_data_set_quest_requirements r "
		"JOIN ga_character_quests q "
		"  ON q.quest_id = r.quest_id AND q.character_id = ? AND q.status = 'active' "
		"LEFT JOIN ga_character_quest_progress p "
		"  ON p.character_id = q.character_id "
		" AND p.quest_requirement_id = r.quest_requirement_id "
		"WHERE r.requirement_type_value_id = " + std::to_string(requirement_type) +
		"  AND r." + target_column + " = ? "
		"  AND COALESCE(p.count, 0) < r.count";

	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[Quest] Credit%s prepare failed: %s\n", label, sqlite3_errmsg(db));
		return moved;
	}
	sqlite3_bind_int64(stmt, 1, character_id);
	sqlite3_bind_int(stmt, 2, target_id);

	while (sqlite3_step(stmt) == SQLITE_ROW) {
		Database::QuestProgress pr;
		pr.quest_id             = sqlite3_column_int(stmt, 0);
		pr.quest_requirement_id = sqlite3_column_int(stmt, 1);
		pr.required             = sqlite3_column_int(stmt, 2);
		const int current       = sqlite3_column_int(stmt, 3);

		pr.count = current + 1;
		if (pr.required > 0 && pr.count > pr.required) pr.count = pr.required;
		pr.completed = (pr.required > 0 && pr.count >= pr.required);
		moved.push_back(pr);
	}
	sqlite3_finalize(stmt);

	for (const auto& pr : moved) {
		sqlite3_stmt* up = nullptr;
		rc = sqlite3_prepare_v2(db,
			"INSERT INTO ga_character_quest_progress "
			"  (character_id, quest_id, quest_requirement_id, count, updated_at) "
			"VALUES (?, ?, ?, ?, strftime('%s','now')) "
			"ON CONFLICT(character_id, quest_requirement_id) DO UPDATE SET "
			"  count = excluded.count, updated_at = excluded.updated_at",
			-1, &up, nullptr);
		if (rc != SQLITE_OK || !up) {
			Logger::Log("db", "[Quest] Credit%s upsert prepare failed: %s\n",
				label, sqlite3_errmsg(db));
			continue;
		}
		sqlite3_bind_int64(up, 1, character_id);
		sqlite3_bind_int(up, 2, pr.quest_id);
		sqlite3_bind_int(up, 3, pr.quest_requirement_id);
		sqlite3_bind_int(up, 4, pr.count);
		sqlite3_step(up);
		sqlite3_finalize(up);

		Logger::Log("quest",
			"[Quest] %s credit char=%lld target=%d quest=%d req=%d -> %d/%d%s\n",
			label, (long long)character_id, target_id, pr.quest_id,
			pr.quest_requirement_id, pr.count, pr.required,
			pr.completed ? " (requirement complete)" : "");
	}

	return moved;
}

std::vector<Database::QuestProgress> Database::CreditKillQuestRequirements(int64_t character_id,
                                                                          int bot_id) {
	return CreditStoredRequirements(character_id, 1428, "target_bot_id", bot_id, "Kill");
}

std::vector<Database::QuestProgress> Database::CreditVolumeQuestRequirements(int64_t character_id,
                                                                            int ui_volume_id) {
	return CreditStoredRequirements(character_id, 1431, "target_ui_volume_id", ui_volume_id,
	                                "Volume");
}

// ===========================================================================
// Loot rolls + quest requirement evaluation.
// Components are never real items — a drop is credited straight onto the
// Collect requirement that wanted it. See Database.hpp for the rationale.
// ===========================================================================

static int LookupIntColumn(const char* sql, int key) {
	sqlite3* db = Database::GetConnection();
	if (!db) return 0;
	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK || !stmt) return 0;
	sqlite3_bind_int(stmt, 1, key);
	int value = 0;
	if (sqlite3_step(stmt) == SQLITE_ROW) value = sqlite3_column_int(stmt, 0);
	sqlite3_finalize(stmt);
	return value;
}

int Database::GetQuestRewardLootTableId(int quest_id) {
	if (quest_id <= 0) return 0;
	return LookupIntColumn(
		"SELECT COALESCE(loot_table_id, 0) FROM asm_data_set_quests WHERE quest_id = ?", quest_id);
}

int Database::GetBotLootTableId(int bot_id) {
	if (bot_id <= 0) return 0;
	return LookupIntColumn(
		"SELECT COALESCE(loot_table_id, 0) FROM asm_data_set_bots WHERE bot_id = ?", bot_id);
}

namespace {

// Private RNG for drop rolls. Not CRT rand().
std::mt19937& LootRng() {
	static std::mt19937 gen(
		static_cast<unsigned>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
	return gen;
}
float Roll100() { return std::uniform_real_distribution<float>(0.0f, 100.0f)(LootRng()); }

constexpr int kMaxLootDepth = 4;

void RollLootInto(sqlite3* db, int loot_table_id, int depth,
                  std::map<int, int>& out, std::set<int>& visited) {
	if (loot_table_id <= 0 || depth > kMaxLootDepth) return;
	if (!visited.insert(loot_table_id).second) return;

	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"SELECT li.item_id, li.sub_loot_table_id, li.quantity, li.drop_chance, "
		"       COALESCE(i.item_type_value_id, 0) "
		"FROM asm_data_set_loot_table_items li "
		"LEFT JOIN asm_data_set_items i ON i.item_id = li.item_id "
		"WHERE li.loot_table_id = ? ORDER BY li.sort_order",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("loot", "[Loot] roll prepare failed (table=%d): %s\n",
			loot_table_id, sqlite3_errmsg(db));
		return;
	}
	sqlite3_bind_int(stmt, 1, loot_table_id);

	while (sqlite3_step(stmt) == SQLITE_ROW) {
		const int   itemId   = sqlite3_column_int(stmt, 0);
		const int   subTable = sqlite3_column_int(stmt, 1);
		const int   quantity = sqlite3_column_int(stmt, 2);
		const float chance   = static_cast<float>(sqlite3_column_double(stmt, 3));
		const int   itemType = sqlite3_column_int(stmt, 4);

		if (chance < 100.0f && Roll100() >= chance) continue;
		if (subTable > 0) { RollLootInto(db, subTable, depth + 1, out, visited); continue; }

		// Only components matter — nothing else has anywhere to go. Blueprints,
		// appearance items and consumables are rolled (so the odds stay
		// faithful) and then discarded.
		if (itemId <= 0 || itemType != 1171) continue;
		out[itemId] += (quantity > 0 ? quantity : 1);
	}
	sqlite3_finalize(stmt);
}

}  // namespace

std::map<int, int> Database::RollLootTableComponents(int loot_table_id) {
	std::map<int, int> out;
	sqlite3* db = GetConnection();
	if (!db || loot_table_id <= 0) return out;
	std::set<int> visited;
	RollLootInto(db, loot_table_id, 0, out, visited);
	return out;
}

std::vector<Database::ComponentGrant> Database::GrantComponents(
		int64_t user_id, const std::map<int, int>& items) {
	std::vector<ComponentGrant> granted;
	if (user_id <= 0 || items.empty()) return granted;
	sqlite3* db = GetConnection();
	if (!db) return granted;

	for (const auto& kv : items) {
		const int item_id = kv.first;
		const int qty     = kv.second > 0 ? kv.second : 1;
		if (item_id <= 0) continue;

		ComponentGrant g;
		g.item_id  = item_id;
		g.quantity = qty;

		// A live row means the client already has a map entry for this stack, so
		// the grant only changes INSTANCE_COUNT. No live row means a NEW entry,
		// and r_ItemCount has to grow by one. Mirrors GetAllComponents' filter —
		// the two must agree or the count drifts.
		bool exists = false;
		{
			sqlite3_stmt* q = nullptr;
			if (sqlite3_prepare_v2(db,
					"SELECT 1 FROM ga_user_components "
					"WHERE user_id = ? AND item_id = ? AND quantity > 0",
					-1, &q, nullptr) == SQLITE_OK && q) {
				sqlite3_bind_int64(q, 1, user_id);
				sqlite3_bind_int(q, 2, item_id);
				exists = (sqlite3_step(q) == SQLITE_ROW);
				sqlite3_finalize(q);
			}
		}
		g.new_row = !exists;

		sqlite3_stmt* up = nullptr;
		if (sqlite3_prepare_v2(db,
				"INSERT INTO ga_user_components (user_id, item_id, quantity, updated_at) "
				"VALUES (?, ?, ?, strftime('%s','now')) "
				"ON CONFLICT(user_id, item_id) DO UPDATE SET "
				"  quantity = quantity + excluded.quantity, updated_at = excluded.updated_at",
				-1, &up, nullptr) != SQLITE_OK || !up) {
			continue;
		}
		sqlite3_bind_int64(up, 1, user_id);
		sqlite3_bind_int(up, 2, item_id);
		sqlite3_bind_int(up, 3, qty);
		sqlite3_step(up);
		sqlite3_finalize(up);

		g.new_total = GetComponentCount(user_id, item_id);
		g.quality_value_id = LookupIntColumn(
			"SELECT COALESCE(quality_value_id, 0) FROM asm_data_set_items WHERE item_id = ?",
			item_id);
		granted.push_back(g);

		Logger::Log("loot", "[Loot] user=%lld component %d +%d -> %d%s\n",
			(long long)user_id, item_id, qty, g.new_total,
			g.new_row ? " (new inventory row)" : "");
	}
	return granted;
}

int Database::GetExpectedItemCount(int64_t user_id, int profile_id) {
	if (user_id <= 0) return 0;
	sqlite3* db = GetConnection();
	if (!db) return 0;

	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db,
			"SELECT (SELECT COUNT(*) FROM ga_players_inventory "
			"        WHERE user_id = ?1 AND (profile_id = 0 OR profile_id = ?2)) "
			"     + (SELECT COUNT(*) FROM ga_user_components "
			"        WHERE user_id = ?1 AND quantity > 0)",
			-1, &stmt, nullptr) != SQLITE_OK || !stmt) {
		return 0;
	}
	sqlite3_bind_int64(stmt, 1, user_id);
	sqlite3_bind_int(stmt, 2, profile_id);
	int total = 0;
	if (sqlite3_step(stmt) == SQLITE_ROW) total = sqlite3_column_int(stmt, 0);
	sqlite3_finalize(stmt);
	return total;
}

int Database::GetComponentCount(int64_t user_id, int item_id) {
	if (user_id <= 0 || item_id <= 0) return 0;
	sqlite3* db = GetConnection();
	if (!db) return 0;
	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db,
			"SELECT quantity FROM ga_user_components WHERE user_id = ? AND item_id = ?",
			-1, &stmt, nullptr) != SQLITE_OK || !stmt) {
		return 0;
	}
	sqlite3_bind_int64(stmt, 1, user_id);
	sqlite3_bind_int(stmt, 2, item_id);
	int qty = 0;
	if (sqlite3_step(stmt) == SQLITE_ROW) qty = sqlite3_column_int(stmt, 0);
	sqlite3_finalize(stmt);
	return qty;
}

std::vector<Database::ComponentRow> Database::GetAllComponents(int64_t user_id) {
	std::vector<ComponentRow> rows;
	if (user_id <= 0) return rows;
	sqlite3* db = GetConnection();
	if (!db) return rows;

	// Depleted stacks are deleted, not kept at zero — a `0 Units` row in the
	// player's bag list is not a thing retail ever showed. The filter also
	// covers legacy rows predating migration v165.
	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db,
			"SELECT c.item_id, c.quantity, COALESCE(i.quality_value_id, 0) "
			"FROM ga_user_components c "
			"LEFT JOIN asm_data_set_items i ON i.item_id = c.item_id "
			"WHERE c.user_id = ? AND c.quantity > 0 ORDER BY c.item_id",
			-1, &stmt, nullptr) != SQLITE_OK || !stmt) {
		return rows;
	}
	sqlite3_bind_int64(stmt, 1, user_id);
	while (sqlite3_step(stmt) == SQLITE_ROW) {
		ComponentRow r;
		r.item_id          = sqlite3_column_int(stmt, 0);
		r.quantity         = sqlite3_column_int(stmt, 1);
		r.quality_value_id = sqlite3_column_int(stmt, 2);
		rows.push_back(r);
	}
	sqlite3_finalize(stmt);
	return rows;
}

std::vector<int> Database::ConsumeQuestRequirementItems(int64_t user_id, int quest_id) {
	std::vector<int> removed;
	if (user_id <= 0) return removed;
	sqlite3* db = GetConnection();
	if (!db) return removed;

	for (const auto& r : GetQuestRequirements(quest_id)) {
		if (r.requirement_type_value_id != 1429 || r.target_item_id <= 0) continue;
		const int need = r.count > 0 ? r.count : 1;
		if (GetComponentCount(user_id, r.target_item_id) <= 0) continue;  // not a component we hold

		sqlite3_stmt* stmt = nullptr;
		if (sqlite3_prepare_v2(db,
				"UPDATE ga_user_components SET quantity = MAX(0, quantity - ?), "
				"       updated_at = strftime('%s','now') "
				"WHERE user_id = ? AND item_id = ?",
				-1, &stmt, nullptr) != SQLITE_OK || !stmt) {
			continue;
		}
		sqlite3_bind_int(stmt, 1, need);
		sqlite3_bind_int64(stmt, 2, user_id);
		sqlite3_bind_int(stmt, 3, r.target_item_id);
		sqlite3_step(stmt);
		sqlite3_finalize(stmt);

		const int left = GetComponentCount(user_id, r.target_item_id);
		Logger::Log("loot", "[Loot] turn-in quest=%d spent item=%d x%d -> %d left\n",
			quest_id, r.target_item_id, need, left);

		// Depleted -> the stack stops existing. The caller pushes a STATE=2
		// record for it and restates r_ItemCount.
		if (left <= 0) {
			sqlite3_stmt* del = nullptr;
			if (sqlite3_prepare_v2(db,
					"DELETE FROM ga_user_components WHERE user_id = ? AND item_id = ?",
					-1, &del, nullptr) == SQLITE_OK && del) {
				sqlite3_bind_int64(del, 1, user_id);
				sqlite3_bind_int(del, 2, r.target_item_id);
				sqlite3_step(del);
				sqlite3_finalize(del);
				removed.push_back(r.target_item_id);
				Logger::Log("loot", "[Loot] turn-in quest=%d component %d depleted — row removed\n",
					quest_id, r.target_item_id);
			}
		}
	}
	return removed;
}

int Database::TakeComponents(int64_t user_id, int item_id, int count) {
	if (user_id <= 0 || item_id <= 0 || count <= 0) return 0;
	sqlite3* db = GetConnection();
	if (!db) return 0;

	const int have = GetComponentCount(user_id, item_id);
	if (have <= 0) return 0;
	const int take = have < count ? have : count;

	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db,
			"UPDATE ga_user_components SET quantity = MAX(0, quantity - ?), "
			"       updated_at = strftime('%s','now') "
			"WHERE user_id = ? AND item_id = ?",
			-1, &stmt, nullptr) == SQLITE_OK && stmt) {
		sqlite3_bind_int(stmt, 1, take);
		sqlite3_bind_int64(stmt, 2, user_id);
		sqlite3_bind_int(stmt, 3, item_id);
		sqlite3_step(stmt);
		sqlite3_finalize(stmt);
	}

	if (GetComponentCount(user_id, item_id) <= 0) {
		sqlite3_stmt* del = nullptr;
		if (sqlite3_prepare_v2(db,
				"DELETE FROM ga_user_components WHERE user_id = ? AND item_id = ?",
				-1, &del, nullptr) == SQLITE_OK && del) {
			sqlite3_bind_int64(del, 1, user_id);
			sqlite3_bind_int(del, 2, item_id);
			sqlite3_step(del);
			sqlite3_finalize(del);
		}
	}

	Logger::Log("loot", "[Loot] -components take user=%lld item=%d x%d -> %d left\n",
		(long long)user_id, item_id, take, GetComponentCount(user_id, item_id));
	return take;
}

std::vector<Database::QuestRequirementRow> Database::GetQuestRequirements(int quest_id) {
	std::vector<QuestRequirementRow> rows;
	if (quest_id <= 0) return rows;
	sqlite3* db = GetConnection();
	if (!db) return rows;

	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db,
			"SELECT quest_requirement_id, requirement_type_value_id, count, "
			"       COALESCE(target_item_id, 0), COALESCE(target_bot_id, 0), "
			"       COALESCE(target_ui_volume_id, 0) "
			"FROM asm_data_set_quest_requirements WHERE quest_id = ?",
			-1, &stmt, nullptr) != SQLITE_OK || !stmt) {
		return rows;
	}
	sqlite3_bind_int(stmt, 1, quest_id);
	while (sqlite3_step(stmt) == SQLITE_ROW) {
		QuestRequirementRow r;
		r.quest_requirement_id      = sqlite3_column_int(stmt, 0);
		r.requirement_type_value_id = sqlite3_column_int(stmt, 1);
		r.count                     = sqlite3_column_int(stmt, 2);
		r.target_item_id            = sqlite3_column_int(stmt, 3);
		r.target_bot_id             = sqlite3_column_int(stmt, 4);
		r.target_ui_volume_id       = sqlite3_column_int(stmt, 5);
		rows.push_back(r);
	}
	sqlite3_finalize(stmt);
	return rows;
}

bool Database::AreQuestRequirementsMet(int64_t user_id, int64_t character_id, int quest_id,
                                       std::vector<std::string>* unmet) {
	const auto reqs = GetQuestRequirements(quest_id);
	bool ok = true;

	for (const auto& r : reqs) {
		const int need = r.count > 0 ? r.count : 1;

		if (r.requirement_type_value_id == 1429) {          // Collect — stock
			// Same source the CLIENT uses (it sums nInstanceCount over its own
			// inventory map), so server and client always agree.
			const int have = GetComponentCount(user_id, r.target_item_id);
			if (have < need) {
				ok = false;
				if (unmet) unmet->push_back("collect item " + std::to_string(r.target_item_id)
					+ ": " + std::to_string(have) + "/" + std::to_string(need));
			}
			continue;
		}

		// Kill (1428) and Interact with Volume (1431) share the counter table.
		if (r.requirement_type_value_id == 1428 ||
		    r.requirement_type_value_id == 1431) {
			sqlite3* db = GetConnection();
			int have = 0;
			if (db) {
				sqlite3_stmt* stmt = nullptr;
				if (sqlite3_prepare_v2(db,
						"SELECT count FROM ga_character_quest_progress "
						"WHERE character_id = ? AND quest_requirement_id = ?",
						-1, &stmt, nullptr) == SQLITE_OK && stmt) {
					sqlite3_bind_int64(stmt, 1, character_id);
					sqlite3_bind_int(stmt, 2, r.quest_requirement_id);
					if (sqlite3_step(stmt) == SQLITE_ROW) have = sqlite3_column_int(stmt, 0);
					sqlite3_finalize(stmt);
				}
			}
			if (have < need) {
				ok = false;
				if (unmet) unmet->push_back("req " + std::to_string(r.quest_requirement_id)
					+ ": " + std::to_string(have) + "/" + std::to_string(need));
			}
		}
		// Every other type (Complete Mission, Finish PVP Mission, Script
		// Triggered) is unimplemented — treat as satisfied so those quests stay
		// completable instead of becoming dead ends.
	}

	return ok;
}

std::vector<Database::CharacterQuestState> Database::GetCharacterQuestStates(int64_t character_id) {
	std::vector<CharacterQuestState> rows;
	if (character_id <= 0) return rows;
	sqlite3* db = GetConnection();
	if (!db) return rows;

	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db,
			"SELECT quest_id, status, COALESCE(completed_at, 0) "
			"FROM ga_character_quests WHERE character_id = ? ORDER BY quest_id",
			-1, &stmt, nullptr) != SQLITE_OK || !stmt) {
		Logger::Log("quest", "[Quest] GetCharacterQuestStates prepare failed: %s\n",
			sqlite3_errmsg(db));
		return rows;
	}
	sqlite3_bind_int64(stmt, 1, character_id);
	while (sqlite3_step(stmt) == SQLITE_ROW) {
		CharacterQuestState s;
		s.quest_id = sqlite3_column_int(stmt, 0);
		const char* st = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
		s.status = st ? st : "";
		s.completed_at = sqlite3_column_int64(stmt, 2);
		rows.push_back(std::move(s));
	}
	sqlite3_finalize(stmt);
	return rows;
}

std::vector<std::pair<int, int>> Database::GetQuestRequirementCounts(int64_t user_id,
                                                                    int64_t character_id,
                                                                    int quest_id) {
	std::vector<std::pair<int, int>> out;

	for (const auto& r : GetQuestRequirements(quest_id)) {
		int have = 0;

		if (r.requirement_type_value_id == 1429) {          // Collect — stock
			have = GetComponentCount(user_id, r.target_item_id);
			if (r.count > 0 && have > r.count) have = r.count;
			out.push_back({ r.quest_requirement_id, have });
			continue;
		}

		// Kill (1428) and Interact with Volume (1431) share the counter table.
		if (r.requirement_type_value_id == 1428 ||
		    r.requirement_type_value_id == 1431) {
			sqlite3* db = GetConnection();
			if (db) {
				sqlite3_stmt* stmt = nullptr;
				if (sqlite3_prepare_v2(db,
						"SELECT count FROM ga_character_quest_progress "
						"WHERE character_id = ? AND quest_requirement_id = ?",
						-1, &stmt, nullptr) == SQLITE_OK && stmt) {
					sqlite3_bind_int64(stmt, 1, character_id);
					sqlite3_bind_int(stmt, 2, r.quest_requirement_id);
					if (sqlite3_step(stmt) == SQLITE_ROW) have = sqlite3_column_int(stmt, 0);
					sqlite3_finalize(stmt);
				}
			}
		} else {
			continue;   // nothing meaningful to replay for unimplemented types
		}

		out.push_back({ r.quest_requirement_id, have });
	}
	return out;
}

// ===========================================================================
// User moderation — session history, bans, dashboard reads.
// ===========================================================================

int64_t Database::InsertSession(int64_t user_id_or_zero,
                                const std::string& username,
                                const std::string& ip,
                                const std::string& outcome) {
	sqlite3* db = GetConnection();
	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"INSERT INTO ga_user_sessions (user_id, username, ip, login_at, logout_at, outcome) "
		"VALUES (?, ?, ?, strftime('%s','now'), "
		"        CASE WHEN ? IN ('rejected','banned') THEN strftime('%s','now') ELSE NULL END, "
		"        ?)",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[Session] InsertSession prepare failed: %s\n", sqlite3_errmsg(db));
		return 0;
	}
	if (user_id_or_zero > 0) {
		sqlite3_bind_int64(stmt, 1, user_id_or_zero);
	} else {
		sqlite3_bind_null(stmt, 1);
	}
	sqlite3_bind_text(stmt, 2, username.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 3, ip.c_str(),       -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 4, outcome.c_str(),  -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 5, outcome.c_str(),  -1, SQLITE_TRANSIENT);
	int step = sqlite3_step(stmt);
	int64_t row_id = (step == SQLITE_DONE) ? sqlite3_last_insert_rowid(db) : 0;
	sqlite3_finalize(stmt);
	return row_id;
}

void Database::FinalizeSession(int64_t session_row_id) {
	if (session_row_id <= 0) return;
	sqlite3* db = GetConnection();
	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"UPDATE ga_user_sessions SET logout_at = strftime('%s','now') "
		"WHERE id = ? AND logout_at IS NULL",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[Session] FinalizeSession prepare failed: %s\n", sqlite3_errmsg(db));
		return;
	}
	sqlite3_bind_int64(stmt, 1, session_row_id);
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);
}

std::optional<Database::ActiveBan> Database::FindActiveBanForUser(int64_t user_id) {
	if (user_id <= 0) return std::nullopt;
	sqlite3* db = GetConnection();
	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"SELECT id, reason, banned_at FROM ga_user_bans "
		"WHERE user_id = ? AND lifted_at IS NULL LIMIT 1",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[Ban] FindActiveBanForUser prepare failed: %s\n", sqlite3_errmsg(db));
		return std::nullopt;
	}
	sqlite3_bind_int64(stmt, 1, user_id);
	std::optional<ActiveBan> out;
	if (sqlite3_step(stmt) == SQLITE_ROW) {
		ActiveBan b;
		b.id        = sqlite3_column_int64(stmt, 0);
		const char* r = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
		if (r) b.reason = r;
		b.banned_at = sqlite3_column_int64(stmt, 2);
		out = std::move(b);
	}
	sqlite3_finalize(stmt);
	return out;
}

std::optional<Database::ActiveBan> Database::FindActiveBanForIp(const std::string& ip) {
	if (ip.empty()) return std::nullopt;
	sqlite3* db = GetConnection();
	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"SELECT id, reason, banned_at FROM ga_ip_bans "
		"WHERE ip = ? AND lifted_at IS NULL LIMIT 1",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[Ban] FindActiveBanForIp prepare failed: %s\n", sqlite3_errmsg(db));
		return std::nullopt;
	}
	sqlite3_bind_text(stmt, 1, ip.c_str(), -1, SQLITE_TRANSIENT);
	std::optional<ActiveBan> out;
	if (sqlite3_step(stmt) == SQLITE_ROW) {
		ActiveBan b;
		b.id        = sqlite3_column_int64(stmt, 0);
		const char* r = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
		if (r) b.reason = r;
		b.banned_at = sqlite3_column_int64(stmt, 2);
		out = std::move(b);
	}
	sqlite3_finalize(stmt);
	return out;
}

void Database::InsertOrReplaceUserBan(int64_t user_id, const std::string& reason) {
	if (user_id <= 0) return;
	sqlite3* db = GetConnection();
	sqlite3_stmt* stmt = nullptr;
	// Upsert: on re-ban of a lifted row, clear lifted_at and refresh
	// reason+banned_at. Keeps one row per user.
	int rc = sqlite3_prepare_v2(db,
		"INSERT INTO ga_user_bans (user_id, reason, banned_at, lifted_at) "
		"VALUES (?, ?, strftime('%s','now'), NULL) "
		"ON CONFLICT(user_id) DO UPDATE SET "
		"  reason = excluded.reason, banned_at = excluded.banned_at, lifted_at = NULL",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[Ban] InsertOrReplaceUserBan prepare failed: %s\n", sqlite3_errmsg(db));
		return;
	}
	sqlite3_bind_int64(stmt, 1, user_id);
	sqlite3_bind_text (stmt, 2, reason.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);
}

void Database::InsertOrReplaceIpBan(const std::string& ip, const std::string& reason) {
	if (ip.empty()) return;
	sqlite3* db = GetConnection();
	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"INSERT INTO ga_ip_bans (ip, reason, banned_at, lifted_at) "
		"VALUES (?, ?, strftime('%s','now'), NULL) "
		"ON CONFLICT(ip) DO UPDATE SET "
		"  reason = excluded.reason, banned_at = excluded.banned_at, lifted_at = NULL",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[Ban] InsertOrReplaceIpBan prepare failed: %s\n", sqlite3_errmsg(db));
		return;
	}
	sqlite3_bind_text(stmt, 1, ip.c_str(),     -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 2, reason.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);
}

void Database::LiftUserBan(int64_t user_id) {
	if (user_id <= 0) return;
	sqlite3* db = GetConnection();
	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"UPDATE ga_user_bans SET lifted_at = strftime('%s','now') "
		"WHERE user_id = ? AND lifted_at IS NULL",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[Ban] LiftUserBan prepare failed: %s\n", sqlite3_errmsg(db));
		return;
	}
	sqlite3_bind_int64(stmt, 1, user_id);
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);
}

void Database::LiftIpBan(const std::string& ip) {
	if (ip.empty()) return;
	sqlite3* db = GetConnection();
	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"UPDATE ga_ip_bans SET lifted_at = strftime('%s','now') "
		"WHERE ip = ? AND lifted_at IS NULL",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[Ban] LiftIpBan prepare failed: %s\n", sqlite3_errmsg(db));
		return;
	}
	sqlite3_bind_text(stmt, 1, ip.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);
}

namespace {
	Database::SessionRow ScanSessionRow(sqlite3_stmt* stmt) {
		Database::SessionRow r;
		r.id = sqlite3_column_int64(stmt, 0);
		if (sqlite3_column_type(stmt, 1) != SQLITE_NULL) {
			r.user_id = sqlite3_column_int64(stmt, 1);
		}
		const char* u  = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
		if (u) r.username = u;
		const char* ip = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
		if (ip) r.ip = ip;
		r.login_at = sqlite3_column_int64(stmt, 4);
		if (sqlite3_column_type(stmt, 5) != SQLITE_NULL) {
			r.logout_at = sqlite3_column_int64(stmt, 5);
		}
		const char* o = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
		if (o) r.outcome = o;
		return r;
	}
}

std::vector<Database::SessionRow> Database::GetRecentSessionsDistinctByUser(int limit) {
	std::vector<SessionRow> out;
	if (limit <= 0) limit = 50;
	sqlite3* db = GetConnection();
	sqlite3_stmt* stmt = nullptr;
	// Window-function dedup: latest row per (user_id, username) bucket.
	// COALESCE(user_id,-id) keeps NULL-user rows distinct from each other.
	int rc = sqlite3_prepare_v2(db,
		"SELECT id, user_id, username, ip, login_at, logout_at, outcome FROM ("
		"  SELECT *, ROW_NUMBER() OVER ("
		"    PARTITION BY COALESCE(user_id, -id), username ORDER BY login_at DESC"
		"  ) AS rn FROM ga_user_sessions"
		") WHERE rn = 1 ORDER BY login_at DESC LIMIT ?",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[Session] GetRecentSessionsDistinctByUser prepare failed: %s\n", sqlite3_errmsg(db));
		return out;
	}
	sqlite3_bind_int(stmt, 1, limit);
	while (sqlite3_step(stmt) == SQLITE_ROW) out.push_back(ScanSessionRow(stmt));
	sqlite3_finalize(stmt);
	return out;
}

std::vector<Database::SessionRow> Database::GetSessionsForUser(int64_t user_id, int limit) {
	std::vector<SessionRow> out;
	if (user_id <= 0) return out;
	if (limit <= 0) limit = 50;
	sqlite3* db = GetConnection();
	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"SELECT id, user_id, username, ip, login_at, logout_at, outcome "
		"FROM ga_user_sessions WHERE user_id = ? ORDER BY login_at DESC LIMIT ?",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[Session] GetSessionsForUser prepare failed: %s\n", sqlite3_errmsg(db));
		return out;
	}
	sqlite3_bind_int64(stmt, 1, user_id);
	sqlite3_bind_int  (stmt, 2, limit);
	while (sqlite3_step(stmt) == SQLITE_ROW) out.push_back(ScanSessionRow(stmt));
	sqlite3_finalize(stmt);
	return out;
}

std::vector<Database::SessionRow> Database::GetSessionsForIp(const std::string& ip, int limit) {
	std::vector<SessionRow> out;
	if (ip.empty()) return out;
	if (limit <= 0) limit = 50;
	sqlite3* db = GetConnection();
	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"SELECT id, user_id, username, ip, login_at, logout_at, outcome "
		"FROM ga_user_sessions WHERE ip = ? ORDER BY login_at DESC LIMIT ?",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[Session] GetSessionsForIp prepare failed: %s\n", sqlite3_errmsg(db));
		return out;
	}
	sqlite3_bind_text(stmt, 1, ip.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_int (stmt, 2, limit);
	while (sqlite3_step(stmt) == SQLITE_ROW) out.push_back(ScanSessionRow(stmt));
	sqlite3_finalize(stmt);
	return out;
}

std::vector<Database::ActiveBanRow> Database::GetActiveUserBans() {
	std::vector<ActiveBanRow> out;
	sqlite3* db = GetConnection();
	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"SELECT id, user_id, reason, banned_at FROM ga_user_bans "
		"WHERE lifted_at IS NULL ORDER BY banned_at DESC",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[Ban] GetActiveUserBans prepare failed: %s\n", sqlite3_errmsg(db));
		return out;
	}
	while (sqlite3_step(stmt) == SQLITE_ROW) {
		ActiveBanRow r;
		r.id              = sqlite3_column_int64(stmt, 0);
		r.user_id_or_zero = sqlite3_column_int64(stmt, 1);
		const char* reason = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
		if (reason) r.reason = reason;
		r.banned_at       = sqlite3_column_int64(stmt, 3);
		out.push_back(std::move(r));
	}
	sqlite3_finalize(stmt);
	return out;
}

std::vector<Database::ActiveBanRow> Database::GetActiveIpBans() {
	std::vector<ActiveBanRow> out;
	sqlite3* db = GetConnection();
	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"SELECT id, ip, reason, banned_at FROM ga_ip_bans "
		"WHERE lifted_at IS NULL ORDER BY banned_at DESC",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[Ban] GetActiveIpBans prepare failed: %s\n", sqlite3_errmsg(db));
		return out;
	}
	while (sqlite3_step(stmt) == SQLITE_ROW) {
		ActiveBanRow r;
		r.id        = sqlite3_column_int64(stmt, 0);
		const char* ip = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
		if (ip) r.ip_or_empty = ip;
		const char* reason = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
		if (reason) r.reason = reason;
		r.banned_at = sqlite3_column_int64(stmt, 3);
		out.push_back(std::move(r));
	}
	sqlite3_finalize(stmt);
	return out;
}

int64_t Database::FindUserIdByUsername(const std::string& username) {
	if (username.empty()) return 0;
	sqlite3* db = GetConnection();
	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"SELECT id FROM ga_users WHERE username = ? LIMIT 1",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[User] FindUserIdByUsername prepare failed: %s\n", sqlite3_errmsg(db));
		return 0;
	}
	sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
	int64_t id = 0;
	if (sqlite3_step(stmt) == SQLITE_ROW) id = sqlite3_column_int64(stmt, 0);
	sqlite3_finalize(stmt);
	return id;
}

std::vector<Database::FriendRow> Database::GetFriendsForUser(int64_t user_id) {
	std::vector<FriendRow> rows;
	if (user_id <= 0) return rows;
	sqlite3* db = GetConnection();
	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"SELECT f.friend_user_id, u.username, f.ignore_flag, f.notes "
		"FROM ga_friends f JOIN ga_users u ON u.id = f.friend_user_id "
		"WHERE f.user_id = ? ORDER BY u.username",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[Friends] GetFriendsForUser prepare failed: %s\n", sqlite3_errmsg(db));
		return rows;
	}
	sqlite3_bind_int64(stmt, 1, user_id);
	while (sqlite3_step(stmt) == SQLITE_ROW) {
		FriendRow r;
		r.friend_user_id = sqlite3_column_int64(stmt, 0);
		const char* name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
		r.friend_name = name ? name : "";
		r.ignore_flag = sqlite3_column_int(stmt, 2) != 0;
		const char* notes = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
		r.notes = notes ? notes : "";
		rows.push_back(std::move(r));
	}
	sqlite3_finalize(stmt);
	return rows;
}

bool Database::AddFriend(int64_t user_id, const std::string& name, bool ignore_flag) {
	if (user_id <= 0 || name.empty()) return false;
	sqlite3* db = GetConnection();
	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"SELECT id FROM ga_users WHERE username = ? COLLATE NOCASE LIMIT 1",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[Friends] AddFriend lookup prepare failed: %s\n", sqlite3_errmsg(db));
		return false;
	}
	sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
	int64_t friend_id = 0;
	if (sqlite3_step(stmt) == SQLITE_ROW) friend_id = sqlite3_column_int64(stmt, 0);
	sqlite3_finalize(stmt);
	if (friend_id == 0) return false;
	if (friend_id == user_id) return true;  // self-add: silently ignore

	stmt = nullptr;
	rc = sqlite3_prepare_v2(db,
		"INSERT INTO ga_friends (user_id, friend_user_id, ignore_flag) VALUES (?, ?, ?) "
		"ON CONFLICT(user_id, friend_user_id) DO UPDATE SET ignore_flag = excluded.ignore_flag",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[Friends] AddFriend insert prepare failed: %s\n", sqlite3_errmsg(db));
		return false;
	}
	sqlite3_bind_int64(stmt, 1, user_id);
	sqlite3_bind_int64(stmt, 2, friend_id);
	sqlite3_bind_int(stmt, 3, ignore_flag ? 1 : 0);
	if (sqlite3_step(stmt) != SQLITE_DONE) {
		Logger::Log("db", "[Friends] AddFriend insert failed: %s\n", sqlite3_errmsg(db));
	}
	sqlite3_finalize(stmt);
	g_friends_epoch++;
	return true;
}

void Database::RemoveFriend(int64_t user_id, int64_t friend_user_id) {
	if (user_id <= 0 || friend_user_id <= 0) return;
	sqlite3* db = GetConnection();
	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"DELETE FROM ga_friends WHERE user_id = ? AND friend_user_id = ?",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[Friends] RemoveFriend prepare failed: %s\n", sqlite3_errmsg(db));
		return;
	}
	sqlite3_bind_int64(stmt, 1, user_id);
	sqlite3_bind_int64(stmt, 2, friend_user_id);
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	g_friends_epoch++;
}

void Database::SetFriendNotes(int64_t user_id, int64_t friend_user_id, const std::string& notes) {
	if (user_id <= 0 || friend_user_id <= 0) return;
	sqlite3* db = GetConnection();
	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"UPDATE ga_friends SET notes = ? WHERE user_id = ? AND friend_user_id = ?",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[Friends] SetFriendNotes prepare failed: %s\n", sqlite3_errmsg(db));
		return;
	}
	sqlite3_bind_text(stmt, 1, notes.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_int64(stmt, 2, user_id);
	sqlite3_bind_int64(stmt, 3, friend_user_id);
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);
}

bool Database::IsIgnoring(const std::string& owner_name, const std::string& other_name) {
	if (owner_name.empty() || other_name.empty()) return false;
	sqlite3* db = GetConnection();
	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"SELECT 1 FROM ga_friends f "
		"JOIN ga_users o ON o.id = f.user_id "
		"JOIN ga_users t ON t.id = f.friend_user_id "
		"WHERE o.username = ?1 COLLATE NOCASE AND t.username = ?2 COLLATE NOCASE "
		"AND f.ignore_flag = 1 LIMIT 1",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[Friends] IsIgnoring prepare failed: %s\n", sqlite3_errmsg(db));
		return false;
	}
	sqlite3_bind_text(stmt, 1, owner_name.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 2, other_name.c_str(), -1, SQLITE_TRANSIENT);
	const bool ignoring = sqlite3_step(stmt) == SQLITE_ROW;
	sqlite3_finalize(stmt);
	return ignoring;
}

uint64_t Database::FriendsEpoch() {
	return g_friends_epoch.load();
}

std::vector<std::string> Database::GetIgnoredNames(const std::string& owner_name) {
	std::vector<std::string> out;
	if (owner_name.empty()) return out;
	sqlite3* db = GetConnection();
	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"SELECT lower(t.username) FROM ga_friends f "
		"JOIN ga_users o ON o.id = f.user_id "
		"JOIN ga_users t ON t.id = f.friend_user_id "
		"WHERE o.username = ? COLLATE NOCASE AND f.ignore_flag = 1",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[Friends] GetIgnoredNames prepare failed: %s\n", sqlite3_errmsg(db));
		return out;
	}
	sqlite3_bind_text(stmt, 1, owner_name.c_str(), -1, SQLITE_TRANSIENT);
	while (sqlite3_step(stmt) == SQLITE_ROW) {
		const char* name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
		if (name) out.push_back(name);
	}
	sqlite3_finalize(stmt);
	return out;
}

bool Database::SetUserPvpVerification(int64_t user_id, bool verified) {
	if (user_id <= 0) return false;
	sqlite3* db = GetConnection();
	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"UPDATE ga_users SET verified_for_pvp = ? WHERE id = ?",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[User] SetUserPvpVerification prepare failed: %s\n", sqlite3_errmsg(db));
		return false;
	}
	sqlite3_bind_int(stmt, 1, verified ? 1 : 0);
	sqlite3_bind_int64(stmt, 2, user_id);
	const bool ok = sqlite3_step(stmt) == SQLITE_DONE;
	sqlite3_finalize(stmt);
	return ok;
}

bool Database::IsUserVerifiedForPvp(int64_t user_id) {
	if (user_id <= 0) return false;
	sqlite3* db = GetConnection();
	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"SELECT verified_for_pvp FROM ga_users WHERE id = ? LIMIT 1",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[User] IsUserVerifiedForPvp prepare failed: %s\n", sqlite3_errmsg(db));
		return false;
	}
	sqlite3_bind_int64(stmt, 1, user_id);
	bool verified = false;
	if (sqlite3_step(stmt) == SQLITE_ROW) verified = sqlite3_column_int(stmt, 0) != 0;
	sqlite3_finalize(stmt);
	return verified;
}

bool Database::ClearUserVerifier(int64_t user_id) {
	if (user_id <= 0) return false;
	sqlite3* db = GetConnection();
	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"UPDATE ga_users SET password_verifier = NULL WHERE id = ?",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[User] ClearUserVerifier prepare failed: %s\n", sqlite3_errmsg(db));
		return false;
	}
	sqlite3_bind_int64(stmt, 1, user_id);
	const bool ok = sqlite3_step(stmt) == SQLITE_DONE;
	sqlite3_finalize(stmt);
	return ok;
}

// ---- User roles (spectator mode, design 2026-07-18) ------------------------

bool Database::UserHasRole(int64_t user_id, const std::string& role) {
	if (user_id <= 0) return false;
	sqlite3* db = GetConnection();
	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"SELECT 1 FROM ga_user_roles WHERE user_id = ? AND role = ? LIMIT 1",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[User] UserHasRole prepare failed: %s\n", sqlite3_errmsg(db));
		return false;
	}
	sqlite3_bind_int64(stmt, 1, user_id);
	sqlite3_bind_text(stmt, 2, role.c_str(), -1, SQLITE_TRANSIENT);
	const bool has_role = sqlite3_step(stmt) == SQLITE_ROW;
	sqlite3_finalize(stmt);
	return has_role;
}

void Database::GrantRole(int64_t user_id, const std::string& role) {
	if (user_id <= 0) return;
	sqlite3* db = GetConnection();
	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"INSERT OR IGNORE INTO ga_user_roles (user_id, role) VALUES (?, ?)",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[User] GrantRole prepare failed: %s\n", sqlite3_errmsg(db));
		return;
	}
	sqlite3_bind_int64(stmt, 1, user_id);
	sqlite3_bind_text(stmt, 2, role.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);
}

void Database::RevokeRole(int64_t user_id, const std::string& role) {
	if (user_id <= 0) return;
	sqlite3* db = GetConnection();
	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"DELETE FROM ga_user_roles WHERE user_id = ? AND role = ?",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[User] RevokeRole prepare failed: %s\n", sqlite3_errmsg(db));
		return;
	}
	sqlite3_bind_int64(stmt, 1, user_id);
	sqlite3_bind_text(stmt, 2, role.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);
}

bool Database::SetUserDlc(int64_t user_id, const std::string& identifier, bool installed) {
	if (user_id <= 0 || identifier.empty()) return false;
	sqlite3* db = GetConnection();

	// Resolve the launcher-facing string identifier to the catalog row.
	int64_t dlc_id = 0;
	{
		sqlite3_stmt* stmt = nullptr;
		int rc = sqlite3_prepare_v2(db,
			"SELECT dlc_id FROM ga_dlc WHERE identifier = ?",
			-1, &stmt, nullptr);
		if (rc != SQLITE_OK || !stmt) {
			Logger::Log("db", "[Dlc] SetUserDlc lookup prepare failed: %s\n", sqlite3_errmsg(db));
			return false;
		}
		sqlite3_bind_text(stmt, 1, identifier.c_str(), -1, SQLITE_TRANSIENT);
		if (sqlite3_step(stmt) == SQLITE_ROW) dlc_id = sqlite3_column_int64(stmt, 0);
		sqlite3_finalize(stmt);
	}
	if (dlc_id == 0) return false;

	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"INSERT INTO ga_user_dlc (user_id, dlc_id, installed, updated_at) "
		"VALUES (?, ?, ?, strftime('%s','now')) "
		"ON CONFLICT(user_id, dlc_id) DO UPDATE SET "
		"  installed = excluded.installed, updated_at = excluded.updated_at",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[Dlc] SetUserDlc upsert prepare failed: %s\n", sqlite3_errmsg(db));
		return false;
	}
	sqlite3_bind_int64(stmt, 1, user_id);
	sqlite3_bind_int64(stmt, 2, dlc_id);
	sqlite3_bind_int(stmt, 3, installed ? 1 : 0);
	const bool ok = sqlite3_step(stmt) == SQLITE_DONE;
	sqlite3_finalize(stmt);
	return ok;
}

std::vector<Database::DlcRow> Database::GetAllDlc() {
	std::vector<DlcRow> out;
	sqlite3* db = GetConnection();
	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"SELECT dlc_id, identifier, name FROM ga_dlc ORDER BY dlc_id",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[Dlc] GetAllDlc prepare failed: %s\n", sqlite3_errmsg(db));
		return out;
	}
	while (sqlite3_step(stmt) == SQLITE_ROW) {
		DlcRow row;
		row.dlc_id = sqlite3_column_int64(stmt, 0);
		if (auto* p = sqlite3_column_text(stmt, 1)) row.identifier = (const char*)p;
		if (auto* p = sqlite3_column_text(stmt, 2)) row.name = (const char*)p;
		out.push_back(std::move(row));
	}
	sqlite3_finalize(stmt);
	return out;
}

std::vector<int64_t> Database::GetInstalledDlcIds(int64_t user_id) {
	std::vector<int64_t> out;
	if (user_id <= 0) return out;
	sqlite3* db = GetConnection();
	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"SELECT dlc_id FROM ga_user_dlc WHERE user_id = ? AND installed = 1",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[Dlc] GetInstalledDlcIds prepare failed: %s\n", sqlite3_errmsg(db));
		return out;
	}
	sqlite3_bind_int64(stmt, 1, user_id);
	while (sqlite3_step(stmt) == SQLITE_ROW)
		out.push_back(sqlite3_column_int64(stmt, 0));
	sqlite3_finalize(stmt);
	return out;
}

bool Database::SetUserAdminNotes(int64_t user_id, const std::string& notes) {
	if (user_id <= 0) return false;
	sqlite3* db = GetConnection();
	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"UPDATE ga_users SET admin_notes = ? WHERE id = ?",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[User] SetUserAdminNotes prepare failed: %s\n", sqlite3_errmsg(db));
		return false;
	}
	if (notes.empty()) sqlite3_bind_null(stmt, 1);
	else               sqlite3_bind_text(stmt, 1, notes.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_int64(stmt, 2, user_id);
	const bool ok = sqlite3_step(stmt) == SQLITE_DONE;
	sqlite3_finalize(stmt);
	return ok;
}

std::string Database::GetUserPreference(int64_t user_id, const std::string& key) {
	if (user_id <= 0) return "";
	sqlite3* db = GetConnection();
	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"SELECT config_value FROM ga_user_preferences WHERE user_id = ? AND config_key = ?",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[User] GetUserPreference prepare failed: %s\n", sqlite3_errmsg(db));
		return "";
	}
	sqlite3_bind_int64(stmt, 1, user_id);
	sqlite3_bind_text(stmt, 2, key.c_str(), -1, SQLITE_TRANSIENT);
	std::string value;
	if (sqlite3_step(stmt) == SQLITE_ROW) {
		const unsigned char* text = sqlite3_column_text(stmt, 0);
		if (text) value = reinterpret_cast<const char*>(text);
	}
	sqlite3_finalize(stmt);
	return value;
}

bool Database::SetUserPreference(int64_t user_id, const std::string& key,
                                 const std::string& value) {
	if (user_id <= 0) return false;
	sqlite3* db = GetConnection();
	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"INSERT INTO ga_user_preferences (user_id, config_key, config_value) "
		"VALUES (?, ?, ?) "
		"ON CONFLICT(user_id, config_key) DO UPDATE SET config_value = excluded.config_value",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[User] SetUserPreference prepare failed: %s\n", sqlite3_errmsg(db));
		return false;
	}
	sqlite3_bind_int64(stmt, 1, user_id);
	sqlite3_bind_text(stmt, 2, key.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 3, value.c_str(), -1, SQLITE_TRANSIENT);
	const bool ok = sqlite3_step(stmt) == SQLITE_DONE;
	sqlite3_finalize(stmt);
	return ok;
}

bool Database::UpsertIpCheck(const std::string& ip,
                             const std::string& country_code,
                             const std::string& country,
                             const std::string& isp,
                             bool proxy, bool hosting) {
	if (ip.empty()) return false;
	sqlite3* db = GetConnection();
	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"INSERT OR REPLACE INTO ga_ip_checks "
		"(ip, country_code, country, isp, proxy, hosting, checked_at) "
		"VALUES (?, ?, ?, ?, ?, ?, strftime('%s','now'))",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[User] UpsertIpCheck prepare failed: %s\n", sqlite3_errmsg(db));
		return false;
	}
	sqlite3_bind_text(stmt, 1, ip.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 2, country_code.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 3, country.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 4, isp.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_int(stmt, 5, proxy ? 1 : 0);
	sqlite3_bind_int(stmt, 6, hosting ? 1 : 0);
	const bool ok = sqlite3_step(stmt) == SQLITE_DONE;
	sqlite3_finalize(stmt);
	return ok;
}

// ---- Match stats (design 2026-06-12) ---------------------------------------

namespace {
// 0 → NULL for optional identity columns.
void BindIdOrNull(sqlite3_stmt* stmt, int idx, int64_t v) {
	if (v != 0) sqlite3_bind_int64(stmt, idx, v);
	else        sqlite3_bind_null(stmt, idx);
}
void BindIntOrNull(sqlite3_stmt* stmt, int idx, int v) {
	if (v != 0) sqlite3_bind_int(stmt, idx, v);
	else        sqlite3_bind_null(stmt, idx);
}
}  // namespace

int64_t Database::InsertMatchEvent(const MatchEventRow& row) {
	sqlite3* db = GetConnection();
	if (!db) return 0;
	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"INSERT INTO ga_match_events (instance_id, ts, game_time, event_type,"
		" actor_user_id, actor_character_id, actor_bot_id, actor_task_force,"
		" target_user_id, target_character_id, target_bot_id, target_task_force,"
		" owner_user_id, owner_character_id, device_id, detail, flags)"
		" VALUES (?, strftime('%s','now'), ?, ?,"
		"  ?, ?, ?, ?,  ?, ?, ?, ?,  ?, ?, ?, ?, ?)",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("matchstats", "[DB] InsertMatchEvent prepare failed: %s\n",
			sqlite3_errmsg(db));
		return 0;
	}
	sqlite3_bind_int64(stmt, 1, row.instance_id);
	sqlite3_bind_double(stmt, 2, row.game_time);
	sqlite3_bind_text(stmt, 3, row.event_type.c_str(), -1, SQLITE_TRANSIENT);
	BindIdOrNull (stmt, 4,  row.actor_user_id);
	BindIdOrNull (stmt, 5,  row.actor_character_id);
	BindIntOrNull(stmt, 6,  row.actor_bot_id);
	BindIntOrNull(stmt, 7,  row.actor_task_force);
	BindIdOrNull (stmt, 8,  row.target_user_id);
	BindIdOrNull (stmt, 9,  row.target_character_id);
	BindIntOrNull(stmt, 10, row.target_bot_id);
	BindIntOrNull(stmt, 11, row.target_task_force);
	BindIdOrNull (stmt, 12, row.owner_user_id);
	BindIdOrNull (stmt, 13, row.owner_character_id);
	BindIntOrNull(stmt, 14, row.device_id);
	BindIdOrNull (stmt, 15, row.detail);
	sqlite3_bind_int(stmt, 16, row.flags);
	rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	if (rc != SQLITE_DONE) {
		Logger::Log("matchstats", "[DB] InsertMatchEvent step failed: %s\n",
			sqlite3_errmsg(db));
		return 0;
	}
	return sqlite3_last_insert_rowid(db);
}

void Database::UpsertMatchPlayerStats(const MatchPlayerStatsRow& row) {
	sqlite3* db = GetConnection();
	if (!db) return;
	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"INSERT INTO ga_match_player_stats (instance_id, user_id, character_id,"
		" task_force, rep_points, kills, assists, damage_taken, damage_dealt,"
		" buff_value, healing, defense, deaths, obj_points, bot_kills,"
		" capture_seconds, contest_seconds, objective_captures,"
		" beacon_spawns_provided, beacon_spawns_used, beacons_destroyed,"
		" time_played_seconds)"
		" VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)"
		" ON CONFLICT(instance_id, character_id, task_force) DO UPDATE SET"
		"  user_id=excluded.user_id, rep_points=excluded.rep_points,"
		"  kills=excluded.kills, assists=excluded.assists,"
		"  damage_taken=excluded.damage_taken, damage_dealt=excluded.damage_dealt,"
		"  buff_value=excluded.buff_value, healing=excluded.healing,"
		"  defense=excluded.defense, deaths=excluded.deaths,"
		"  obj_points=excluded.obj_points, bot_kills=excluded.bot_kills,"
		"  capture_seconds=excluded.capture_seconds,"
		"  contest_seconds=excluded.contest_seconds,"
		"  objective_captures=excluded.objective_captures,"
		"  beacon_spawns_provided=excluded.beacon_spawns_provided,"
		"  beacon_spawns_used=excluded.beacon_spawns_used,"
		"  beacons_destroyed=excluded.beacons_destroyed,"
		"  time_played_seconds=excluded.time_played_seconds",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("matchstats", "[DB] UpsertMatchPlayerStats prepare failed: %s\n",
			sqlite3_errmsg(db));
		return;
	}
	sqlite3_bind_int64(stmt, 1, row.instance_id);
	sqlite3_bind_int64(stmt, 2, row.user_id);
	sqlite3_bind_int64(stmt, 3, row.character_id);
	sqlite3_bind_int(stmt, 4, row.task_force);
	for (int i = 0; i < 11; i++) sqlite3_bind_int(stmt, 5 + i, row.scores[i]);
	sqlite3_bind_double(stmt, 16, row.capture_seconds);
	sqlite3_bind_double(stmt, 17, row.contest_seconds);
	sqlite3_bind_int(stmt, 18, row.objective_captures);
	sqlite3_bind_int(stmt, 19, row.beacon_spawns_provided);
	sqlite3_bind_int(stmt, 20, row.beacon_spawns_used);
	sqlite3_bind_int(stmt, 21, row.beacons_destroyed);
	sqlite3_bind_double(stmt, 22, row.time_played_seconds);
	rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	if (rc != SQLITE_DONE) {
		Logger::Log("matchstats", "[DB] UpsertMatchPlayerStats step failed: %s\n",
			sqlite3_errmsg(db));
	}
}

void Database::UpsertMatchDeviceStatsBatch(const std::vector<MatchDeviceStatsRow>& rows) {
	if (rows.empty()) return;
	sqlite3* db = GetConnection();
	if (!db) return;

	// One transaction for the whole burst. Without this each row committed on
	// its own, and a full PvP instance's 60s flush is a couple of hundred
	// rows — a couple of hundred WAL commits, back to back, on the thread
	// that also runs every TcpSession, ChatSession and matchmaking timer.
	sqlite3_exec(db, "BEGIN IMMEDIATE", nullptr, nullptr, nullptr);
	for (const MatchDeviceStatsRow& row : rows) {
		UpsertMatchDeviceStats(row);
	}
	sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr);
}

void Database::UpsertMatchDeviceStats(const MatchDeviceStatsRow& row) {
	sqlite3* db = GetConnection();
	if (!db) return;
	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"INSERT INTO ga_match_device_stats (instance_id, user_id, character_id,"
		" task_force, device_id, damage, healing, player_kills, bot_kills,"
		" debuffs_removed, overheal, uses, power_restored, power_wasted,"
		" buffed_damage_dealt, protected_damage_taken, rescues,"
		" boost_targets, boost_overwrites, boost_wasted_secs)"
		" VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)"
		" ON CONFLICT(instance_id, character_id, task_force, device_id) DO UPDATE SET"
		"  user_id=excluded.user_id, damage=excluded.damage,"
		"  healing=excluded.healing, player_kills=excluded.player_kills,"
		"  bot_kills=excluded.bot_kills,"
		"  debuffs_removed=excluded.debuffs_removed, overheal=excluded.overheal,"
		"  uses=excluded.uses, power_restored=excluded.power_restored,"
		"  power_wasted=excluded.power_wasted,"
		"  buffed_damage_dealt=excluded.buffed_damage_dealt,"
		"  protected_damage_taken=excluded.protected_damage_taken,"
		"  rescues=excluded.rescues, boost_targets=excluded.boost_targets,"
		"  boost_overwrites=excluded.boost_overwrites,"
		"  boost_wasted_secs=excluded.boost_wasted_secs",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("matchstats", "[DB] UpsertMatchDeviceStats prepare failed: %s\n",
			sqlite3_errmsg(db));
		return;
	}
	sqlite3_bind_int64(stmt, 1, row.instance_id);
	sqlite3_bind_int64(stmt, 2, row.user_id);
	sqlite3_bind_int64(stmt, 3, row.character_id);
	sqlite3_bind_int(stmt, 4, row.task_force);
	sqlite3_bind_int(stmt, 5, row.device_id);
	sqlite3_bind_int(stmt, 6, row.damage);
	sqlite3_bind_int(stmt, 7, row.healing);
	sqlite3_bind_int(stmt, 8, row.player_kills);
	sqlite3_bind_int(stmt, 9, row.bot_kills);
	sqlite3_bind_int(stmt, 10, row.debuffs_removed);
	sqlite3_bind_int(stmt, 11, row.overheal);
	sqlite3_bind_int(stmt, 12, row.uses);
	sqlite3_bind_int(stmt, 13, row.power_restored);
	sqlite3_bind_int(stmt, 14, row.power_wasted);
	sqlite3_bind_int(stmt, 15, row.buffed_damage_dealt);
	sqlite3_bind_int(stmt, 16, row.protected_damage_taken);
	sqlite3_bind_int(stmt, 17, row.rescues);
	sqlite3_bind_int(stmt, 18, row.boost_targets);
	sqlite3_bind_int(stmt, 19, row.boost_overwrites);
	sqlite3_bind_int(stmt, 20, row.boost_wasted_secs);
	rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	if (rc != SQLITE_DONE) {
		Logger::Log("matchstats", "[DB] UpsertMatchDeviceStats step failed: %s\n",
			sqlite3_errmsg(db));
	}
}

void Database::GetQueueStatsToggles(uint32_t queue_id,
                                    bool& device_stats, bool& effectiveness) {
	device_stats = false;
	effectiveness = false;
	sqlite3* db = GetConnection();
	if (!db) return;
	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db,
			"SELECT record_device_stats, record_effectiveness "
			"FROM ga_queues WHERE queue_id = ?",
			-1, &stmt, nullptr) != SQLITE_OK || !stmt) {
		return;
	}
	sqlite3_bind_int(stmt, 1, (int)queue_id);
	if (sqlite3_step(stmt) == SQLITE_ROW) {
		device_stats  = sqlite3_column_int(stmt, 0) != 0;
		effectiveness = sqlite3_column_int(stmt, 1) != 0;
	}
	sqlite3_finalize(stmt);
}

void Database::SetInstanceOutcomeIfNull(int64_t instance_id,
                                        const std::string& outcome,
                                        int winning_task_force) {
	sqlite3* db = GetConnection();
	if (!db) return;
	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"UPDATE ga_instances SET outcome = ?, winning_task_force = ? "
		"WHERE instance_id = ? AND outcome IS NULL AND is_home_map = 0",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("matchstats", "[DB] SetInstanceOutcomeIfNull prepare failed: %s\n",
			sqlite3_errmsg(db));
		return;
	}
	sqlite3_bind_text(stmt, 1, outcome.c_str(), -1, SQLITE_TRANSIENT);
	if (winning_task_force != 0) sqlite3_bind_int(stmt, 2, winning_task_force);
	else                         sqlite3_bind_null(stmt, 2);
	sqlite3_bind_int64(stmt, 3, instance_id);
	rc = sqlite3_step(stmt);
	const int changed = sqlite3_changes(db);
	sqlite3_finalize(stmt);
	if (rc != SQLITE_DONE) {
		Logger::Log("matchstats", "[DB] SetInstanceOutcomeIfNull step failed: %s\n",
			sqlite3_errmsg(db));
	} else {
		Logger::Log("matchstats", "[Outcome] instance=%lld outcome=%s wtf=%d applied=%d\n",
			(long long)instance_id, outcome.c_str(), winning_task_force, changed);
	}
}

void Database::SetInstanceDeathCounts(int64_t instance_id,
                                      int count_deaths_attackers,
                                      int count_deaths_defenders) {
	sqlite3* db = GetConnection();
	if (!db) return;
	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"UPDATE ga_instances SET count_deaths_attackers = ?, count_deaths_defenders = ? "
		"WHERE instance_id = ? AND is_home_map = 0",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("matchstats", "[DB] SetInstanceDeathCounts prepare failed: %s\n",
			sqlite3_errmsg(db));
		return;
	}
	sqlite3_bind_int(stmt, 1, count_deaths_attackers);
	sqlite3_bind_int(stmt, 2, count_deaths_defenders);
	sqlite3_bind_int64(stmt, 3, instance_id);
	rc = sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	if (rc != SQLITE_DONE) {
		Logger::Log("matchstats", "[DB] SetInstanceDeathCounts step failed: %s\n",
			sqlite3_errmsg(db));
	} else {
		Logger::Log("matchstats", "[DeathCounts] instance=%lld attackers=%d defenders=%d\n",
			(long long)instance_id, count_deaths_attackers, count_deaths_defenders);
	}
}

// ---- Agencies (design 2026-07-18) ------------------------------------------

int64_t Database::GetAgencyIdForUser(int64_t user_id) {
	if (user_id <= 0) return 0;
	sqlite3* db = GetConnection();
	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db,
			"SELECT agency_id FROM ga_agency_members WHERE user_id = ? LIMIT 1",
			-1, &stmt, nullptr) != SQLITE_OK || !stmt) {
		return 0;
	}
	sqlite3_bind_int64(stmt, 1, user_id);
	int64_t id = 0;
	if (sqlite3_step(stmt) == SQLITE_ROW) id = sqlite3_column_int64(stmt, 0);
	sqlite3_finalize(stmt);
	return id;
}

int64_t Database::CreateAgency(const std::string& name,
                               float r, float g, float b,
                               int64_t leader_user_id,
                               int64_t leader_character_id,
                               const std::string& leader_name) {
	if (name.empty() || leader_user_id <= 0) return 0;
	if (GetAgencyIdForUser(leader_user_id) != 0) {
		Logger::Log("agency", "[DB] CreateAgency: user %lld already in an agency\n",
			(long long)leader_user_id);
		return 0;
	}

	sqlite3* db = GetConnection();
	char* err = nullptr;
	if (sqlite3_exec(db, "BEGIN IMMEDIATE", nullptr, nullptr, &err) != SQLITE_OK) {
		if (err) sqlite3_free(err);
		return 0;
	}

	int64_t agency_id = 0;
	bool ok = true;
	{
		sqlite3_stmt* stmt = nullptr;
		if (sqlite3_prepare_v2(db,
				"INSERT INTO ga_agencies "
				"(name, color_r, color_g, color_b, leader_user_id, created_at) "
				"VALUES (?, ?, ?, ?, ?, strftime('%s','now'))",
				-1, &stmt, nullptr) != SQLITE_OK || !stmt) {
			ok = false;
		} else {
			sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
			sqlite3_bind_double(stmt, 2, r);
			sqlite3_bind_double(stmt, 3, g);
			sqlite3_bind_double(stmt, 4, b);
			sqlite3_bind_int64(stmt, 5, leader_user_id);
			if (sqlite3_step(stmt) != SQLITE_DONE) ok = false;
			sqlite3_finalize(stmt);
		}
		if (ok) agency_id = sqlite3_last_insert_rowid(db);
	}

	// The client identifies the leader as the rank whose rank_LEVEL == 0
	// (LocalPlayerIsLeader checks GetRankData(myRankId).nRankLevel == 0).
	// Leader MUST be rank_level 0; lower authority = higher level number.
	if (ok) {
		// Officer gets the people-management + message bits (see AgencyPerm);
		// facility/inventory stay leader-only until the leader grants them.
		static const int kOfficerPerms =
			AGENCY_PERM_INVITE | AGENCY_PERM_PROMOTE | AGENCY_PERM_DEMOTE |
			AGENCY_PERM_KICK | AGENCY_PERM_EDIT_MOTD | AGENCY_PERM_EDIT_PUBLIC_MSG |
			AGENCY_PERM_EDIT_OFFICER_MSG | AGENCY_PERM_VIEW_OFFICER_MSG;
		static const struct { int id, level, perms; const char* nm; } kRanks[] = {
			{ 0, 0, (int)0xFFFFFFFF, "Leader"  },
			{ 1, 1, kOfficerPerms,   "Officer" },
			{ 2, 2, 0x00000000,      "Member"  },
		};
		sqlite3_stmt* stmt = nullptr;
		if (sqlite3_prepare_v2(db,
				"INSERT INTO ga_agency_ranks "
				"(agency_id, rank_id, rank_level, permissions, rank_name) "
				"VALUES (?, ?, ?, ?, ?)",
				-1, &stmt, nullptr) != SQLITE_OK || !stmt) {
			ok = false;
		} else {
			for (const auto& rk : kRanks) {
				sqlite3_reset(stmt);
				sqlite3_bind_int64(stmt, 1, agency_id);
				sqlite3_bind_int(stmt, 2, rk.id);
				sqlite3_bind_int(stmt, 3, rk.level);
				sqlite3_bind_int(stmt, 4, rk.perms);
				sqlite3_bind_text(stmt, 5, rk.nm, -1, SQLITE_STATIC);
				if (sqlite3_step(stmt) != SQLITE_DONE) { ok = false; break; }
			}
			sqlite3_finalize(stmt);
		}
	}

	// Add the creator as a member at the leader rank (rank_id 0).
	if (ok) {
		sqlite3_stmt* stmt = nullptr;
		if (sqlite3_prepare_v2(db,
				"INSERT INTO ga_agency_members "
				"(user_id, agency_id, character_id, player_name, rank_id, joined_at) "
				"VALUES (?, ?, ?, ?, 0, strftime('%s','now'))",
				-1, &stmt, nullptr) != SQLITE_OK || !stmt) {
			ok = false;
		} else {
			sqlite3_bind_int64(stmt, 1, leader_user_id);
			sqlite3_bind_int64(stmt, 2, agency_id);
			sqlite3_bind_int64(stmt, 3, leader_character_id);
			sqlite3_bind_text(stmt, 4, leader_name.c_str(), -1, SQLITE_TRANSIENT);
			if (sqlite3_step(stmt) != SQLITE_DONE) ok = false;
			sqlite3_finalize(stmt);
		}
	}

	if (ok) {
		sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr);
		Logger::Log("agency", "[DB] CreateAgency '%s' id=%lld leader=%lld\n",
			name.c_str(), (long long)agency_id, (long long)leader_user_id);
		return agency_id;
	}
	sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
	Logger::Log("agency", "[DB] CreateAgency '%s' failed (name taken?)\n", name.c_str());
	return 0;
}

std::optional<Database::AgencyInfo> Database::GetAgencyInfo(int64_t agency_id) {
	if (agency_id <= 0) return std::nullopt;
	sqlite3* db = GetConnection();

	AgencyInfo info;
	{
		sqlite3_stmt* stmt = nullptr;
		if (sqlite3_prepare_v2(db,
				"SELECT id, name, motd, information, color_r, color_g, color_b, "
				"       recruiting, recruiting_text, sub_only, leader_user_id "
				"FROM ga_agencies WHERE id = ? LIMIT 1",
				-1, &stmt, nullptr) != SQLITE_OK || !stmt) {
			return std::nullopt;
		}
		sqlite3_bind_int64(stmt, 1, agency_id);
		if (sqlite3_step(stmt) != SQLITE_ROW) {
			sqlite3_finalize(stmt);
			return std::nullopt;
		}
		auto col_text = [&](int c) {
			const unsigned char* t = sqlite3_column_text(stmt, c);
			return t ? std::string(reinterpret_cast<const char*>(t)) : std::string();
		};
		info.id             = sqlite3_column_int64(stmt, 0);
		info.name           = col_text(1);
		info.motd           = col_text(2);
		info.information    = col_text(3);
		info.color_r        = (float)sqlite3_column_double(stmt, 4);
		info.color_g        = (float)sqlite3_column_double(stmt, 5);
		info.color_b        = (float)sqlite3_column_double(stmt, 6);
		info.recruiting     = sqlite3_column_int(stmt, 7) != 0;
		info.recruiting_text= col_text(8);
		info.sub_only       = sqlite3_column_int(stmt, 9) != 0;
		info.leader_user_id = sqlite3_column_int64(stmt, 10);
		sqlite3_finalize(stmt);
	}

	{
		sqlite3_stmt* stmt = nullptr;
		if (sqlite3_prepare_v2(db,
				"SELECT rank_id, rank_level, permissions, rank_name "
				"FROM ga_agency_ranks WHERE agency_id = ? ORDER BY rank_level ASC",
				-1, &stmt, nullptr) == SQLITE_OK && stmt) {
			sqlite3_bind_int64(stmt, 1, agency_id);
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				AgencyRankRow rk;
				rk.rank_id     = sqlite3_column_int(stmt, 0);
				rk.rank_level  = sqlite3_column_int(stmt, 1);
				rk.permissions = sqlite3_column_int(stmt, 2);
				const unsigned char* nm = sqlite3_column_text(stmt, 3);
				rk.rank_name   = nm ? reinterpret_cast<const char*>(nm) : "";
				info.ranks.push_back(std::move(rk));
			}
			sqlite3_finalize(stmt);
		}
	}

	{
		sqlite3_stmt* stmt = nullptr;
		// profile_id is joined in rather than looked up per member — the roster
		// is polled every 2s per client, so a per-member query would be O(N)
		// round trips per poll.
		if (sqlite3_prepare_v2(db,
				"SELECT m.user_id, m.character_id, m.player_name, m.rank_id, "
				"       m.public_comment, m.officer_comment, COALESCE(c.profile_id, 0) "
				"FROM ga_agency_members m "
				"LEFT JOIN ga_characters c ON c.id = m.character_id "
				"WHERE m.agency_id = ?",
				-1, &stmt, nullptr) == SQLITE_OK && stmt) {
			sqlite3_bind_int64(stmt, 1, agency_id);
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				AgencyMemberRow m;
				m.user_id      = sqlite3_column_int64(stmt, 0);
				m.character_id = sqlite3_column_int64(stmt, 1);
				const unsigned char* nm = sqlite3_column_text(stmt, 2);
				m.player_name  = nm ? reinterpret_cast<const char*>(nm) : "";
				m.rank_id      = sqlite3_column_int(stmt, 3);
				const unsigned char* pc = sqlite3_column_text(stmt, 4);
				m.public_comment  = pc ? reinterpret_cast<const char*>(pc) : "";
				const unsigned char* oc = sqlite3_column_text(stmt, 5);
				m.officer_comment = oc ? reinterpret_cast<const char*>(oc) : "";
				m.profile_id      = (uint32_t)sqlite3_column_int(stmt, 6);
				info.members.push_back(std::move(m));
			}
			sqlite3_finalize(stmt);
		}
	}

	return info;
}

bool Database::AddAgencyMember(int64_t agency_id, int64_t user_id,
                               int64_t character_id,
                               const std::string& player_name,
                               int rank_id) {
	if (agency_id <= 0 || user_id <= 0) return false;
	if (GetAgencyIdForUser(user_id) != 0) {
		Logger::Log("agency", "[DB] AddAgencyMember: user %lld already in an agency\n",
			(long long)user_id);
		return false;
	}
	sqlite3* db = GetConnection();
	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db,
			"INSERT INTO ga_agency_members "
			"(user_id, agency_id, character_id, player_name, rank_id, joined_at) "
			"VALUES (?, ?, ?, ?, ?, strftime('%s','now'))",
			-1, &stmt, nullptr) != SQLITE_OK || !stmt) {
		return false;
	}
	sqlite3_bind_int64(stmt, 1, user_id);
	sqlite3_bind_int64(stmt, 2, agency_id);
	sqlite3_bind_int64(stmt, 3, character_id);
	sqlite3_bind_text(stmt, 4, player_name.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_int(stmt, 5, rank_id);
	const bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
	sqlite3_finalize(stmt);
	Logger::Log("agency", "[DB] AddAgencyMember agency=%lld user=%lld '%s' rank=%d ok=%d\n",
		(long long)agency_id, (long long)user_id, player_name.c_str(), rank_id, (int)ok);
	return ok;
}

bool Database::SetAgencyMemberRank(int64_t agency_id, int64_t character_id,
                                   int rank_id) {
	if (agency_id <= 0 || character_id <= 0) return false;
	sqlite3* db = GetConnection();
	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db,
			"UPDATE ga_agency_members SET rank_id = ? "
			"WHERE agency_id = ? AND character_id = ?",
			-1, &stmt, nullptr) != SQLITE_OK || !stmt) {
		return false;
	}
	sqlite3_bind_int(stmt, 1, rank_id);
	sqlite3_bind_int64(stmt, 2, agency_id);
	sqlite3_bind_int64(stmt, 3, character_id);
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	const bool ok = sqlite3_changes(db) > 0;
	Logger::Log("agency", "[DB] SetAgencyMemberRank agency=%lld char=%lld rank=%d ok=%d\n",
		(long long)agency_id, (long long)character_id, rank_id, (int)ok);
	return ok;
}

bool Database::RemoveAgencyMember(int64_t agency_id, int64_t user_id) {
	if (agency_id <= 0 || user_id <= 0) return false;
	sqlite3* db = GetConnection();
	sqlite3_stmt* stmt = nullptr;
	// Keyed by user_id (the table's PK) — the stored character_id is only a
	// join-time snapshot and may not match the character currently played.
	if (sqlite3_prepare_v2(db,
			"DELETE FROM ga_agency_members WHERE agency_id = ? AND user_id = ?",
			-1, &stmt, nullptr) != SQLITE_OK || !stmt) {
		return false;
	}
	sqlite3_bind_int64(stmt, 1, agency_id);
	sqlite3_bind_int64(stmt, 2, user_id);
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	const bool ok = sqlite3_changes(db) > 0;
	Logger::Log("agency", "[DB] RemoveAgencyMember agency=%lld user=%lld ok=%d\n",
		(long long)agency_id, (long long)user_id, (int)ok);
	return ok;
}

void Database::DisbandAgency(int64_t agency_id) {
	if (agency_id <= 0) return;
	sqlite3* db = GetConnection();
	int64_t owned_alliance = 0;
	{
		sqlite3_stmt* st = nullptr;
		if (sqlite3_prepare_v2(db, "SELECT id FROM ga_alliances WHERE owner_agency_id = ? LIMIT 1",
				-1, &st, nullptr) == SQLITE_OK && st) {
			sqlite3_bind_int64(st, 1, agency_id);
			if (sqlite3_step(st) == SQLITE_ROW) owned_alliance = sqlite3_column_int64(st, 0);
			sqlite3_finalize(st);
		}
	}
	auto exec_bind = [&](const char* sql, int64_t v) {
		sqlite3_stmt* st = nullptr;
		if (sqlite3_prepare_v2(db, sql, -1, &st, nullptr) == SQLITE_OK && st) {
			sqlite3_bind_int64(st, 1, v);
			sqlite3_step(st);
			sqlite3_finalize(st);
		}
	};
	exec_bind("DELETE FROM ga_agency_members WHERE agency_id = ?", agency_id);
	exec_bind("DELETE FROM ga_agency_ranks   WHERE agency_id = ?", agency_id);
	exec_bind("DELETE FROM ga_alliance_members WHERE agency_id = ?", agency_id);
	exec_bind("DELETE FROM ga_agencies WHERE id = ?", agency_id);
	if (owned_alliance != 0) {
		exec_bind("DELETE FROM ga_alliance_members WHERE alliance_id = ?", owned_alliance);
		exec_bind("DELETE FROM ga_alliances WHERE id = ?", owned_alliance);
	}
	Logger::Log("agency", "[DB] DisbandAgency id=%lld (owned_alliance=%lld)\n",
		(long long)agency_id, (long long)owned_alliance);
}

// ---- Alliances (design 2026-07-18) -----------------------------------------

int64_t Database::GetAllianceIdForAgency(int64_t agency_id) {
	if (agency_id <= 0) return 0;
	sqlite3* db = GetConnection();
	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db,
			"SELECT alliance_id FROM ga_alliance_members WHERE agency_id = ? LIMIT 1",
			-1, &stmt, nullptr) != SQLITE_OK || !stmt) {
		return 0;
	}
	sqlite3_bind_int64(stmt, 1, agency_id);
	int64_t id = 0;
	if (sqlite3_step(stmt) == SQLITE_ROW) id = sqlite3_column_int64(stmt, 0);
	sqlite3_finalize(stmt);
	return id;
}

bool Database::AddAgencyToAlliance(int64_t alliance_id, int64_t agency_id) {
	if (alliance_id <= 0 || agency_id <= 0) return false;
	if (GetAllianceIdForAgency(agency_id) != 0) return false;  // one alliance per agency
	sqlite3* db = GetConnection();
	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db,
			"INSERT INTO ga_alliance_members (agency_id, alliance_id, joined_at) "
			"VALUES (?, ?, strftime('%s','now'))",
			-1, &stmt, nullptr) != SQLITE_OK || !stmt) {
		return false;
	}
	sqlite3_bind_int64(stmt, 1, agency_id);
	sqlite3_bind_int64(stmt, 2, alliance_id);
	bool ok = sqlite3_step(stmt) == SQLITE_DONE;
	sqlite3_finalize(stmt);
	return ok;
}

int64_t Database::CreateAlliance(const std::string& name, int64_t owner_agency_id) {
	if (name.empty() || owner_agency_id <= 0) return 0;
	if (GetAllianceIdForAgency(owner_agency_id) != 0) {
		Logger::Log("agency", "[DB] CreateAlliance: agency %lld already in an alliance\n",
			(long long)owner_agency_id);
		return 0;
	}

	sqlite3* db = GetConnection();
	char* err = nullptr;
	if (sqlite3_exec(db, "BEGIN IMMEDIATE", nullptr, nullptr, &err) != SQLITE_OK) {
		if (err) sqlite3_free(err);
		return 0;
	}

	int64_t alliance_id = 0;
	bool ok = true;
	{
		sqlite3_stmt* stmt = nullptr;
		if (sqlite3_prepare_v2(db,
				"INSERT INTO ga_alliances (name, owner_agency_id, created_at) "
				"VALUES (?, ?, strftime('%s','now'))",
				-1, &stmt, nullptr) != SQLITE_OK || !stmt) {
			ok = false;
		} else {
			sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
			sqlite3_bind_int64(stmt, 2, owner_agency_id);
			if (sqlite3_step(stmt) != SQLITE_DONE) ok = false;  // name UNIQUE
			sqlite3_finalize(stmt);
		}
		if (ok) alliance_id = sqlite3_last_insert_rowid(db);
	}
	if (ok) {
		sqlite3_stmt* stmt = nullptr;
		if (sqlite3_prepare_v2(db,
				"INSERT INTO ga_alliance_members (agency_id, alliance_id, joined_at) "
				"VALUES (?, ?, strftime('%s','now'))",
				-1, &stmt, nullptr) != SQLITE_OK || !stmt) {
			ok = false;
		} else {
			sqlite3_bind_int64(stmt, 1, owner_agency_id);
			sqlite3_bind_int64(stmt, 2, alliance_id);
			if (sqlite3_step(stmt) != SQLITE_DONE) ok = false;
			sqlite3_finalize(stmt);
		}
	}

	if (ok) {
		sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr);
		Logger::Log("agency", "[DB] CreateAlliance '%s' id=%lld owner_agency=%lld\n",
			name.c_str(), (long long)alliance_id, (long long)owner_agency_id);
		return alliance_id;
	}
	sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
	Logger::Log("agency", "[DB] CreateAlliance '%s' failed (name taken?)\n", name.c_str());
	return 0;
}

std::optional<Database::AllianceInfo> Database::GetAllianceInfo(int64_t alliance_id) {
	if (alliance_id <= 0) return std::nullopt;
	sqlite3* db = GetConnection();

	AllianceInfo info;
	{
		sqlite3_stmt* stmt = nullptr;
		if (sqlite3_prepare_v2(db,
				"SELECT al.id, al.name, al.motd, al.information, al.owner_agency_id, "
				"       COALESCE(ag.name,''), al.created_at "
				"FROM ga_alliances al "
				"LEFT JOIN ga_agencies ag ON ag.id = al.owner_agency_id "
				"WHERE al.id = ? LIMIT 1",
				-1, &stmt, nullptr) != SQLITE_OK || !stmt) {
			return std::nullopt;
		}
		sqlite3_bind_int64(stmt, 1, alliance_id);
		if (sqlite3_step(stmt) != SQLITE_ROW) {
			sqlite3_finalize(stmt);
			return std::nullopt;
		}
		auto txt = [&](int c) {
			const unsigned char* t = sqlite3_column_text(stmt, c);
			return t ? std::string(reinterpret_cast<const char*>(t)) : std::string();
		};
		info.id                = sqlite3_column_int64(stmt, 0);
		info.name              = txt(1);
		info.motd              = txt(2);
		info.information       = txt(3);
		info.owner_agency_id   = sqlite3_column_int64(stmt, 4);
		info.owner_agency_name = txt(5);
		info.created_at        = sqlite3_column_int64(stmt, 6);
		sqlite3_finalize(stmt);
	}

	{
		sqlite3_stmt* stmt = nullptr;
		if (sqlite3_prepare_v2(db,
				"SELECT ag.id, ag.name, "
				"       (SELECT COUNT(*) FROM ga_agency_members m WHERE m.agency_id = ag.id) "
				"FROM ga_alliance_members am "
				"JOIN ga_agencies ag ON ag.id = am.agency_id "
				"WHERE am.alliance_id = ?",
				-1, &stmt, nullptr) == SQLITE_OK && stmt) {
			sqlite3_bind_int64(stmt, 1, alliance_id);
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				AllianceMemberRow m;
				m.agency_id    = sqlite3_column_int64(stmt, 0);
				const unsigned char* nm = sqlite3_column_text(stmt, 1);
				m.agency_name  = nm ? reinterpret_cast<const char*>(nm) : "";
				m.member_count = sqlite3_column_int(stmt, 2);
				info.members.push_back(std::move(m));
			}
			sqlite3_finalize(stmt);
		}
	}

	return info;
}

void Database::DisbandAlliance(int64_t alliance_id) {
	if (alliance_id <= 0) return;
	sqlite3* db = GetConnection();
	auto exec_bind = [&](const char* sql, int64_t v) {
		sqlite3_stmt* st = nullptr;
		if (sqlite3_prepare_v2(db, sql, -1, &st, nullptr) == SQLITE_OK && st) {
			sqlite3_bind_int64(st, 1, v);
			sqlite3_step(st);
			sqlite3_finalize(st);
		}
	};
	exec_bind("DELETE FROM ga_alliance_members WHERE alliance_id = ?", alliance_id);
	exec_bind("DELETE FROM ga_alliances WHERE id = ?", alliance_id);
	Logger::Log("agency", "[DB] DisbandAlliance id=%lld\n", (long long)alliance_id);
}

bool Database::SetAgencyText(int64_t agency_id, bool is_motd,
                             const std::string& text) {
	if (agency_id <= 0) return false;
	sqlite3* db = GetConnection();
	sqlite3_stmt* stmt = nullptr;
	const char* sql = is_motd ? "UPDATE ga_agencies SET motd = ? WHERE id = ?"
	                          : "UPDATE ga_agencies SET information = ? WHERE id = ?";
	if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK || !stmt) return false;
	sqlite3_bind_text(stmt, 1, text.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_int64(stmt, 2, agency_id);
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	const bool ok = sqlite3_changes(db) > 0;
	Logger::Log("agency", "[DB] SetAgencyText agency=%lld motd=%d len=%d ok=%d\n",
		(long long)agency_id, (int)is_motd, (int)text.size(), (int)ok);
	return ok;
}

bool Database::SetAgencyMemberComment(int64_t agency_id, int64_t character_id,
                                      bool officer, const std::string& text) {
	if (agency_id <= 0 || character_id <= 0) return false;
	sqlite3* db = GetConnection();
	sqlite3_stmt* stmt = nullptr;
	const char* sql = officer
		? "UPDATE ga_agency_members SET officer_comment = ? "
		  "WHERE agency_id = ? AND character_id = ?"
		: "UPDATE ga_agency_members SET public_comment = ? "
		  "WHERE agency_id = ? AND character_id = ?";
	if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK || !stmt) return false;
	sqlite3_bind_text(stmt, 1, text.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_int64(stmt, 2, agency_id);
	sqlite3_bind_int64(stmt, 3, character_id);
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	const bool ok = sqlite3_changes(db) > 0;
	Logger::Log("agency", "[DB] SetAgencyMemberComment agency=%lld char=%lld officer=%d ok=%d\n",
		(long long)agency_id, (long long)character_id, (int)officer, (int)ok);
	return ok;
}

bool Database::ReplaceAgencyRanks(int64_t agency_id,
                                  const std::vector<AgencyRankRow>& ranks) {
	if (agency_id <= 0 || ranks.empty()) return false;
	// The client identifies the leader by rank_level 0 — refuse a set that
	// would leave the agency without one.
	bool has_leader_rank = false;
	for (const auto& rk : ranks) if (rk.rank_level == 0) has_leader_rank = true;
	if (!has_leader_rank) {
		Logger::Log("agency", "[DB] ReplaceAgencyRanks: rejected, no rank_level 0 row\n");
		return false;
	}

	sqlite3* db = GetConnection();
	if (sqlite3_exec(db, "BEGIN IMMEDIATE", nullptr, nullptr, nullptr) != SQLITE_OK) return false;

	bool ok = true;
	{
		sqlite3_stmt* st = nullptr;
		if (sqlite3_prepare_v2(db, "DELETE FROM ga_agency_ranks WHERE agency_id = ?",
				-1, &st, nullptr) == SQLITE_OK && st) {
			sqlite3_bind_int64(st, 1, agency_id);
			if (sqlite3_step(st) != SQLITE_DONE) ok = false;
			sqlite3_finalize(st);
		} else {
			ok = false;
		}
	}
	if (ok) {
		sqlite3_stmt* st = nullptr;
		if (sqlite3_prepare_v2(db,
				"INSERT INTO ga_agency_ranks "
				"(agency_id, rank_id, rank_level, permissions, rank_name) "
				"VALUES (?, ?, ?, ?, ?)",
				-1, &st, nullptr) == SQLITE_OK && st) {
			for (const auto& rk : ranks) {
				sqlite3_reset(st);
				sqlite3_bind_int64(st, 1, agency_id);
				sqlite3_bind_int(st, 2, rk.rank_id);
				sqlite3_bind_int(st, 3, rk.rank_level);
				sqlite3_bind_int(st, 4, rk.permissions);
				sqlite3_bind_text(st, 5, rk.rank_name.c_str(), -1, SQLITE_TRANSIENT);
				if (sqlite3_step(st) != SQLITE_DONE) { ok = false; break; }
			}
			sqlite3_finalize(st);
		} else {
			ok = false;
		}
	}
	// Anyone on a rank that no longer exists lands on the bottom rank.
	if (ok) {
		int bottom_rank = ranks.front().rank_id;
		int bottom_level = ranks.front().rank_level;
		for (const auto& rk : ranks) {
			if (rk.rank_level > bottom_level) { bottom_level = rk.rank_level; bottom_rank = rk.rank_id; }
		}
		sqlite3_stmt* st = nullptr;
		if (sqlite3_prepare_v2(db,
				"UPDATE ga_agency_members SET rank_id = ? WHERE agency_id = ? AND rank_id NOT IN "
				"(SELECT rank_id FROM ga_agency_ranks WHERE agency_id = ?)",
				-1, &st, nullptr) == SQLITE_OK && st) {
			sqlite3_bind_int(st, 1, bottom_rank);
			sqlite3_bind_int64(st, 2, agency_id);
			sqlite3_bind_int64(st, 3, agency_id);
			sqlite3_step(st);
			sqlite3_finalize(st);
		}
	}

	sqlite3_exec(db, ok ? "COMMIT" : "ROLLBACK", nullptr, nullptr, nullptr);
	Logger::Log("agency", "[DB] ReplaceAgencyRanks agency=%lld rows=%d ok=%d\n",
		(long long)agency_id, (int)ranks.size(), (int)ok);
	return ok;
}

std::map<int64_t, Database::OnlineAgencyMemberRow>
Database::GetOnlineAgencyMembers(int64_t agency_id) {
	std::map<int64_t, OnlineAgencyMemberRow> out;
	if (agency_id <= 0) return out;
	sqlite3_stmt* stmt = nullptr;
	// Join through ga_characters: m.character_id is only a join-time snapshot,
	// so a member online on another character must still match. c.profile_id
	// is the class of the character being played right now.
	if (sqlite3_prepare_v2(GetConnection(),
			"SELECT c.user_id, i.map_name, c.profile_id "
			"FROM ga_instance_players ip "
			"JOIN ga_instances i ON i.instance_id = ip.instance_id AND i.state != 'STOPPED' "
			"JOIN ga_characters c ON c.id = ip.character_id "
			"JOIN ga_agency_members m ON m.user_id = c.user_id "
			"WHERE m.agency_id = ? AND ip.left_at IS NULL",
			-1, &stmt, nullptr) != SQLITE_OK || !stmt) {
		return out;
	}
	sqlite3_bind_int64(stmt, 1, agency_id);
	while (sqlite3_step(stmt) == SQLITE_ROW) {
		const unsigned char* mp = sqlite3_column_text(stmt, 1);
		OnlineAgencyMemberRow row;
		row.map_name   = mp ? reinterpret_cast<const char*>(mp) : "";
		row.profile_id = (uint32_t)sqlite3_column_int(stmt, 2);
		out[sqlite3_column_int64(stmt, 0)] = std::move(row);
	}
	sqlite3_finalize(stmt);
	return out;
}

bool Database::SetAgencyRecruiting(int64_t agency_id, const std::string& text,
                                   bool recruiting, bool sub_only) {
	if (agency_id <= 0) return false;
	sqlite3* db = GetConnection();
	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db,
			"UPDATE ga_agencies SET recruiting_text = ?, recruiting = ?, sub_only = ? "
			"WHERE id = ?",
			-1, &stmt, nullptr) != SQLITE_OK || !stmt) {
		return false;
	}
	sqlite3_bind_text(stmt, 1, text.c_str(), -1, SQLITE_TRANSIENT);
	sqlite3_bind_int(stmt, 2, recruiting ? 1 : 0);
	sqlite3_bind_int(stmt, 3, sub_only ? 1 : 0);
	sqlite3_bind_int64(stmt, 4, agency_id);
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	const bool ok = sqlite3_changes(db) > 0;
	Logger::Log("agency", "[DB] SetAgencyRecruiting agency=%lld recruiting=%d sub_only=%d ok=%d\n",
		(long long)agency_id, (int)recruiting, (int)sub_only, (int)ok);
	return ok;
}

bool Database::SetAgencyLeader(int64_t agency_id, int64_t user_id) {
	if (agency_id <= 0 || user_id <= 0) return false;
	sqlite3* db = GetConnection();
	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db, "UPDATE ga_agencies SET leader_user_id = ? WHERE id = ?",
			-1, &stmt, nullptr) != SQLITE_OK || !stmt) {
		return false;
	}
	sqlite3_bind_int64(stmt, 1, user_id);
	sqlite3_bind_int64(stmt, 2, agency_id);
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	const bool ok = sqlite3_changes(db) > 0;
	Logger::Log("agency", "[DB] SetAgencyLeader agency=%lld user=%lld ok=%d\n",
		(long long)agency_id, (long long)user_id, (int)ok);
	return ok;
}

static std::vector<int64_t> query_user_ids(const char* sql, int64_t bind_value) {
	std::vector<int64_t> out;
	if (bind_value <= 0) return out;
	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(Database::GetConnection(), sql, -1, &stmt, nullptr) != SQLITE_OK
	    || !stmt) {
		return out;
	}
	sqlite3_bind_int64(stmt, 1, bind_value);
	while (sqlite3_step(stmt) == SQLITE_ROW) out.push_back(sqlite3_column_int64(stmt, 0));
	sqlite3_finalize(stmt);
	return out;
}

std::vector<int64_t> Database::GetAllianceMemberUserIds(int64_t alliance_id) {
	return query_user_ids(
		"SELECT m.user_id FROM ga_agency_members m "
		"JOIN ga_alliance_members am ON am.agency_id = m.agency_id "
		"WHERE am.alliance_id = ?", alliance_id);
}

std::vector<int64_t> Database::GetAgencyMemberUserIds(int64_t agency_id) {
	return query_user_ids("SELECT user_id FROM ga_agency_members WHERE agency_id = ?",
	                      agency_id);
}

int64_t Database::GetAgencyIdByLeaderName(const std::string& leader_name) {
	if (leader_name.empty()) return 0;
	sqlite3* db = GetConnection();
	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db,
			"SELECT agency_id FROM ga_agency_members "
			"WHERE rank_id = 0 AND player_name = ? LIMIT 1",
			-1, &stmt, nullptr) != SQLITE_OK || !stmt) {
		return 0;
	}
	sqlite3_bind_text(stmt, 1, leader_name.c_str(), -1, SQLITE_TRANSIENT);
	int64_t id = 0;
	if (sqlite3_step(stmt) == SQLITE_ROW) id = sqlite3_column_int64(stmt, 0);
	sqlite3_finalize(stmt);
	return id;
}

std::map<int64_t, Database::AffiliationRow> Database::GetAffiliationsByCharacter() {
	std::map<int64_t, AffiliationRow> out;
	sqlite3* db = GetConnection();
	sqlite3_stmt* stmt = nullptr;
	// One row per character of every member account — membership is per
	// account, so every character a member plays carries the affiliation.
	if (sqlite3_prepare_v2(db,
			"SELECT c.id, ag.name, COALESCE(al.name,'') "
			"FROM ga_agency_members m "
			"JOIN ga_characters c ON c.user_id = m.user_id "
			"JOIN ga_agencies ag ON ag.id = m.agency_id "
			"LEFT JOIN ga_alliance_members am ON am.agency_id = m.agency_id "
			"LEFT JOIN ga_alliances al ON al.id = am.alliance_id",
			-1, &stmt, nullptr) != SQLITE_OK || !stmt) {
		return out;
	}
	while (sqlite3_step(stmt) == SQLITE_ROW) {
		auto txt = [&](int c) {
			const unsigned char* t = sqlite3_column_text(stmt, c);
			return t ? std::string(reinterpret_cast<const char*>(t)) : std::string();
		};
		out[sqlite3_column_int64(stmt, 0)] = AffiliationRow{ txt(1), txt(2) };
	}
	sqlite3_finalize(stmt);
	return out;
}

void Database::RemoveAgencyFromAlliance(int64_t agency_id) {
	if (agency_id <= 0) return;
	sqlite3* db = GetConnection();
	sqlite3_stmt* st = nullptr;
	if (sqlite3_prepare_v2(db, "DELETE FROM ga_alliance_members WHERE agency_id = ?",
			-1, &st, nullptr) == SQLITE_OK && st) {
		sqlite3_bind_int64(st, 1, agency_id);
		sqlite3_step(st);
		sqlite3_finalize(st);
	}
	Logger::Log("agency", "[DB] RemoveAgencyFromAlliance agency=%lld\n", (long long)agency_id);
}
