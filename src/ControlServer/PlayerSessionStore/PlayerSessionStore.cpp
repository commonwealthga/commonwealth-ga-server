#include "src/ControlServer/PlayerSessionStore/PlayerSessionStore.hpp"
#include "src/ControlServer/TcpSession/TcpSession.hpp"
#include "src/ControlServer/Social/AgencyBank.hpp"
#include "src/ControlServer/ApRewards/ApRewards.hpp"
#include "src/ControlServer/ServiceHooks/ServiceHooks.hpp"
#include "src/ControlServer/Database/Database.hpp"
#include "src/ControlServer/Loadouts/ArmorLoadouts.hpp"
#include "src/ControlServer/Loadouts/ClassLoadouts.hpp"
#include "src/ControlServer/Loadouts/CosmeticLoadouts.hpp"
#include "src/ControlServer/Loadouts/ModResolver.hpp"
#include "src/ControlServer/Logger.hpp"
#include "src/ControlServer/Loadouts/StarterOffhands.hpp"
#include "src/ControlServer/AvaParity/AvaParity.hpp"
#include "src/Shared/CosmeticSlots.hpp"
#include "src/Shared/SkillRules.hpp"
#include "src/Shared/SlotUnlock.hpp"
#include "sqlite3.h"
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <map>
#include <mutex>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <cctype>

std::mutex PlayerSessionStore::mutex_;
std::map<std::string, SessionInfo> PlayerSessionStore::by_guid_;
std::map<std::string, SessionInfo> PlayerSessionStore::by_ip_;

static constexpr int kDyeNoneItemId = 3524;

static bool IsDyeNoneSlot(int item_id, int engine_slot) {
	return item_id == kDyeNoneItemId && engine_slot >= 16 && engine_slot <= 20;
}

// asm_data_set_items.item_id values that crash the client on equip (or on
// nearby-player render). Cosmetic-seeding filters them out, and the per-login
// PurgeBlacklistedItems pass strips them from existing inventories + equip
// rows. Add new ids as they are identified.
static constexpr int kBlacklistedItemIds[] = {
	// 7231,  // Flair - Dweller Specs: crash on equip and on nearby player render
};

// Builds " NOT IN (id, id, …)" for splicing into the seed SQL. Returns
// empty string if the blacklist is empty so the caller's WHERE clause stays
// well-formed in that degenerate case.
static std::string BlacklistNotInClause() {
	constexpr size_t n = sizeof(kBlacklistedItemIds) / sizeof(kBlacklistedItemIds[0]);
	if (n == 0) return "";
	std::string s = " NOT IN (";
	for (size_t i = 0; i < n; ++i) {
		if (i) s += ",";
		s += std::to_string(kBlacklistedItemIds[i]);
	}
	s += ")";
	return s;
}

static bool IsBlacklistedItemId(int item_id) {
	for (int id : kBlacklistedItemIds) {
		if (id == item_id) return true;
	}
	return false;
}

static bool EqualsIgnoreAsciiCase(const std::string& a, const std::string& b) {
	if (a.size() != b.size()) return false;
	for (size_t i = 0; i < a.size(); ++i) {
		const auto ca = static_cast<unsigned char>(a[i]);
		const auto cb = static_cast<unsigned char>(b[i]);
		if (std::tolower(ca) != std::tolower(cb)) return false;
	}
	return true;
}

static bool TableColumnExists(sqlite3* db, const char* table, const char* column) {
	std::string sql = "SELECT 1 FROM pragma_table_info('";
	sql += table;
	sql += "') WHERE name = ?";

	sqlite3_stmt* st = nullptr;
	if (sqlite3_prepare_v2(db, sql.c_str(), -1, &st, nullptr) != SQLITE_OK || !st) {
		if (st) sqlite3_finalize(st);
		return false;
	}

	sqlite3_bind_text(st, 1, column, -1, SQLITE_TRANSIENT);
	const bool found = sqlite3_step(st) == SQLITE_ROW;
	sqlite3_finalize(st);
	return found;
}

void PlayerSessionStore::Init() {
	std::lock_guard<std::mutex> lock(mutex_);
	sqlite3* db = Database::GetConnection();
	char* err = nullptr;
	int rc = sqlite3_exec(db, "DELETE FROM ga_players", nullptr, nullptr, &err);
	if (rc != SQLITE_OK) {
		Logger::Log("db", "[PlayerSessionStore] Failed to clear stale sessions: %s\n", err);
		sqlite3_free(err);
	} else {
		Logger::Log("db", "[PlayerSessionStore] Cleared stale player sessions\n");
	}

	rc = sqlite3_exec(db,
		"DELETE FROM ga_character_devices "
		"WHERE character_id IN ("
		"  SELECT c.id FROM ga_characters c "
		"  JOIN ga_users u ON u.id = c.user_id "
		"  WHERE u.username = 'unknown'"
		");"
		"DELETE FROM ga_character_skills "
		"WHERE character_id IN ("
		"  SELECT c.id FROM ga_characters c "
		"  JOIN ga_users u ON u.id = c.user_id "
		"  WHERE u.username = 'unknown'"
		");"
		"DELETE FROM ga_character_quests "
		"WHERE character_id IN ("
		"  SELECT c.id FROM ga_characters c "
		"  JOIN ga_users u ON u.id = c.user_id "
		"  WHERE u.username = 'unknown'"
		");"
		"DELETE FROM ga_characters "
		"WHERE user_id IN (SELECT id FROM ga_users WHERE username = 'unknown');"
		"DELETE FROM ga_players_inventory "
		"WHERE user_id IN (SELECT id FROM ga_users WHERE username = 'unknown');"
		"DELETE FROM ga_players WHERE player_name = 'unknown' OR user_id IN ("
		"  SELECT id FROM ga_users WHERE username = 'unknown'"
		");"
		"DELETE FROM ga_users WHERE username = 'unknown';",
		nullptr, nullptr, &err);
	if (rc != SQLITE_OK) {
		Logger::Log("db", "[PlayerSessionStore] Failed to clear unknown user data: %s\n", err);
		sqlite3_free(err);
		err = nullptr;
	} else {
		Logger::Log("db", "[PlayerSessionStore] Cleared unknown user data\n");
	}

	// === Loadout profiles migration ===
	// Each table is checked independently. Some DBs already have
	// ga_characters.current_item_profile_id but still carry the old
	// ga_character_devices shape, so one global guard is not enough.
	{
		if (!TableColumnExists(db, "ga_characters", "current_item_profile_id")) {
			if (sqlite3_exec(db,
					"ALTER TABLE ga_characters ADD COLUMN current_item_profile_id INTEGER NOT NULL DEFAULT 1",
					nullptr, nullptr, &err) != SQLITE_OK) {
				Logger::Log("db", "[Init] current_item_profile_id migration failed: %s\n",
					err ? err : "(no msg)");
				if (err) { sqlite3_free(err); err = nullptr; }
			} else {
				Logger::Log("db", "[Init] Added ga_characters.current_item_profile_id\n");
			}
		}

		// === Character soft-delete migration ===
		// deleted_at = 0 means active; non-zero = unix epoch of deletion.
		// Rows are never hard-deleted so stats/history/MMR keep resolving.
		if (!TableColumnExists(db, "ga_characters", "deleted_at")) {
			if (sqlite3_exec(db,
					"ALTER TABLE ga_characters ADD COLUMN deleted_at INTEGER NOT NULL DEFAULT 0",
					nullptr, nullptr, &err) != SQLITE_OK) {
				Logger::Log("db", "[Init] deleted_at migration failed: %s\n",
					err ? err : "(no msg)");
				if (err) { sqlite3_free(err); err = nullptr; }
			} else {
				Logger::Log("db", "[Init] Added ga_characters.deleted_at\n");
			}
		}

		// === Agent XP migration ===
		// Level is derived from xp via asm_data_set_xp_levels; 0 = level 1.
		if (!TableColumnExists(db, "ga_characters", "xp")) {
			if (sqlite3_exec(db,
					"ALTER TABLE ga_characters ADD COLUMN xp INTEGER NOT NULL DEFAULT 0",
					nullptr, nullptr, &err) != SQLITE_OK) {
				Logger::Log("db", "[Init] xp migration failed: %s\n",
					err ? err : "(no msg)");
				if (err) { sqlite3_free(err); err = nullptr; }
			} else {
				Logger::Log("db", "[Init] Added ga_characters.xp\n");
			}
		}

		// === Agent currency migration ===
		for (const char* col : {"credits", "tokens"}) {
			if (TableColumnExists(db, "ga_characters", col)) continue;
			const std::string sql = std::string("ALTER TABLE ga_characters ADD COLUMN ")
				+ col + " INTEGER NOT NULL DEFAULT 0";
			if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &err) != SQLITE_OK) {
				Logger::Log("db", "[Init] %s migration failed: %s\n", col,
					err ? err : "(no msg)");
				if (err) { sqlite3_free(err); err = nullptr; }
			} else {
				Logger::Log("db", "[Init] Added ga_characters.%s\n", col);
			}
		}

		// Credits + tokens are per AGENT (AP stays per account in
		// ga_users.agenda_points). Undoes an earlier shared-wallet scheme:
		// drop its triggers and, once (marker), split each merged account back -
		// every live agent gets its pre-merge balance (ga_wallet_premerge); the
		// rest of the shared balance (deleted agents' pre-merge balances + the
		// net change since the merge) goes to the account's active agent, never
		// below 0, so the account total is kept. Every row is logged in
		// ga_wallet_split_log.
		{
			sqlite3_exec(db,
				"DROP TRIGGER IF EXISTS ga_wallet_share;"
				"DROP TRIGGER IF EXISTS ga_wallet_new_agent;",
				nullptr, nullptr, nullptr);
			bool merged = false, split = false;
			{
				sqlite3_stmt* mstmt = nullptr;
				if (sqlite3_prepare_v2(db,
						"SELECT name FROM cs_migration_markers WHERE name IN"
						" ('account_wallet_merge_2026_10_29', 'account_wallet_split_2026_10_29')",
						-1, &mstmt, nullptr) == SQLITE_OK && mstmt) {
					while (sqlite3_step(mstmt) == SQLITE_ROW) {
						const char* n = (const char*)sqlite3_column_text(mstmt, 0);
						if (n && std::string(n) == "account_wallet_split_2026_10_29") split = true;
						else if (n) merged = true;
					}
				}
				if (mstmt) sqlite3_finalize(mstmt);
			}
			if (merged && !split) {
				const char* kSplit =
					"BEGIN IMMEDIATE;"
					"CREATE TABLE IF NOT EXISTS ga_wallet_split_log (user_id INTEGER, character_id INTEGER,"
					"  old_credits INTEGER, old_tokens INTEGER, new_credits INTEGER, new_tokens INTEGER, at INTEGER);"
					"CREATE TEMP TABLE ws_user AS SELECT u.user_id,"
					"  (SELECT c.id FROM ga_characters c WHERE c.user_id = u.user_id ORDER BY"
					"     c.id = (SELECT a.character_id FROM ga_user_active_agent a WHERE a.user_id = c.user_id) DESC,"
					"     c.deleted_at = 0 DESC, c.id DESC LIMIT 1) AS recipient,"
					"  (SELECT SUM(p.credits) FROM ga_wallet_premerge p WHERE p.user_id = u.user_id) AS pre_c,"
					"  (SELECT SUM(p.tokens)  FROM ga_wallet_premerge p WHERE p.user_id = u.user_id) AS pre_t"
					"  FROM (SELECT DISTINCT user_id FROM ga_wallet_premerge) u;"
					"CREATE TEMP TABLE ws_new AS SELECT c.id, c.user_id, c.credits AS old_c, c.tokens AS old_t,"
					"  CASE WHEN c.id = w.recipient THEN MAX(0, c.credits - w.pre_c"
					"         + COALESCE((SELECT p.credits FROM ga_wallet_premerge p WHERE p.id = c.id), 0)"
					"         + COALESCE((SELECT SUM(p.credits) FROM ga_wallet_premerge p JOIN ga_characters d ON d.id = p.id"
					"                     WHERE p.user_id = c.user_id AND d.deleted_at != 0 AND d.id != c.id), 0))"
					"       WHEN c.deleted_at != 0 THEN 0"
					"       ELSE COALESCE((SELECT p.credits FROM ga_wallet_premerge p WHERE p.id = c.id), 0) END AS new_c,"
					"  CASE WHEN c.id = w.recipient THEN MAX(0, c.tokens - w.pre_t"
					"         + COALESCE((SELECT p.tokens FROM ga_wallet_premerge p WHERE p.id = c.id), 0)"
					"         + COALESCE((SELECT SUM(p.tokens) FROM ga_wallet_premerge p JOIN ga_characters d ON d.id = p.id"
					"                     WHERE p.user_id = c.user_id AND d.deleted_at != 0 AND d.id != c.id), 0))"
					"       WHEN c.deleted_at != 0 THEN 0"
					"       ELSE COALESCE((SELECT p.tokens FROM ga_wallet_premerge p WHERE p.id = c.id), 0) END AS new_t"
					"  FROM ga_characters c JOIN ws_user w ON w.user_id = c.user_id;"
					"INSERT INTO ga_wallet_split_log SELECT user_id, id, old_c, old_t, new_c, new_t, strftime('%s','now') FROM ws_new;"
					"UPDATE ga_characters SET credits = (SELECT new_c FROM ws_new WHERE ws_new.id = ga_characters.id),"
					"  tokens = (SELECT new_t FROM ws_new WHERE ws_new.id = ga_characters.id)"
					"  WHERE id IN (SELECT id FROM ws_new);"
					"DROP TABLE ws_new; DROP TABLE ws_user;"
					"INSERT OR IGNORE INTO cs_migration_markers (name) VALUES ('account_wallet_split_2026_10_29');"
					"COMMIT;";
				if (sqlite3_exec(db, kSplit, nullptr, nullptr, &err) != SQLITE_OK) {
					Logger::Log("db", "[Database] wallet split failed: %s\n", err ? err : "?");
					if (err) { sqlite3_free(err); err = nullptr; }
					sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
				} else {
					Logger::Log("db", "[Database] wallet split done (per-agent credits/tokens, log in ga_wallet_split_log)\n");
				}
			}
		}

		const bool devices_need_migration =
			!TableColumnExists(db, "ga_character_devices", "item_profile_id") ||
			!TableColumnExists(db, "ga_character_devices", "equipped_slot");
		if (devices_need_migration) {
			Logger::Log("db", "[Init] Rebuilding ga_character_devices for loadout profiles\n");
			const bool has_equipped_slot = TableColumnExists(db, "ga_character_devices", "equipped_slot");
			const bool has_equip_slot = TableColumnExists(db, "ga_character_devices", "equip_slot");
			const bool has_inventory_id = TableColumnExists(db, "ga_character_devices", "inventory_id");
			const char* slot_column = has_equipped_slot ? "equipped_slot" : (has_equip_slot ? "equip_slot" : nullptr);

			std::vector<std::string> sqls = {
				"DROP TABLE IF EXISTS ga_character_devices_v2",
				"CREATE TABLE ga_character_devices_v2 ("
				" id              INTEGER PRIMARY KEY AUTOINCREMENT,"
				" character_id    INTEGER NOT NULL REFERENCES ga_characters(id),"
				" item_profile_id INTEGER NOT NULL,"
				" inventory_id    INTEGER NOT NULL REFERENCES ga_players_inventory(id),"
				" equipped_slot   INTEGER NOT NULL,"
				" UNIQUE(character_id, item_profile_id, equipped_slot)"
				")",
			};
			if (slot_column && has_inventory_id) {
				std::string copy_devices =
					"INSERT INTO ga_character_devices_v2 (character_id, item_profile_id, inventory_id, equipped_slot)"
					" SELECT character_id, 1, inventory_id, ";
				copy_devices += slot_column;
				copy_devices += " FROM ga_character_devices";
				sqls.push_back(copy_devices);
			} else {
				Logger::Log("db", "[Init] ga_character_devices has no copyable slot/inventory columns; rebuilding empty\n");
			}
			sqls.push_back("DROP TABLE IF EXISTS ga_character_devices");
			sqls.push_back("ALTER TABLE ga_character_devices_v2 RENAME TO ga_character_devices");
			sqls.push_back("CREATE INDEX IF NOT EXISTS idx_ga_character_devices_char ON ga_character_devices(character_id)");

			char* merr = nullptr;
			for (const auto& sql : sqls) {
				if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &merr) != SQLITE_OK) {
					Logger::Log("db", "[Init] Device migration step failed: %s -- SQL: %s\n",
						merr ? merr : "(no msg)", sql.c_str());
					if (merr) { sqlite3_free(merr); merr = nullptr; }
				}
			}
			Logger::Log("db", "[Init] ga_character_devices loadout-profile rebuild complete\n");
		}

		if (!TableColumnExists(db, "ga_character_skills", "item_profile_id")) {
			Logger::Log("db", "[Init] Rebuilding ga_character_skills for loadout profiles\n");
			std::vector<std::string> sqls = {
				"DROP TABLE IF EXISTS ga_character_skills_v2",
				"CREATE TABLE ga_character_skills_v2 ("
				" id              INTEGER PRIMARY KEY AUTOINCREMENT,"
				" character_id    INTEGER NOT NULL REFERENCES ga_characters(id),"
				" item_profile_id INTEGER NOT NULL,"
				" skill_group_id  INTEGER NOT NULL,"
				" skill_id        INTEGER NOT NULL,"
				" points          INTEGER NOT NULL DEFAULT 0,"
				" UNIQUE(character_id, item_profile_id, skill_group_id, skill_id)"
				")",
				"INSERT INTO ga_character_skills_v2 (character_id, item_profile_id, skill_group_id, skill_id, points)"
				" SELECT character_id, 1, skill_group_id, skill_id, points FROM ga_character_skills",
				"DROP TABLE ga_character_skills",
				"ALTER TABLE ga_character_skills_v2 RENAME TO ga_character_skills",
				"CREATE INDEX IF NOT EXISTS idx_ga_character_skills_char ON ga_character_skills(character_id)",
			};
			char* merr = nullptr;
			for (const auto& sql : sqls) {
				if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &merr) != SQLITE_OK) {
					Logger::Log("db", "[Init] Skill migration step failed: %s -- SQL: %s\n",
						merr ? merr : "(no msg)", sql.c_str());
					if (merr) { sqlite3_free(merr); merr = nullptr; }
				}
			}
			Logger::Log("db", "[Init] ga_character_skills loadout-profile rebuild complete\n");
		}
	}

	by_guid_.clear();
	by_ip_.clear();
}

void PlayerSessionStore::Register(const SessionInfo& info) {
	std::lock_guard<std::mutex> lock(mutex_);
	sqlite3* db = Database::GetConnection();

	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"INSERT OR REPLACE INTO ga_players "
		"(session_guid, player_name, ip_address, user_id, created_at, last_seen_at) "
		"VALUES (?, ?, ?, ?, strftime('%s','now'), strftime('%s','now'))",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[PlayerSessionStore] Failed to prepare register: %s\n", sqlite3_errmsg(db));
	} else {
		sqlite3_bind_text(stmt, 1, info.session_guid.c_str(), -1, SQLITE_TRANSIENT);
		sqlite3_bind_text(stmt, 2, info.player_name.c_str(),  -1, SQLITE_TRANSIENT);
		sqlite3_bind_text(stmt, 3, info.ip_address.c_str(),   -1, SQLITE_TRANSIENT);
		if (info.user_id != 0)
			sqlite3_bind_int64(stmt, 4, info.user_id);
		else
			sqlite3_bind_null(stmt, 4);
		rc = sqlite3_step(stmt);
		sqlite3_finalize(stmt);
		if (rc != SQLITE_DONE)
			Logger::Log("db", "[PlayerSessionStore] Failed to register player: %s\n", sqlite3_errmsg(db));
	}

	by_guid_[info.session_guid] = info;
	by_ip_[info.ip_address] = info;

	Logger::Log("db", "[PlayerSessionStore] Registered player '%s' guid=%s ip=%s\n",
		info.player_name.c_str(), info.session_guid.c_str(), info.ip_address.c_str());
}

void PlayerSessionStore::Unregister(const std::string& session_guid) {
	std::lock_guard<std::mutex> lock(mutex_);
	sqlite3* db = Database::GetConnection();

	char sql[256];
	snprintf(sql, sizeof(sql),
		"DELETE FROM ga_players WHERE session_guid='%s'",
		session_guid.c_str());

	char* err = nullptr;
	int rc = sqlite3_exec(db, sql, nullptr, nullptr, &err);
	if (rc != SQLITE_OK) {
		Logger::Log("db", "[PlayerSessionStore] Failed to unregister player: %s\n", err);
		sqlite3_free(err);
	}

	auto it = by_guid_.find(session_guid);
	if (it != by_guid_.end()) {
		by_ip_.erase(it->second.ip_address);
	}
	by_guid_.erase(session_guid);

	Logger::Log("db", "[PlayerSessionStore] Unregistered player guid=%s\n", session_guid.c_str());
}

std::optional<SessionInfo> PlayerSessionStore::GetByGuid(const std::string& guid) {
	std::lock_guard<std::mutex> lock(mutex_);
	auto it = by_guid_.find(guid);
	if (it != by_guid_.end()) return it->second;
	return std::nullopt;
}

SessionInfo* PlayerSessionStore::GetByGuidPtr(const std::string& guid) {
	std::lock_guard<std::mutex> lock(mutex_);
	auto it = by_guid_.find(guid);
	if (it != by_guid_.end()) return &it->second;
	return nullptr;
}

std::optional<SessionInfo> PlayerSessionStore::GetByIp(const std::string& ip) {
	std::lock_guard<std::mutex> lock(mutex_);
	auto it = by_ip_.find(ip);
	if (it != by_ip_.end()) return it->second;
	return std::nullopt;
}

std::optional<SessionInfo> PlayerSessionStore::GetByPlayerName(const std::string& player_name) {
	std::lock_guard<std::mutex> lock(mutex_);
	for (const auto& kv : by_guid_) {
		if (EqualsIgnoreAsciiCase(kv.second.player_name, player_name)) {
			return kv.second;
		}
	}
	return std::nullopt;
}

int64_t PlayerSessionStore::UpsertUser(const std::string& username) {
	std::lock_guard<std::mutex> lock(mutex_);
	sqlite3* db = Database::GetConnection();

	// Insert if not present (ignore on conflict keeps existing row).
	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db, "INSERT OR IGNORE INTO ga_users (username) VALUES (?)", -1, &stmt, nullptr);
	if (rc == SQLITE_OK && stmt) {
		sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
		sqlite3_step(stmt);
		sqlite3_finalize(stmt);
	} else {
		Logger::Log("db", "[PlayerSessionStore] UpsertUser insert prepare failed: %s\n", sqlite3_errmsg(db));
		return 0;
	}

	// Fetch id.
	int64_t id = 0;
	rc = sqlite3_prepare_v2(db, "SELECT id FROM ga_users WHERE username = ?", -1, &stmt, nullptr);
	if (rc == SQLITE_OK && stmt) {
		sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
		if (sqlite3_step(stmt) == SQLITE_ROW)
			id = sqlite3_column_int64(stmt, 0);
		sqlite3_finalize(stmt);
	} else {
		Logger::Log("db", "[PlayerSessionStore] UpsertUser select prepare failed: %s\n", sqlite3_errmsg(db));
	}

	return id;
}

std::string PlayerSessionStore::FindProvisionedUsername(const std::string& username) {
	std::lock_guard<std::mutex> lock(mutex_);
	std::string out;
	sqlite3* db = Database::GetConnection();
	if (!db) return out;
	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"SELECT username FROM ga_users WHERE username = ? COLLATE NOCASE "
		"AND password_verifier IS NOT NULL AND length(password_verifier) > 0 LIMIT 1",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[PlayerSessionStore] FindProvisionedUsername prepare failed: %s\n", sqlite3_errmsg(db));
		return out;
	}
	sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
	if (sqlite3_step(stmt) == SQLITE_ROW) {
		const unsigned char* t = sqlite3_column_text(stmt, 0);
		if (t) out = reinterpret_cast<const char*>(t);
	}
	sqlite3_finalize(stmt);
	return out;
}

PlayerSessionStore::UserAuth PlayerSessionStore::GetUserAuth(const std::string& username) {
	std::lock_guard<std::mutex> lock(mutex_);
	UserAuth out;
	sqlite3* db = Database::GetConnection();
	if (!db) return out;

	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"SELECT id, password_verifier, registered_at FROM ga_users WHERE username = ? LIMIT 1",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[PlayerSessionStore] GetUserAuth prepare failed: %s\n", sqlite3_errmsg(db));
		return out;
	}
	sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
	if (sqlite3_step(stmt) == SQLITE_ROW) {
		out.exists = true;
		out.user_id = sqlite3_column_int64(stmt, 0);
		const void* blob = sqlite3_column_blob(stmt, 1);
		const int   blen = sqlite3_column_bytes(stmt, 1);
		if (blob && blen > 0) {
			const uint8_t* p = static_cast<const uint8_t*>(blob);
			out.verifier.assign(p, p + blen);
		}
		out.registered_at = sqlite3_column_int64(stmt, 2);
	}
	sqlite3_finalize(stmt);
	return out;
}

void PlayerSessionStore::SetUserVerifier(int64_t user_id,
                                         const std::vector<uint8_t>& verifier,
                                         int64_t now_epoch) {
	if (user_id <= 0 || verifier.empty()) return;
	std::lock_guard<std::mutex> lock(mutex_);
	sqlite3* db = Database::GetConnection();
	if (!db) return;

	sqlite3_stmt* stmt = nullptr;
	// COALESCE keeps the first registration timestamp once set.
	int rc = sqlite3_prepare_v2(db,
		"UPDATE ga_users SET password_verifier = ?, "
		"registered_at = COALESCE(registered_at, ?) WHERE id = ?",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[PlayerSessionStore] SetUserVerifier prepare failed: %s\n", sqlite3_errmsg(db));
		return;
	}
	sqlite3_bind_blob(stmt, 1, verifier.data(), static_cast<int>(verifier.size()), SQLITE_TRANSIENT);
	sqlite3_bind_int64(stmt, 2, now_epoch);
	sqlite3_bind_int64(stmt, 3, user_id);
	if (sqlite3_step(stmt) != SQLITE_DONE)
		Logger::Log("db", "[PlayerSessionStore] SetUserVerifier update failed: %s\n", sqlite3_errmsg(db));
	sqlite3_finalize(stmt);
}

int64_t PlayerSessionStore::InsertCharacter(int64_t user_id, uint32_t profile_id,
                                             uint32_t head_asm_id, uint32_t gender_type_value_id,
                                             const std::vector<uint8_t>& morph_data,
                                             uint32_t hair_asm_id,
                                             uint32_t skin_mat_param_id,
                                             uint32_t eye_mat_param_id) {
	std::lock_guard<std::mutex> lock(mutex_);
	sqlite3* db = Database::GetConnection();

	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"INSERT INTO ga_characters "
		"  (user_id, profile_id, head_asm_id, gender_type_value_id, morph_data, "
		"   hair_asm_id, skin_mat_param_id, eye_mat_param_id) "
		"VALUES (?, ?, ?, ?, ?, ?, ?, ?)",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[PlayerSessionStore] InsertCharacter prepare failed: %s\n", sqlite3_errmsg(db));
		return 0;
	}
	sqlite3_bind_int64(stmt, 1, user_id);
	sqlite3_bind_int(stmt,   2, static_cast<int>(profile_id));
	sqlite3_bind_int(stmt,   3, static_cast<int>(head_asm_id));
	sqlite3_bind_int(stmt,   4, static_cast<int>(gender_type_value_id));
	if (!morph_data.empty())
		sqlite3_bind_blob(stmt, 5, morph_data.data(), static_cast<int>(morph_data.size()), SQLITE_TRANSIENT);
	else
		sqlite3_bind_null(stmt, 5);
	sqlite3_bind_int(stmt,   6, static_cast<int>(hair_asm_id));
	sqlite3_bind_int(stmt,   7, static_cast<int>(skin_mat_param_id));
	sqlite3_bind_int(stmt,   8, static_cast<int>(eye_mat_param_id));
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);

	int64_t newId = sqlite3_last_insert_rowid(db);
	// A new agent starts with an empty bag of its own (per-agent pool).
	ActivateAgentPool(user_id, newId);
	// Seed (or top up) this user's account-scoped inventory pool — both for
	// this profile and any already-existing siblings. ga_character_devices is
	// otherwise left empty; the player equips from the bag via the equip
	// screen.
	SeedInventoryFromLoadouts(user_id);
	// HUMAN BASE ATTRIBUTES (slot 14) is the one exception — the player can't
	// equip it, so we pin it across all 5 loadout profiles before they ever
	// load in. Without this, fresh characters hit the resubmit loop on the
	// REST device until disconnect+reconnect.
	PinClassDeviceSlot14(newId);
	SeedCharacterArmorDefaults(newId);
	SeedStarterWeapons(newId);
	return newId;
}

std::vector<CharacterInfo> PlayerSessionStore::GetCharactersByUserId(int64_t user_id) {
	std::lock_guard<std::mutex> lock(mutex_);
	sqlite3* db = Database::GetConnection();
	std::vector<CharacterInfo> result;

	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"SELECT id, profile_id, head_asm_id, gender_type_value_id, morph_data, "
		"       hair_asm_id, skin_mat_param_id, eye_mat_param_id, current_item_profile_id "
		"FROM ga_characters WHERE user_id = ? AND deleted_at = 0",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[PlayerSessionStore] GetCharactersByUserId prepare failed: %s\n", sqlite3_errmsg(db));
		return result;
	}
	sqlite3_bind_int64(stmt, 1, user_id);

	while (sqlite3_step(stmt) == SQLITE_ROW) {
		CharacterInfo c;
		c.id                   = sqlite3_column_int64(stmt, 0);
		c.user_id              = user_id;
		c.profile_id           = static_cast<uint32_t>(sqlite3_column_int(stmt, 1));
		c.head_asm_id          = static_cast<uint32_t>(sqlite3_column_int(stmt, 2));
		c.gender_type_value_id = static_cast<uint32_t>(sqlite3_column_int(stmt, 3));
		const void* blob = sqlite3_column_blob(stmt, 4);
		int bytes        = sqlite3_column_bytes(stmt, 4);
		if (blob && bytes > 0)
			c.morph_data.assign(static_cast<const uint8_t*>(blob),
			                    static_cast<const uint8_t*>(blob) + bytes);
		c.hair_asm_id              = static_cast<uint32_t>(sqlite3_column_int(stmt, 5));
		c.skin_mat_param_id        = static_cast<uint32_t>(sqlite3_column_int(stmt, 6));
		c.eye_mat_param_id         = static_cast<uint32_t>(sqlite3_column_int(stmt, 7));
		c.current_item_profile_id  = sqlite3_column_int(stmt, 8);
		result.push_back(std::move(c));
	}
	sqlite3_finalize(stmt);
	return result;
}

std::optional<CharacterInfo> PlayerSessionStore::GetCharacterById(int64_t id) {
	std::lock_guard<std::mutex> lock(mutex_);
	sqlite3* db = Database::GetConnection();

	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"SELECT id, user_id, profile_id, head_asm_id, gender_type_value_id, morph_data, "
		"       hair_asm_id, skin_mat_param_id, eye_mat_param_id, deleted_at "
		"FROM ga_characters WHERE id = ?",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[PlayerSessionStore] GetCharacterById prepare failed: %s\n", sqlite3_errmsg(db));
		return std::nullopt;
	}
	sqlite3_bind_int64(stmt, 1, id);

	std::optional<CharacterInfo> result;
	if (sqlite3_step(stmt) == SQLITE_ROW) {
		CharacterInfo c;
		c.id                   = sqlite3_column_int64(stmt, 0);
		c.user_id              = sqlite3_column_int64(stmt, 1);
		c.profile_id           = static_cast<uint32_t>(sqlite3_column_int(stmt, 2));
		c.head_asm_id          = static_cast<uint32_t>(sqlite3_column_int(stmt, 3));
		c.gender_type_value_id = static_cast<uint32_t>(sqlite3_column_int(stmt, 4));
		const void* blob = sqlite3_column_blob(stmt, 5);
		int bytes        = sqlite3_column_bytes(stmt, 5);
		if (blob && bytes > 0)
			c.morph_data.assign(static_cast<const uint8_t*>(blob),
			                    static_cast<const uint8_t*>(blob) + bytes);
		c.hair_asm_id          = static_cast<uint32_t>(sqlite3_column_int(stmt, 6));
		c.skin_mat_param_id    = static_cast<uint32_t>(sqlite3_column_int(stmt, 7));
		c.eye_mat_param_id     = static_cast<uint32_t>(sqlite3_column_int(stmt, 8));
		c.deleted_at           = sqlite3_column_int64(stmt, 9);
		result = std::move(c);
	}
	sqlite3_finalize(stmt);
	return result;
}

bool PlayerSessionStore::SoftDeleteCharacter(int64_t character_id, int64_t user_id) {
	std::lock_guard<std::mutex> lock(mutex_);
	sqlite3* db = Database::GetConnection();

	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"UPDATE ga_characters SET deleted_at = strftime('%s','now') "
		"WHERE id = ? AND user_id = ? AND deleted_at = 0",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[PlayerSessionStore] SoftDeleteCharacter prepare failed: %s\n", sqlite3_errmsg(db));
		return false;
	}
	sqlite3_bind_int64(stmt, 1, character_id);
	sqlite3_bind_int64(stmt, 2, user_id);
	if (sqlite3_step(stmt) != SQLITE_DONE)
		Logger::Log("db", "[PlayerSessionStore] SoftDeleteCharacter update failed: %s\n", sqlite3_errmsg(db));
	sqlite3_finalize(stmt);
	const bool deleted = sqlite3_changes(db) > 0;
	// Deleting an agent removes its bag + components (per-agent pool).
	if (deleted) DeleteAgentPool(user_id, character_id);
	return deleted;
}

// Per-agent bags. See the header + ControlServer Database.cpp
// (ga_user_active_agent). Parked rows: user_id = -character_id.
static void ExecAgentPoolSql(sqlite3* db, const char* sql, int64_t user_id, int64_t character_id,
                             const char* what) {
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db, sql, -1, &s, nullptr) != SQLITE_OK) {
		Logger::Log("db", "[agent-pool] %s prepare failed: %s\n", what, sqlite3_errmsg(db));
		return;
	}
	sqlite3_bind_int64(s, 1, user_id);
	sqlite3_bind_int64(s, 2, character_id);
	if (sqlite3_step(s) != SQLITE_DONE)
		Logger::Log("db", "[agent-pool] %s failed: %s\n", what, sqlite3_errmsg(db));
	sqlite3_finalize(s);
}

void PlayerSessionStore::ActivateAgentPool(int64_t user_id, int64_t character_id) {
	sqlite3* db = Database::GetConnection();
	if (!db || user_id <= 0 || character_id <= 0) return;

	// Owner of the untagged rows in the active pool: the agent that was active
	// when they were written (loot/vendor/craft/mail paths insert with
	// character_id 0), or this agent if none / it was deleted.
	int64_t prev = character_id;
	{
		sqlite3_stmt* s = nullptr;
		if (sqlite3_prepare_v2(db,
		        "SELECT a.character_id FROM ga_user_active_agent a JOIN ga_characters c "
		        "ON c.id = a.character_id AND c.user_id = a.user_id AND c.deleted_at = 0 WHERE a.user_id = ?",
		        -1, &s, nullptr) == SQLITE_OK) {
			sqlite3_bind_int64(s, 1, user_id);
			if (sqlite3_step(s) == SQLITE_ROW) prev = sqlite3_column_int64(s, 0);
			sqlite3_finalize(s);
		}
	}
	sqlite3_exec(db, "SAVEPOINT agent_pool", nullptr, nullptr, nullptr);
	// ?1 = user, ?2 = owner agent for untagged rows / the agent being activated.
	// Rows tagged with an agent that is not this account's (mail from another
	// player keeps the sender's tag) count as untagged.
	ExecAgentPoolSql(db,
		"UPDATE ga_players_inventory SET character_id = ?2 WHERE user_id = ?1 AND (character_id = 0 OR "
		" character_id NOT IN (SELECT id FROM ga_characters WHERE user_id = ?1 AND deleted_at = 0))",
		user_id, prev, "tag inventory");
	ExecAgentPoolSql(db,
		"UPDATE ga_user_components SET character_id = ?2 WHERE user_id = ?1 AND (character_id = 0 OR "
		" character_id NOT IN (SELECT id FROM ga_characters WHERE user_id = ?1 AND deleted_at = 0))",
		user_id, prev, "tag components");
	{
		// Park the other agents' rows, then bring this agent's back.
		ExecAgentPoolSql(db,
			"UPDATE OR IGNORE ga_players_inventory SET user_id = -character_id "
			"WHERE user_id = ?1 AND character_id <> ?2",
			user_id, character_id, "park inventory");
		ExecAgentPoolSql(db,
			"INSERT INTO ga_user_components (user_id, item_id, quantity, updated_at, character_id) "
			"SELECT -character_id, item_id, quantity, updated_at, character_id FROM ga_user_components "
			"WHERE user_id = ?1 AND character_id <> ?2 "
			"ON CONFLICT(user_id, item_id) DO UPDATE SET quantity = quantity + excluded.quantity",
			user_id, character_id, "park components");
		ExecAgentPoolSql(db,
			"DELETE FROM ga_user_components WHERE user_id = ?1 AND character_id <> ?2",
			user_id, character_id, "park components (clear)");
	}
	ExecAgentPoolSql(db,
		"UPDATE OR IGNORE ga_players_inventory SET user_id = ?1 WHERE user_id = -?2 AND character_id = ?2",
		user_id, character_id, "unpark inventory");
	ExecAgentPoolSql(db,
		"INSERT INTO ga_user_components (user_id, item_id, quantity, updated_at, character_id) "
		"SELECT ?1, item_id, quantity, updated_at, character_id FROM ga_user_components "
		"WHERE user_id = -?2 AND character_id = ?2 "
		"ON CONFLICT(user_id, item_id) DO UPDATE SET quantity = quantity + excluded.quantity",
		user_id, character_id, "unpark components");
	ExecAgentPoolSql(db,
		"DELETE FROM ga_user_components WHERE user_id = -?2 AND character_id = ?2 AND ?1 > 0",
		user_id, character_id, "unpark components (clear)");
	ExecAgentPoolSql(db,
		"INSERT OR REPLACE INTO ga_user_active_agent (user_id, character_id) VALUES (?1, ?2)",
		user_id, character_id, "set active");
	sqlite3_exec(db, "RELEASE agent_pool", nullptr, nullptr, nullptr);
	if (prev != character_id)
		Logger::Log("db", "[agent-pool] user %lld: agent %lld active (agent %lld parked)\n",
			(long long)user_id, (long long)character_id, (long long)prev);
}

void PlayerSessionStore::DeleteAgentPool(int64_t user_id, int64_t character_id) {
	sqlite3* db = Database::GetConnection();
	if (!db || user_id <= 0 || character_id <= 0) return;
	sqlite3_exec(db, "SAVEPOINT agent_pool_del", nullptr, nullptr, nullptr);
	// Active agent: untagged rows of the active pool are its too.
	ExecAgentPoolSql(db,
		"UPDATE ga_players_inventory SET character_id = ?2 WHERE user_id = ?1 AND character_id = 0 "
		"AND EXISTS (SELECT 1 FROM ga_user_active_agent WHERE user_id = ?1 AND character_id = ?2)",
		user_id, character_id, "tag inventory (delete)");
	ExecAgentPoolSql(db,
		"UPDATE ga_user_components SET character_id = ?2 WHERE user_id = ?1 AND character_id = 0 "
		"AND EXISTS (SELECT 1 FROM ga_user_active_agent WHERE user_id = ?1 AND character_id = ?2)",
		user_id, character_id, "tag components (delete)");
	ExecAgentPoolSql(db,
		"DELETE FROM ga_character_devices WHERE character_id = ?2 AND ?1 > 0",
		user_id, character_id, "delete devices");
	ExecAgentPoolSql(db,
		"DELETE FROM ga_players_inventory WHERE character_id = ?2 AND user_id IN (?1, -?2)",
		user_id, character_id, "delete inventory");
	ExecAgentPoolSql(db,
		"DELETE FROM ga_user_components WHERE character_id = ?2 AND user_id IN (?1, -?2)",
		user_id, character_id, "delete components");
	ExecAgentPoolSql(db,
		"DELETE FROM ga_user_active_agent WHERE user_id = ?1 AND character_id = ?2",
		user_id, character_id, "clear active");
	sqlite3_exec(db, "RELEASE agent_pool_del", nullptr, nullptr, nullptr);
	Logger::Log("db", "[agent-pool] user %lld: agent %lld deleted, its bag + components removed\n",
		(long long)user_id, (long long)character_id);
}

void PlayerSessionStore::SetSelectedCharacter(const std::string& session_guid, int64_t char_id, uint32_t profile_id) {
	std::lock_guard<std::mutex> lock(mutex_);
	auto it = by_guid_.find(session_guid);
	if (it != by_guid_.end()) {
		it->second.selected_character_id = char_id;
		it->second.selected_profile_id   = profile_id;
		by_ip_[it->second.ip_address].selected_character_id = char_id;
		by_ip_[it->second.ip_address].selected_profile_id   = profile_id;
	}
}

int PlayerSessionStore::GetEffectGroupId(int deviceId) {
	// Legacy scalar version — kept for the seed path that persists a single
	// representative effect_group_id on ga_character_devices. The authoritative
	// list used at response-time now comes from GetEffectGroupIds().
	switch (deviceId) {
		case 7031: return 26173;  // assault jetpack
		case 7032: return 26173;  // medic jetpack
		case 7033: return 26173;  // recon jetpack
		case 7034: return 26173;  // robotic jetpack
		case 2991: return 16670;  // agonizer
		case 2531: return 16653;  // healing grenade
		case 5800: return 22334;  // lifestealer
		case 2906: return 9071;   // adrenaline gun / specialty
		default:   return 0;
	}
}

int PlayerSessionStore::GetBlueprintIdForStatItem(int itemId) {
	static std::map<int, int> cache;
	static std::mutex cacheMutex;
	{
		std::lock_guard<std::mutex> lock(cacheMutex);
		auto it = cache.find(itemId);
		if (it != cache.end()) return it->second;
	}

	int blueprintId = 0;
	if (sqlite3* db = Database::GetConnection()) {
		// Plain-named first (the name must stay the item's own), then the one with the
		// most quality icons (some blueprints of an item have none, or none at the top
		// rarity), then one the bag can salvage, carrying wear, as the universal one did.
		sqlite3_stmt* stmt = nullptr;
		if (sqlite3_prepare_v2(db,
		    "SELECT b.blueprint_id FROM asm_data_set_blueprints b WHERE b.created_item_id = ? "
		    "ORDER BY (b.override_name_msg_id = 0) DESC, "
		    "(SELECT COUNT(*) FROM asm_data_set_blueprint_item_mods m WHERE m.blueprint_id = b.blueprint_id AND m.icon_id > 0) DESC, "
		    "(b.loot_table_group_id = 16) DESC, (b.durability > 0) DESC, "
		    "EXISTS (SELECT 1 FROM asm_data_set_blueprint_item_mods m WHERE m.blueprint_id = b.blueprint_id) DESC, "
		    "b.blueprint_id LIMIT 1", -1, &stmt, nullptr) == SQLITE_OK) {
			sqlite3_bind_int(stmt, 1, itemId);
			if (sqlite3_step(stmt) == SQLITE_ROW)
				blueprintId = sqlite3_column_int(stmt, 0);
		}
		sqlite3_finalize(stmt);
	}
	if (blueprintId == 0) blueprintId = ArmorLoadouts::kUniversalArmorBlueprintId;

	std::lock_guard<std::mutex> lock(cacheMutex);
	cache[itemId] = blueprintId;
	return blueprintId;
}

int PlayerSessionStore::GetBlueprintIdForDevice(int deviceId, bool oc) {
	// Cache: (device_id, oc) → blueprint_id. Looked up once per process per pair.
	static std::map<std::pair<int, bool>, int> cache;
	static std::mutex cacheMutex;

	const auto key = std::make_pair(deviceId, oc);
	{
		std::lock_guard<std::mutex> lock(cacheMutex);
		auto it = cache.find(key);
		if (it != cache.end()) return it->second;
	}

	int blueprintId = 0;
	sqlite3* db = Database::GetConnection();
	if (db) {
		// OC: prefer the lowest blueprint_id with override_name_msg_id != 0.
		// Standard: pick MIN(blueprint_id) regardless. If OC was requested but
		// no OC variant exists for this device, fall through to the standard
		// query so the suffix still renders.
		const char* sql = oc
			? "SELECT blueprint_id FROM asm_data_set_blueprints "
			  "WHERE created_item_id = ? AND override_name_msg_id != 0 "
			  "ORDER BY blueprint_id LIMIT 1"
			// A blueprint in salvage group 16 first - the client enables
			// bag Salvage only when the sent blueprint has that group (bp def +0xE4).
			: "SELECT blueprint_id FROM asm_data_set_blueprints "
			  "WHERE created_item_id = ? ORDER BY (loot_table_group_id = 16) DESC, blueprint_id LIMIT 1";
		sqlite3_stmt* stmt = nullptr;
		if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
			sqlite3_bind_int(stmt, 1, deviceId);
			if (sqlite3_step(stmt) == SQLITE_ROW)
				blueprintId = sqlite3_column_int(stmt, 0);
			sqlite3_finalize(stmt);
		}
		if (oc && blueprintId == 0) {
			if (sqlite3_prepare_v2(db,
			    "SELECT MIN(blueprint_id) FROM asm_data_set_blueprints WHERE created_item_id = ?",
			    -1, &stmt, nullptr) == SQLITE_OK) {
				sqlite3_bind_int(stmt, 1, deviceId);
				if (sqlite3_step(stmt) == SQLITE_ROW)
					blueprintId = sqlite3_column_int(stmt, 0);
				sqlite3_finalize(stmt);
			}
		}
	}

	{
		std::lock_guard<std::mutex> lock(cacheMutex);
		cache[key] = blueprintId;
	}
	return blueprintId;
}

std::vector<int> PlayerSessionStore::GetEffectGroupIds(int deviceId) {
	// Union of every effect group associated with this device in asm.dat:
	//   - asm_data_set_device_effect_groups  (equip effect — what the device
	//     stores at offset 0xE0 from DATA_SET_DEVICE_EFFECT_GROUPS)
	//   - asm_data_set_device_mode_effect_groups  (per-fire-mode effects,
	//     consulted by TgDeviceFire::GetEffectGroup at runtime)
	//
	// The client's FUN_10a13820 @ 0x10a13820 accumulates every row whose
	// INVENTORY_ID matches the inventory item, so we need to emit one
	// DATA_SET_INVENTORY_STATE entry per effect group.
	std::vector<int> result;

	sqlite3* db = Database::GetConnection();
	if (!db) return result;

	auto runQuery = [&](const char* sql) {
		sqlite3_stmt* stmt = nullptr;
		if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
			// Table may not exist yet (asm capture hasn't run) — quietly skip.
			return;
		}
		sqlite3_bind_int(stmt, 1, deviceId);
		while (sqlite3_step(stmt) == SQLITE_ROW) {
			int egid = sqlite3_column_int(stmt, 0);
			if (egid != 0) result.push_back(egid);
		}
		sqlite3_finalize(stmt);
	};

	runQuery(
		"SELECT DISTINCT effect_group_id FROM asm_data_set_device_effect_groups "
		"WHERE device_id = ? AND effect_group_id <> 0");
	runQuery(
		"SELECT DISTINCT effect_group_id FROM asm_data_set_device_mode_effect_groups "
		"WHERE device_id = ? AND effect_group_id <> 0");

	// Dedupe while preserving insertion order (equip effect first, then mode
	// effects in their natural order).
	std::vector<int> dedup;
	dedup.reserve(result.size());
	for (int egid : result) {
		bool seen = false;
		for (int x : dedup) if (x == egid) { seen = true; break; }
		if (!seen) dedup.push_back(egid);
	}

	if (!dedup.empty()) return dedup;

	// Fallback: asm tables empty or missing (pre-capture run). Use the legacy
	// hardcoded single effect so at least the already-working cases keep working.
	int legacy = GetEffectGroupId(deviceId);
	if (legacy != 0) dedup.push_back(legacy);
	return dedup;
}

// Format the rolled-mod effect_group_ids as a CSV for storage in
// ga_character_devices.mod_effect_group_ids. Empty vector → empty string.
static std::string ModsToCsv(const std::vector<int>& mods) {
	if (mods.empty()) return std::string();
	std::ostringstream oss;
	for (size_t i = 0; i < mods.size(); ++i) {
		if (i) oss << ',';
		oss << mods[i];
	}
	return oss.str();
}

// Inverse of ModsToCsv. Tolerates blanks and trailing commas.
static std::vector<int> CsvToMods(const char* csv) {
	std::vector<int> out;
	if (!csv || !*csv) return out;
	const char* p = csv;
	while (*p) {
		while (*p == ',' || *p == ' ') ++p;
		if (!*p) break;
		char* end = nullptr;
		long v = std::strtol(p, &end, 10);
		if (end == p) break;
		out.push_back(static_cast<int>(v));
		p = end;
	}
	return out;
}

// Engine equip point → SVID. Mirrors the binary's FUN_109a1320 table inverted.
// Used in GetDevicesForCharacter to set DeviceRow.slot_value_id from the
// stored equip slot, and in SaveEquippedDevices to figure out which SVID a
// given equip point represents so we can match it against an inventory item's
// allowed_slots whitelist. (The reverse direction — SVID→equip-point — has
// no caller here; SlotValueId() in src/GameServer/Constants/EquipSlot.hpp
// covers it on the game DLL side.)
static int EquipPointToSvid(int point) {
	static const int table[25] = {
		0,   221, 198, 200, 199, 201, 202, 203, 204, 385,
		386, 499, 500, 501, 502, 823, 996, 997, 998, 999,
		1000,1001,1002,1003,1004
	};
	return (point >= 1 && point <= 24) ? table[point] : 0;
}

// True when `equipped_slot` is one of the 7 group-129 armor SVIDs. Armor
// rows store the SVID directly in `ga_character_devices.equipped_slot`
// (no engine-equip-point indirection — armor doesn't bind to ES1..ES24),
// so `slot_value_id` on the DeviceRow is the same value the column already
// holds. See src/GameServer/Constants/EquipSlot.hpp ArmorSlot namespace
// for the full decode (MiscItems[] index = SVID - 1128).
static bool IsArmorSvid(int equipped_slot) {
	switch (equipped_slot) {
		case 1130: /*Head*/     case 1132: /*Hands*/     case 1133: /*Chest*/
		case 1136: /*Arms*/     case 1139: /*Legs*/      case 1142: /*Feet*/
		case 1143: /*Shoulder*/
			return true;
		default:
			return false;
	}
}

// CSV list of slot_value_ids an inventory item may occupy. Offhand devices
// (engine points 7/8/9) are interchangeable — let the player put a grenade
// in any of the three offhand slots. Everything else is single-slot.
static std::string AllowedSlotsCsvFor(int slot_value_id) {
	const bool offhand = (slot_value_id == 203 || slot_value_id == 204 || slot_value_id == 385);
	if (offhand) return "203,204,385";
	char buf[16];
	std::snprintf(buf, sizeof(buf), "%d", slot_value_id);
	return std::string(buf);
}

static std::vector<int> CsvToInts(const char* csv) {
	std::vector<int> out;
	if (!csv || !*csv) return out;
	const char* p = csv;
	while (*p) {
		while (*p == ',' || *p == ' ') ++p;
		if (!*p) break;
		char* end = nullptr;
		long v = std::strtol(p, &end, 10);
		if (end == p) break;
		out.push_back(static_cast<int>(v));
		p = end;
	}
	return out;
}

// Starter-kit mode (config starter_kit_only, default on).
// New agents get only a level-1 class kit; everything else is earned/bought.
static bool g_starterKitOnly = true;
static int  g_starterKitMaxLevel = 5;

void PlayerSessionStore::SetStarterKit(bool only, int max_level) {
	g_starterKitOnly = only;
	g_starterKitMaxLevel = max_level;
	Logger::Log("db", "[PlayerSessionStore] starter_kit_only=%d starter_kit_max_level=%d\n",
		only ? 1 : 0, max_level);
}

// Token limits per agent (config token_limits, 0 = off).
static int64_t g_tokenWeeklyEarn = 0;
static int64_t g_tokenBank = 0;

void PlayerSessionStore::SetTokenLimits(int64_t weekly_earn, int64_t bank) {
	g_tokenWeeklyEarn = std::max<int64_t>(0, weekly_earn);
	g_tokenBank = std::max<int64_t>(0, bank);
	Logger::Log("db", "[PlayerSessionStore] token_limits weekly_earn=%lld bank=%lld\n",
		(long long)g_tokenWeeklyEarn, (long long)g_tokenBank);
}

int64_t PlayerSessionStore::LimitTokenGain(int64_t character_id, int64_t add) {
	if (add <= 0 || (g_tokenWeeklyEarn <= 0 && g_tokenBank <= 0)) return add;
	sqlite3* db = Database::GetConnection();
	if (!db) return add;
	// Weeks start Monday 00:00 UTC (1970-01-01 was a Thursday).
	const int64_t week = ((int64_t)std::time(nullptr) + 3 * 86400) / (7 * 86400);
	sqlite3_exec(db,
		"CREATE TABLE IF NOT EXISTS ga_token_week (character_id INTEGER NOT NULL, "
		"week INTEGER NOT NULL, earned INTEGER NOT NULL DEFAULT 0, "
		"PRIMARY KEY (character_id, week))", nullptr, nullptr, nullptr);
	int64_t allowed = add;
	sqlite3_stmt* s = nullptr;
	if (g_tokenBank > 0 && sqlite3_prepare_v2(db,
	        "SELECT tokens FROM ga_characters WHERE id = ?", -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int64(s, 1, character_id);
		if (sqlite3_step(s) == SQLITE_ROW)
			allowed = std::min(allowed, std::max<int64_t>(0, g_tokenBank - sqlite3_column_int64(s, 0)));
		sqlite3_finalize(s);
	}
	if (g_tokenWeeklyEarn > 0 && sqlite3_prepare_v2(db,
	        "SELECT earned FROM ga_token_week WHERE character_id = ? AND week = ?",
	        -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int64(s, 1, character_id);
		sqlite3_bind_int64(s, 2, week);
		const int64_t earned = sqlite3_step(s) == SQLITE_ROW ? sqlite3_column_int64(s, 0) : 0;
		sqlite3_finalize(s);
		allowed = std::min(allowed, std::max<int64_t>(0, g_tokenWeeklyEarn - earned));
	}
	if (allowed > 0 && sqlite3_prepare_v2(db,
	        "INSERT INTO ga_token_week (character_id, week, earned) VALUES (?1, ?2, ?3) "
	        "ON CONFLICT(character_id, week) DO UPDATE SET earned = earned + ?3",
	        -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int64(s, 1, character_id);
		sqlite3_bind_int64(s, 2, week);
		sqlite3_bind_int64(s, 3, allowed);
		sqlite3_step(s);
		sqlite3_finalize(s);
	}
	if (allowed < add)
		Logger::Log("rewards", "[Rewards] char=%lld token limit: +%lld -> +%lld\n",
			(long long)character_id, (long long)add, (long long)allowed);
	return allowed;
}

// The class's starter kit, built from its upstream loadout list: per equip
// slot the entry with the lowest required_level in the game data
// (asm_data_set_items), kept only if that level <= g_starterKitMaxLevel,
// as COMMON quality with no rolled mods (rolled mods raise the level the
// client asks for). Cached per profile.
static const std::set<int> kTutorialKitDevices = {
	5837, 5838, 5839, 5840,  // class Hands Free jetpacks
	3973, 3970, 3967, 3951,  // Radiant Axe, Ghost Sword, Poison Injector, EnergyBurn Mace
};

static const std::vector<Loadouts::GearSlot>& StarterLoadout(uint32_t profile_id) {
	static std::map<uint32_t, std::vector<Loadouts::GearSlot>> cache;
	auto it = cache.find(profile_id);
	if (it != cache.end()) return it->second;
	std::vector<Loadouts::GearSlot>& out = cache[profile_id];
	sqlite3* db = Database::GetConnection();
	sqlite3_stmt* st = nullptr;
	if (!db || sqlite3_prepare_v2(db,
	        "SELECT MIN(required_level) FROM asm_data_set_items WHERE item_id = ?",
	        -1, &st, nullptr) != SQLITE_OK) {
		if (st) sqlite3_finalize(st);
		cache.erase(profile_id);
		static const std::vector<Loadouts::GearSlot> empty;
		return empty;
	}
	std::map<int, std::pair<int, Loadouts::GearSlot>> best;  // slot svid -> (level, gear)
	for (const Loadouts::GearSlot& g : Loadouts::GetLoadout(profile_id)) {
		sqlite3_reset(st);
		sqlite3_bind_int(st, 1, g.device_id);
		int lvl = 99;
		if (sqlite3_step(st) == SQLITE_ROW && sqlite3_column_type(st, 0) != SQLITE_NULL)
			lvl = sqlite3_column_int(st, 0);
		// The Inception tutorial's own per-class jetpack + melee (its Kismet
		// EquipCharDevices lists in Inception_2/3_TEMP) are the original
		// level-1 kit: they win over other items of the same slot.
		if (kTutorialKitDevices.count(g.device_id) && lvl <= g_starterKitMaxLevel) lvl = -1;
		auto b = best.find(g.slot_value_id);
		if (b == best.end() || lvl < b->second.first) best[g.slot_value_id] = { lvl, g };
	}
	sqlite3_finalize(st);
	for (auto& kv : best) {
		if (kv.second.first > g_starterKitMaxLevel) continue;
		Loadouts::GearSlot g = kv.second.second;
		g.quality = Loadouts::Q_COMMON;
		g.mods = Mods::Result{};
		out.push_back(g);
		Logger::Log("db", "[PlayerSessionStore] starter kit profile=%u slot=%d device=%d level=%d\n",
			profile_id, g.slot_value_id, g.device_id, kv.second.first);
	}
	StarterOffhands::Apply(profile_id, g_starterKitMaxLevel, out);
	return out;
}

// Starter cosmetics: only the class default helmet + suit (no dye/trail
// collection - those come from the dye vendors / drops).
static void SeedStarterCosmetics(int64_t user_id) {
	sqlite3* db = Database::GetConnection();
	if (!db) return;
	sqlite3_stmt* st = nullptr;
	if (sqlite3_prepare_v2(db,
	        "INSERT OR IGNORE INTO ga_players_inventory"
	        "  (user_id, profile_id, device_id, quality, mod_effect_group_ids, oc, allowed_slots, item_id, stock_n) "
	        "VALUES (?, ?, 0, 0, '', 0, ?, ?, 0)",
	        -1, &st, nullptr) != SQLITE_OK) {
		if (st) sqlite3_finalize(st);
		return;
	}
	const uint32_t profiles[] = { Loadouts::PROFILE_MEDIC, Loadouts::PROFILE_ROBOTICS,
	                              Loadouts::PROFILE_ASSAULT, Loadouts::PROFILE_RECON };
	for (uint32_t pid : profiles) {
		const auto& d = CosmeticLoadouts::GetDefaultsForProfile(pid);
		const struct { int item; const char* allowed; } rows[] = {
			{ d.helmet_item_id, "500" }, { d.suit_item_id, "202" } };
		for (const auto& r : rows) {
			if (r.item <= 0) continue;
			sqlite3_reset(st);
			sqlite3_bind_int64(st, 1, user_id);
			sqlite3_bind_int  (st, 2, (int)pid);
			sqlite3_bind_text (st, 3, r.allowed, -1, SQLITE_STATIC);
			sqlite3_bind_int  (st, 4, r.item);
			sqlite3_step(st);
		}
	}
	sqlite3_finalize(st);
}

void PlayerSessionStore::SeedInventoryFromLoadouts(int64_t user_id) {
	sqlite3* db = Database::GetConnection();
	if (!db) return;

	// v76: seed cosmetic ownership (idempotent) before the device pass.
	// Cosmetic rows have item_id > 0 and are scoped out of the device
	// delete pass below via the `AND item_id = 0` clause.
	if (g_starterKitOnly) SeedStarterCosmetics(user_id);
	else                  SeedCosmetics(user_id);

	// Armor seed — same shape as SeedCosmetics. Armor rows also have
	// item_id > 0 so they're scoped out of the device delete pass below by
	// the same `AND item_id = 0` clause; no separate guard needed.
	SeedArmor(user_id);

	// Build the "what should exist" set up front. Keyed by
	// (profile_id, device_id, mods_csv, quality, oc) — same tuple the insert
	// pass uses for its dedupe check. Used twice below: insert pass skips
	// tuples already in inventory, delete pass removes inventory rows whose
	// tuple isn't here anymore (= ClassLoadouts entry was deleted).
	auto LoadoutFor = [](uint32_t pid) {
		return g_starterKitOnly ? &StarterLoadout(pid) : &Loadouts::GetLoadout(pid);
	};
	struct ProfileLoadout { uint32_t profile_id; const std::vector<Loadouts::GearSlot>* loadout; };
	const ProfileLoadout profiles[] = {
		// starter_kit_only: the level-1 class kit instead of the full upstream
		// loadout. Rows outside the desired set are deleted below, which also
		// trims existing agents' free gear on their next login.
		{ Loadouts::PROFILE_MEDIC,    LoadoutFor(Loadouts::PROFILE_MEDIC) },
		{ Loadouts::PROFILE_ROBOTICS, LoadoutFor(Loadouts::PROFILE_ROBOTICS) },
		{ Loadouts::PROFILE_ASSAULT,  LoadoutFor(Loadouts::PROFILE_ASSAULT) },
		{ Loadouts::PROFILE_RECON,    LoadoutFor(Loadouts::PROFILE_RECON) },
	};

	using DesiredKey = std::tuple<int, int, std::string, int, int>;  // profile, device, mods, quality, oc
	std::set<DesiredKey> desired;
	for (const auto& pl : profiles) {
		for (const Loadouts::GearSlot& g : *pl.loadout) {
			// Class device (HUMAN BASE ATTRIBUTES, slot 14) used to be skipped
			// from the inventory pool — it was attached at spawn via a synthetic
			// invId, which corrupted the inventory TMap on cleanup. It's a
			// normal inventory row now; SaveEquippedDevices pins it to slot 14
			// regardless of what the client sends so the player can't un-equip it.
			const std::string modsCsv = ModsToCsv(ModResolver::Resolve(g.device_id, g.quality, g.mods));
			desired.insert({ (int)pl.profile_id, g.device_id, modsCsv, g.quality, g.mods.oc ? 1 : 0 });
		}
	}

	// (0) DELETE pass — remove inventory rows whose tuple is no longer in
	//     ClassLoadouts. Cascades to ga_character_devices (FK isn't enforced
	//     in SQLite without PRAGMA, so we do it manually) so equip slots get
	//     freed when their item disappears.
	//
	//     ASSUMPTION: every row in ga_players_inventory came from a previous
	//     seed pass. Today that's true (no user-acquisition path exists).
	//     When loot drops / store purchases come online we'll need a
	//     `source` column to discriminate seed-sourced rows from user-owned
	//     rows so this pass doesn't nuke earned items.
	int deletedInvRows = 0;
	int deletedDevRows = 0;
	{
		sqlite3_stmt* listStmt = nullptr;
		// v76: scope the delete pass to device rows (item_id = 0). Cosmetic
		// rows (item_id > 0) live in the same table now but aren't driven by
		// ClassLoadouts.cpp — they're managed by SeedCosmetics. Without this
		// gate the delete pass would nuke every cosmetic on every login.
		if (sqlite3_prepare_v2(db,
		    "SELECT id, profile_id, device_id, mod_effect_group_ids, quality, oc "
		    "FROM ga_players_inventory WHERE user_id = ? AND item_id = 0",
		    -1, &listStmt, nullptr) == SQLITE_OK) {
			sqlite3_bind_int64(listStmt, 1, user_id);
			std::vector<int> toDelete;
			while (sqlite3_step(listStmt) == SQLITE_ROW) {
				const int id  = sqlite3_column_int(listStmt, 0);
				const int pid = sqlite3_column_int(listStmt, 1);
				const int dev = sqlite3_column_int(listStmt, 2);
				const char* m = (const char*)sqlite3_column_text(listStmt, 3);
				const int q   = sqlite3_column_int(listStmt, 4);
				const int oc  = sqlite3_column_int(listStmt, 5);
				if (desired.find({ pid, dev, std::string(m ? m : ""), q, oc }) == desired.end()) {
					toDelete.push_back(id);
				}
			}
			sqlite3_finalize(listStmt);

			if (!toDelete.empty()) {
				sqlite3_stmt* delDev = nullptr;
				sqlite3_stmt* delInv = nullptr;
				const bool prepOk =
					sqlite3_prepare_v2(db,
					    "DELETE FROM ga_character_devices WHERE inventory_id = ?",
					    -1, &delDev, nullptr) == SQLITE_OK &&
					sqlite3_prepare_v2(db,
					    "DELETE FROM ga_players_inventory WHERE id = ?",
					    -1, &delInv, nullptr) == SQLITE_OK;
				if (prepOk) {
					for (int id : toDelete) {
						sqlite3_reset(delDev);
						sqlite3_clear_bindings(delDev);
						sqlite3_bind_int(delDev, 1, id);
						sqlite3_step(delDev);
						deletedDevRows += sqlite3_changes(db);

						sqlite3_reset(delInv);
						sqlite3_clear_bindings(delInv);
						sqlite3_bind_int(delInv, 1, id);
						sqlite3_step(delInv);
						deletedInvRows += sqlite3_changes(db);
					}
				} else {
					Logger::Log("db",
						"[PlayerSessionStore] Seed delete prepare failed: %s\n", sqlite3_errmsg(db));
				}
				if (delDev) sqlite3_finalize(delDev);
				if (delInv) sqlite3_finalize(delInv);
			}
		}
	}

	// (1) For every (profile, gear) row in ClassLoadouts.cpp: insert into
	//     ga_players_inventory if a matching (user, profile, device, mods,
	//     quality, oc) tuple isn't there yet. Idempotent across logins.
	sqlite3_stmt* selStmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"SELECT id FROM ga_players_inventory "
		"WHERE user_id = ? AND profile_id = ? AND device_id = ? "
		"  AND quality = ? AND mod_effect_group_ids = ? AND oc = ? LIMIT 1",
		-1, &selStmt, nullptr);
	if (rc != SQLITE_OK || !selStmt) {
		Logger::Log("db", "[PlayerSessionStore] Seed sel prepare failed: %s\n", sqlite3_errmsg(db));
		return;
	}
	sqlite3_stmt* insStmt = nullptr;
	rc = sqlite3_prepare_v2(db,
		"INSERT INTO ga_players_inventory "
		"(user_id, profile_id, device_id, quality, mod_effect_group_ids, oc, allowed_slots) "
		"VALUES (?, ?, ?, ?, ?, ?, ?)",
		-1, &insStmt, nullptr);
	if (rc != SQLITE_OK || !insStmt) {
		Logger::Log("db", "[PlayerSessionStore] Seed ins prepare failed: %s\n", sqlite3_errmsg(db));
		sqlite3_finalize(selStmt);
		return;
	}

	int inserted = 0;
	for (const auto& pl : profiles) {
		for (const Loadouts::GearSlot& g : *pl.loadout) {
			const std::vector<int> egids = ModResolver::Resolve(g.device_id, g.quality, g.mods);
			const std::string modsCsv = ModsToCsv(egids);

			sqlite3_reset(selStmt);
			sqlite3_clear_bindings(selStmt);
			sqlite3_bind_int64(selStmt, 1, user_id);
			sqlite3_bind_int(selStmt,   2, (int)pl.profile_id);
			sqlite3_bind_int(selStmt,   3, g.device_id);
			sqlite3_bind_int(selStmt,   4, g.quality);
			sqlite3_bind_text(selStmt,  5, modsCsv.c_str(), -1, SQLITE_TRANSIENT);
			sqlite3_bind_int(selStmt,   6, g.mods.oc ? 1 : 0);
			if (sqlite3_step(selStmt) == SQLITE_ROW) {
				continue;  // already in inventory; preserve stable id
			}

			const std::string allowed = AllowedSlotsCsvFor(g.slot_value_id);
			sqlite3_reset(insStmt);
			sqlite3_clear_bindings(insStmt);
			sqlite3_bind_int64(insStmt, 1, user_id);
			sqlite3_bind_int(insStmt,   2, (int)pl.profile_id);
			sqlite3_bind_int(insStmt,   3, g.device_id);
			sqlite3_bind_int(insStmt,   4, g.quality);
			sqlite3_bind_text(insStmt,  5, modsCsv.c_str(), -1, SQLITE_TRANSIENT);
			sqlite3_bind_int(insStmt,   6, g.mods.oc ? 1 : 0);
			sqlite3_bind_text(insStmt,  7, allowed.c_str(),  -1, SQLITE_TRANSIENT);
			sqlite3_step(insStmt);
			++inserted;
		}
	}
	sqlite3_finalize(selStmt);
	sqlite3_finalize(insStmt);

	// Per-character opening-loadout / slot-14 pin / cosmetic-default /
	// armor-default seeding REMOVED. Empty ga_character_devices means
	// nothing equipped on the character — period. The player equips from
	// the bag via the equip screen. ga_players_inventory pool is still
	// seeded above so items exist to equip from.

	Logger::Log("db",
		"[PlayerSessionStore] Seed: user=%lld inventory+=%d inventory-=%d device-rows-cleared=%d\n",
		user_id, inserted, deletedInvRows, deletedDevRows);
}

void PlayerSessionStore::SeedCosmetics(int64_t user_id) {
	// No lock: matches SeedInventoryFromLoadouts's pattern. Callers that hold
	// mutex_ (e.g. InsertCharacter → SeedInventoryFromLoadouts) would deadlock
	// on a recursive acquire since mutex_ is non-recursive.
	sqlite3* db = Database::GetConnection();
	if (!db) return;

	// Pass 1: helmets (1006), helmet flairs (1007), suits (1008), trails (1612)
	// One row each. profile_id derived from skill_id (per-class appearance
	// skills are 425-432 and 496-497; everything else falls to shared 0).
	//
	// CRITICAL: store i.item_id (the game-logical id), NOT i.id (the row
	// PK). The wire ITEM_ID field and the engine natives (HeadFlairId etc.)
	// both expect the game item_id. Storing the PK leads to cosmetics
	// rendering as random unrelated items on the client (v76 seed bug,
	// cleaned up by v77).
	const std::string kSeedHelmsSuitsTrails =
		"INSERT OR IGNORE INTO ga_players_inventory"
		"  (user_id, profile_id, device_id, quality, mod_effect_group_ids, oc, allowed_slots, item_id, stock_n) "
		"SELECT ?, "
		"       CASE i.skill_id "
		"         WHEN 427 THEN 680 WHEN 428 THEN 680 "  // Assault Suits / Helmets
		"         WHEN 431 THEN 567 WHEN 432 THEN 567 "  // Medic
		"         WHEN 425 THEN 679 WHEN 426 THEN 679 "  // Robotics
		"         WHEN 497 THEN 681 WHEN 496 THEN 681 "  // Recon
		"         ELSE 0 "
		"       END, "
		"       0, 0, '', 0, "
		"       CASE "
		"         WHEN i.item_subtype_value_id IN (1006, 1007) THEN '500' "      // ES12 Helmet
		"         WHEN i.item_subtype_value_id = 1008          THEN '202' "      // ES6  Suit
		"         WHEN i.item_type_value_id    = 1612          THEN '1001' "     // ES21 Trail
		"       END, "
		"       i.item_id, 0 "
		"FROM asm_data_set_items i "
		"WHERE (i.item_type_value_id = 1612 "
		"   OR (i.item_type_value_id = 950 AND i.item_subtype_value_id IN (1006, 1007, 1008)))"
		+ (BlacklistNotInClause().empty() ? std::string()
		                                  : " AND i.item_id" + BlacklistNotInClause());
	{
		sqlite3_stmt* stmt = nullptr;
		if (sqlite3_prepare_v2(db, kSeedHelmsSuitsTrails.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
			sqlite3_bind_int64(stmt, 1, user_id);
			sqlite3_step(stmt);
			sqlite3_finalize(stmt);
		} else {
			Logger::Log("cosmetic-seed",
				"SeedCosmetics: prepare(helmets/suits/trails) failed: %s\n",
				sqlite3_errmsg(db));
		}
	}

	// Pass 2: dyes (1020) — 5 stock copies each. allowed_slots lists all 5
	// dye slots so any copy can be equipped to any dye slot. Uses i.item_id
	// (game id), not i.id (row PK) — same reason as Pass 1.
	const std::string kSeedDyes =
		"INSERT OR IGNORE INTO ga_players_inventory"
		"  (user_id, profile_id, device_id, quality, mod_effect_group_ids, oc, allowed_slots, item_id, stock_n) "
		"SELECT ?, 0, 0, 0, '', 0, '996,997,998,999,1000', i.item_id, n.n "
		"FROM asm_data_set_items i "
		"CROSS JOIN (SELECT 0 AS n UNION SELECT 1 UNION SELECT 2 UNION SELECT 3 UNION SELECT 4) n "
		"WHERE i.item_type_value_id = 1020"
		+ (BlacklistNotInClause().empty() ? std::string()
		                                  : " AND i.item_id" + BlacklistNotInClause());
	{
		sqlite3_stmt* stmt = nullptr;
		if (sqlite3_prepare_v2(db, kSeedDyes.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
			sqlite3_bind_int64(stmt, 1, user_id);
			sqlite3_step(stmt);
			sqlite3_finalize(stmt);
		} else {
			Logger::Log("cosmetic-seed", "SeedCosmetics: prepare(dyes) failed: %s\n",
				sqlite3_errmsg(db));
		}
	}

	Logger::Log("cosmetic-seed", "SeedCosmetics: user=%lld done\n", (long long)user_id);
}

void PlayerSessionStore::SeedCharacterCosmeticDefaults(int64_t character_id, uint32_t profile_id) {
	// No lock: same rationale as SeedCosmetics — caller may already hold mutex_.
	sqlite3* db = Database::GetConnection();
	if (!db) return;

	const auto& defaults = CosmeticLoadouts::GetDefaultsForProfile(profile_id);

	int64_t user_id = 0;
	{
		sqlite3_stmt* s = nullptr;
		if (sqlite3_prepare_v2(db,
		        "SELECT user_id FROM ga_characters WHERE id = ?",
		        -1, &s, nullptr) == SQLITE_OK) {
			sqlite3_bind_int64(s, 1, character_id);
			if (sqlite3_step(s) == SQLITE_ROW) {
				user_id = sqlite3_column_int64(s, 0);
			}
			sqlite3_finalize(s);
		}
	}
	if (user_id == 0) {
		Logger::Log("cosmetic-seed",
			"SeedCharacterCosmeticDefaults: no user for char=%lld\n",
			(long long)character_id);
		return;
	}

	// Cosmetic suit/helmet defaults seed at the DB-internal slots (22/23) so
	// they don't collide with the gameplay suit/helmet device rows at slot
	// 6/12 from SeedInventoryFromLoadouts. Dyes and jetpack trail keep their
	// engine slots — they don't collide with anything.
	struct Slot { int db_slot; int item_id; };
	const Slot slots[] = {
		{ CosmeticSlots::kCosmeticHelmetDbSlot, defaults.helmet_item_id   },
		{ CosmeticSlots::kCosmeticSuitDbSlot,   defaults.suit_item_id     },
		{ 16, defaults.dye_primary      },
		{ 17, defaults.dye_secondary    },
		{ 18, defaults.dye_emissive     },
		{ 19, defaults.dye_weapon_pri   },
		{ 20, defaults.dye_weapon_emi   },
		{ 21, defaults.jetpack_trail    },
	};

	const char* kFindInv =
		"SELECT id FROM ga_players_inventory "
		"WHERE user_id = ? AND item_id = ? "
		"ORDER BY stock_n ASC LIMIT 1";
	// Defaults land only in profile 1. Other profiles stay visually blank
	// until the player explicitly saves cosmetics there.
	const char* kInsertEquip =
		"INSERT OR IGNORE INTO ga_character_devices (character_id, item_profile_id, inventory_id, equipped_slot) "
		"VALUES (?, 1, ?, ?)";

	for (const auto& sl : slots) {
		if (sl.item_id == 0) continue;
		if (IsBlacklistedItemId(sl.item_id)) {
			Logger::Log("blacklist",
				"SeedCharacterCosmeticDefaults: skipping blacklisted default item=%d (char=%lld db_slot=%d)\n",
				sl.item_id, (long long)character_id, sl.db_slot);
			continue;
		}

		int invId = 0;
		sqlite3_stmt* selStmt = nullptr;
		if (sqlite3_prepare_v2(db, kFindInv, -1, &selStmt, nullptr) == SQLITE_OK) {
			sqlite3_bind_int64(selStmt, 1, user_id);
			sqlite3_bind_int  (selStmt, 2, sl.item_id);
			if (sqlite3_step(selStmt) == SQLITE_ROW) {
				invId = sqlite3_column_int(selStmt, 0);
			}
			sqlite3_finalize(selStmt);
		}
		if (invId == 0) {
			Logger::Log("cosmetic-seed",
				"SeedCharacterCosmeticDefaults: no inv row for char=%lld user=%lld item=%d (db_slot %d)\n",
				(long long)character_id, (long long)user_id, sl.item_id, sl.db_slot);
			continue;
		}

		sqlite3_stmt* insStmt = nullptr;
		if (sqlite3_prepare_v2(db, kInsertEquip, -1, &insStmt, nullptr) == SQLITE_OK) {
			sqlite3_bind_int64(insStmt, 1, character_id);
			sqlite3_bind_int  (insStmt, 2, invId);
			sqlite3_bind_int  (insStmt, 3, sl.db_slot);
			sqlite3_step(insStmt);
			sqlite3_finalize(insStmt);
		}
	}

	Logger::Log("cosmetic-seed",
		"SeedCharacterCosmeticDefaults: char=%lld profile=%u done\n",
		(long long)character_id, profile_id);
}

void PlayerSessionStore::PurgeBlacklistedItems(int64_t user_id) {
	// No lock: matches the other seed helpers — callers (TcpSession login path)
	// don't hold mutex_, and the DB writes are protected by SQLite's own
	// concurrency.
	constexpr size_t kBlacklistSize =
		sizeof(kBlacklistedItemIds) / sizeof(kBlacklistedItemIds[0]);
	if (kBlacklistSize == 0) return;

	sqlite3* db = Database::GetConnection();
	if (!db) return;

	const std::string in_clause = BlacklistNotInClause();
	// Convert " NOT IN (…)" → " IN (…)" by slicing off the leading " NOT".
	const std::string in_only = in_clause.substr(4);

	// Pre-pass: enumerate what will be deleted so we can log per-row context.
	{
		const std::string sql =
			"SELECT cd.character_id, inv.item_id, cd.equipped_slot, cd.item_profile_id "
			"FROM ga_character_devices cd "
			"JOIN ga_players_inventory inv ON inv.id = cd.inventory_id "
			"WHERE inv.user_id = ? AND inv.item_id" + in_only;
		sqlite3_stmt* stmt = nullptr;
		if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
			sqlite3_bind_int64(stmt, 1, user_id);
			while (sqlite3_step(stmt) == SQLITE_ROW) {
				int64_t cid  = sqlite3_column_int64(stmt, 0);
				int     iid  = sqlite3_column_int  (stmt, 1);
				int     slot = sqlite3_column_int  (stmt, 2);
				int     pid  = sqlite3_column_int  (stmt, 3);
				Logger::Log("blacklist",
					"purging equipped char=%lld profile=%d slot=%d item_id=%d\n",
					(long long)cid, pid, slot, iid);
			}
			sqlite3_finalize(stmt);
		}
	}

	// Transaction so the equip-row purge and the inventory-row purge can't
	// observe a half-applied state.
	char* err = nullptr;
	sqlite3_exec(db, "BEGIN IMMEDIATE", nullptr, nullptr, &err);
	if (err) { sqlite3_free(err); err = nullptr; }

	int equipped_rows = 0;
	int inventory_rows = 0;

	// Step 1: unequip — drop any ga_character_devices row whose inventory_id
	// points at a blacklisted item owned by this user. Has to come first
	// (before the inventory DELETE) because the subquery joins through
	// ga_players_inventory.
	{
		const std::string sql =
			"DELETE FROM ga_character_devices "
			"WHERE inventory_id IN ("
			"  SELECT id FROM ga_players_inventory "
			"  WHERE user_id = ? AND item_id" + in_only +
			")";
		sqlite3_stmt* stmt = nullptr;
		if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
			sqlite3_bind_int64(stmt, 1, user_id);
			if (sqlite3_step(stmt) == SQLITE_DONE) {
				equipped_rows = sqlite3_changes(db);
			}
			sqlite3_finalize(stmt);
		} else {
			Logger::Log("blacklist", "PurgeBlacklistedItems: prepare(equip-del) failed: %s\n",
				sqlite3_errmsg(db));
		}
	}

	// Step 2: remove from inventory.
	{
		const std::string sql =
			"DELETE FROM ga_players_inventory "
			"WHERE user_id = ? AND item_id" + in_only;
		sqlite3_stmt* stmt = nullptr;
		if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
			sqlite3_bind_int64(stmt, 1, user_id);
			if (sqlite3_step(stmt) == SQLITE_DONE) {
				inventory_rows = sqlite3_changes(db);
			}
			sqlite3_finalize(stmt);
		} else {
			Logger::Log("blacklist", "PurgeBlacklistedItems: prepare(inv-del) failed: %s\n",
				sqlite3_errmsg(db));
		}
	}

	sqlite3_exec(db, "COMMIT", nullptr, nullptr, &err);
	if (err) { sqlite3_free(err); err = nullptr; }

	if (equipped_rows > 0 || inventory_rows > 0) {
		Logger::Log("blacklist",
			"PurgeBlacklistedItems: user=%lld unequipped=%d inventory_removed=%d\n",
			(long long)user_id, equipped_rows, inventory_rows);
	}
}

void PlayerSessionStore::SeedArmor(int64_t user_id) {
	// No lock: matches SeedCosmetics's pattern. Callers may already hold mutex_.
	sqlite3* db = Database::GetConnection();
	if (!db) return;

	// 7 slots × 5 variants = 35 rows. stock_n is the variant index (0..4) so
	// the existing UNIQUE(user_id, item_id, stock_n) index gives us natural
	// idempotency with no extra dedupe logic.
	//
	// Per-row fixed fields:
	//   user_id        — bound
	//   profile_id     — 0 (armor is shared across class profiles — every
	//                       class wears the same armor pool)
	//   device_id      — 0 (armor pieces have no backing device; the
	//                       Inventory marshal's cosmetic-style path applies)
	//   quality        — 1162 (Q_EPIC; matches 6-mod variants)
	//   mod_effect_group_ids — variant CSV from ArmorLoadouts::kVariants
	//   oc             — 0
	//   allowed_slots  — the slot's group-126 SVID as a decimal string
	//                    (e.g. "1107" for Head). Lets a future swap UI
	//                    refuse cross-slot equips at validate time.
	//   item_id        — slot's base item_id from ArmorLoadouts::kSlots
	//   stock_n        — variant index
	sqlite3_stmt* stmt = nullptr;
	const char* kSeed =
		"INSERT OR IGNORE INTO ga_players_inventory"
		"  (user_id, profile_id, device_id, quality, mod_effect_group_ids, oc, allowed_slots, item_id, stock_n) "
		"VALUES (?, 0, 0, 1162, ?, 0, ?, ?, ?)";
	if (sqlite3_prepare_v2(db, kSeed, -1, &stmt, nullptr) != SQLITE_OK) {
		Logger::Log("armor-seed", "SeedArmor: prepare failed: %s\n", sqlite3_errmsg(db));
		return;
	}

	int inserted = 0;
	for (int si = 0; si < ArmorLoadouts::kSlotCount; ++si) {
		const auto& slot = ArmorLoadouts::kSlots[si];
		const std::string allowed = std::to_string(slot.slot_value_id);
		// starter_kit_only: no free epic variants - only the "No Armor" rows below.
		for (int vi = 0; vi < (g_starterKitOnly ? 0 : ArmorLoadouts::kVariantCount); ++vi) {
			const auto& variant = ArmorLoadouts::kVariants[vi];
			// Prepend the slot's baseline +10% Health Mod egid to the variant's
			// rolled-mod CSV. Two effects in one step:
			//   1) The wire's DATA_SET_INVENTORY_STATE will list this egid, so
			//      the client's item tooltip renders the +10% Health Mod line.
			//   2) Armor.cpp's per-egid ApplyBuff fanout naturally picks up
			//      the prop-390 base-10 effect from this egid and folds it
			//      into MaxHP (no hardcoded ApplyBuff(412,...) needed any more).
			// Order within the CSV doesn't affect math — every egid is one
			// stacking instance. The base egid's prop (390) has no ui_code so
			// it doesn't render a letter, leaving the visible suffix as
			// `[rrrrrr]` / `[nnnnnn]` / `[bbbbbb]` / `[mmmmmm]` / `[rrrnnn]`.
			std::string csv = std::to_string(slot.base_effect_egid);
			csv += ',';
			csv += variant.mods_csv;

			sqlite3_reset(stmt);
			sqlite3_clear_bindings(stmt);
			sqlite3_bind_int64(stmt, 1, user_id);
			sqlite3_bind_text (stmt, 2, csv.c_str(),      -1, SQLITE_TRANSIENT);
			sqlite3_bind_text (stmt, 3, allowed.c_str(),  -1, SQLITE_TRANSIENT);
			sqlite3_bind_int  (stmt, 4, slot.base_item_id);
			sqlite3_bind_int  (stmt, 5, vi);  // stock_n = variant index
			if (sqlite3_step(stmt) == SQLITE_DONE && sqlite3_changes(db) > 0) {
				++inserted;
			}
		}
	}
	sqlite3_finalize(stmt);

	// "No Armor" sentinel — one row per slot, stock_n = kVariantCount (5).
	// Empty mod_effect_group_ids → ParseEgidCsv("") = {} → zero buffs when
	// applied. Appears in the armor picker alongside the 5 stat variants so
	// the player can select it to clear a slot. ApplyDefaultArmor handles
	// the empty-CSV row naturally (zero egids, zero ApplyBuff calls).
	int insertedNone = 0;
	sqlite3_stmt* noneStmt = nullptr;
	const char* kNone =
		"INSERT OR IGNORE INTO ga_players_inventory"
		"  (user_id, profile_id, device_id, quality, mod_effect_group_ids, oc, allowed_slots, item_id, stock_n) "
		"VALUES (?, 0, 0, 1165, '', 0, ?, ?, ?)";
	if (sqlite3_prepare_v2(db, kNone, -1, &noneStmt, nullptr) == SQLITE_OK) {
		for (int si = 0; si < ArmorLoadouts::kSlotCount; ++si) {
			const auto& slot = ArmorLoadouts::kSlots[si];
			const std::string allowed = std::to_string(slot.slot_value_id);
			sqlite3_reset(noneStmt);
			sqlite3_clear_bindings(noneStmt);
			sqlite3_bind_int64(noneStmt, 1, user_id);
			sqlite3_bind_text (noneStmt, 2, allowed.c_str(), -1, SQLITE_TRANSIENT);
			sqlite3_bind_int  (noneStmt, 3, slot.base_item_id);
			sqlite3_bind_int  (noneStmt, 4, ArmorLoadouts::kVariantCount);
			if (sqlite3_step(noneStmt) == SQLITE_DONE && sqlite3_changes(db) > 0) ++insertedNone;
		}
		sqlite3_finalize(noneStmt);
	}

	Logger::Log("armor-seed",
		"SeedArmor: user=%lld inserted=%d (of %d possible — rest already present) none=%d\n",
		(long long)user_id, inserted, ArmorLoadouts::kSlotCount * ArmorLoadouts::kVariantCount, insertedNone);
}

void PlayerSessionStore::SeedCharacterArmorDefaults(int64_t character_id) {
	// No lock: same rationale as SeedCharacterCosmeticDefaults.
	sqlite3* db = Database::GetConnection();
	if (!db) return;

	// Resolve user_id from character — needed to find the right inventory rows.
	int64_t user_id = 0;
	{
		sqlite3_stmt* s = nullptr;
		if (sqlite3_prepare_v2(db,
		        "SELECT user_id FROM ga_characters WHERE id = ?",
		        -1, &s, nullptr) == SQLITE_OK) {
			sqlite3_bind_int64(s, 1, character_id);
			if (sqlite3_step(s) == SQLITE_ROW) {
				user_id = sqlite3_column_int64(s, 0);
			}
			sqlite3_finalize(s);
		}
	}
	if (user_id == 0) {
		Logger::Log("armor-seed",
			"SeedCharacterArmorDefaults: no user for char=%lld\n",
			(long long)character_id);
		return;
	}

	// For each armor slot, find the inventory row for the default variant
	// (matched by item_id + stock_n) and INSERT OR IGNORE into
	// ga_character_devices. The UNIQUE(character_id, equipped_slot) index
	// makes this idempotent — players who've moved armor away from the
	// default keep their selection on every login.
	const char* kFindInv =
		"SELECT id FROM ga_players_inventory "
		"WHERE user_id = ? AND item_id = ? AND stock_n = ?";
	// item_profile_id is NOT NULL — armor defaults land in profile 1.
	// Armor::FetchEquippedArmor filters by item_profile_id at runtime, and
	// SaveEquippedDevices's armor pass is profile-scoped, so each profile
	// gets its own armor rows the first time the player saves into it.
	const char* kInsertEquip =
		"INSERT OR IGNORE INTO ga_character_devices (character_id, item_profile_id, inventory_id, equipped_slot) "
		"VALUES (?, 1, ?, ?)";

	sqlite3_stmt* selStmt = nullptr;
	sqlite3_stmt* insStmt = nullptr;
	if (sqlite3_prepare_v2(db, kFindInv,     -1, &selStmt, nullptr) != SQLITE_OK ||
	    sqlite3_prepare_v2(db, kInsertEquip, -1, &insStmt, nullptr) != SQLITE_OK) {
		Logger::Log("armor-seed",
			"SeedCharacterArmorDefaults: prepare failed: %s\n",
			sqlite3_errmsg(db));
		if (selStmt) sqlite3_finalize(selStmt);
		if (insStmt) sqlite3_finalize(insStmt);
		return;
	}

	int equipped = 0;
	const int defaultStockN = ArmorLoadouts::kDefaultVariantIndex;
	for (int si = 0; si < ArmorLoadouts::kSlotCount; ++si) {
		const auto& slot = ArmorLoadouts::kSlots[si];

		sqlite3_reset(selStmt);
		sqlite3_clear_bindings(selStmt);
		sqlite3_bind_int64(selStmt, 1, user_id);
		sqlite3_bind_int  (selStmt, 2, slot.base_item_id);
		sqlite3_bind_int  (selStmt, 3, defaultStockN);
		int invId = 0;
		if (sqlite3_step(selStmt) == SQLITE_ROW) {
			invId = sqlite3_column_int(selStmt, 0);
		}
		if (invId == 0) {
			Logger::Log("armor-seed",
				"SeedCharacterArmorDefaults: no inv row for char=%lld user=%lld slot=%s item=%d variant=%d — SeedArmor must run first\n",
				(long long)character_id, (long long)user_id,
				slot.name, slot.base_item_id, defaultStockN);
			continue;
		}

		sqlite3_reset(insStmt);
		sqlite3_clear_bindings(insStmt);
		sqlite3_bind_int64(insStmt, 1, character_id);
		sqlite3_bind_int  (insStmt, 2, invId);
		sqlite3_bind_int  (insStmt, 3, slot.slot_value_id);
		if (sqlite3_step(insStmt) == SQLITE_DONE && sqlite3_changes(db) > 0) {
			++equipped;
		}
	}
	sqlite3_finalize(selStmt);
	sqlite3_finalize(insStmt);

	Logger::Log("armor-seed",
		"SeedCharacterArmorDefaults: char=%lld newly equipped=%d (of %d slots — rest already had a piece)\n",
		(long long)character_id, equipped, ArmorLoadouts::kSlotCount);
}

void PlayerSessionStore::PinClassDeviceSlot14(int64_t character_id) {
	// No lock: same rationale as SeedCharacterArmorDefaults — callers
	// (InsertCharacter, GSC_SELECT_CHARACTER) may already hold mutex_.
	sqlite3* db = Database::GetConnection();
	if (!db) return;

	// Resolve user_id + class profile_id from character — needed to find the
	// class device inventory row, which is profile-scoped (Medic/Robotics/etc).
	int64_t user_id = 0;
	uint32_t profile_id = 0;
	{
		sqlite3_stmt* s = nullptr;
		if (sqlite3_prepare_v2(db,
		        "SELECT user_id, profile_id FROM ga_characters WHERE id = ?",
		        -1, &s, nullptr) == SQLITE_OK) {
			sqlite3_bind_int64(s, 1, character_id);
			if (sqlite3_step(s) == SQLITE_ROW) {
				user_id    = sqlite3_column_int64(s, 0);
				profile_id = static_cast<uint32_t>(sqlite3_column_int(s, 1));
			}
			sqlite3_finalize(s);
		}
	}
	if (user_id == 0) {
		Logger::Log("db",
			"[PlayerSessionStore] PinClassDeviceSlot14: no user for char=%lld\n",
			(long long)character_id);
		return;
	}

	// Find the class device inventory_id. allowed_slots='502' is unique to
	// the class device — same query as SaveEquippedDevices's pin path.
	int class_inv_id = 0;
	{
		sqlite3_stmt* s = nullptr;
		if (sqlite3_prepare_v2(db,
		        "SELECT id FROM ga_players_inventory "
		        "WHERE user_id = ? AND (profile_id = 0 OR profile_id = ?) "
		        "  AND allowed_slots = '502' "
		        "ORDER BY profile_id DESC "  // prefer profile-specific over shared
		        "LIMIT 1",
		        -1, &s, nullptr) == SQLITE_OK) {
			sqlite3_bind_int64(s, 1, user_id);
			sqlite3_bind_int  (s, 2, (int)profile_id);
			if (sqlite3_step(s) == SQLITE_ROW) {
				class_inv_id = sqlite3_column_int(s, 0);
			}
			sqlite3_finalize(s);
		}
	}
	if (class_inv_id == 0) {
		Logger::Log("db",
			"[PlayerSessionStore] PinClassDeviceSlot14: no class device in inventory for char=%lld user=%lld profile=%u\n",
			(long long)character_id, (long long)user_id, profile_id);
		return;
	}

	// Pin slot 14 across all 5 loadout profiles. INSERT OR IGNORE is keyed on
	// (character_id, item_profile_id, equipped_slot) via
	// idx_ga_character_devices_uniq, so any profile that already has a row
	// (player-modified or previously pinned) is left untouched.
	sqlite3_stmt* ins = nullptr;
	if (sqlite3_prepare_v2(db,
	        "INSERT OR IGNORE INTO ga_character_devices "
	        "(character_id, item_profile_id, inventory_id, equipped_slot) "
	        "VALUES (?, ?, ?, 14)",
	        -1, &ins, nullptr) != SQLITE_OK) {
		Logger::Log("db",
			"[PlayerSessionStore] PinClassDeviceSlot14: prepare failed: %s\n",
			sqlite3_errmsg(db));
		return;
	}
	int pinned = 0;
	for (int item_profile_id = 1; item_profile_id <= 5; ++item_profile_id) {
		sqlite3_reset(ins);
		sqlite3_clear_bindings(ins);
		sqlite3_bind_int64(ins, 1, character_id);
		sqlite3_bind_int  (ins, 2, item_profile_id);
		sqlite3_bind_int  (ins, 3, class_inv_id);
		if (sqlite3_step(ins) == SQLITE_DONE && sqlite3_changes(db) > 0) {
			++pinned;
		}
	}
	sqlite3_finalize(ins);

	Logger::Log("db",
		"[PlayerSessionStore] PinClassDeviceSlot14: char=%lld inv=%d newly-pinned=%d (of 5 profiles — rest already had slot 14)\n",
		(long long)character_id, class_inv_id, pinned);
}

PlayerSessionStore::LevelXp PlayerSessionStore::GetCharacterLevelXp(int64_t character_id) {
	LevelXp out;
	sqlite3* db = Database::GetConnection();
	if (!db) return out;
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db,
	        "SELECT COALESCE((SELECT MAX(l.level) FROM asm_data_set_xp_levels l "
	        "                 WHERE l.xp_value <= c.xp), 1), c.xp "
	        "FROM ga_characters c WHERE c.id = ?",
	        -1, &s, nullptr) != SQLITE_OK) {
		return out;
	}
	sqlite3_bind_int64(s, 1, character_id);
	if (sqlite3_step(s) == SQLITE_ROW) {
		out.level = static_cast<uint32_t>(sqlite3_column_int(s, 0));
		out.xp    = sqlite3_column_int64(s, 1);
	}
	sqlite3_finalize(s);
	if (out.level < 1) out.level = 1;
	return out;
}

std::map<int64_t, uint32_t> PlayerSessionStore::GetCharacterLevels(const std::vector<int64_t>& character_ids) {
	std::map<int64_t, uint32_t> out;
	if (character_ids.empty()) return out;
	sqlite3* db = Database::GetConnection();
	if (!db) return out;
	std::string in;
	for (int64_t id : character_ids) {
		if (id <= 0) continue;
		if (!in.empty()) in += ",";
		in += std::to_string(id);
	}
	if (in.empty()) return out;
	const std::string sql =
		"SELECT c.id, COALESCE((SELECT MAX(l.level) FROM asm_data_set_xp_levels l "
		"                       WHERE l.xp_value <= c.xp), 1) "
		"FROM ga_characters c WHERE c.id IN (" + in + ")";
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db, sql.c_str(), -1, &s, nullptr) != SQLITE_OK) return out;
	while (sqlite3_step(s) == SQLITE_ROW) {
		const int lv = sqlite3_column_int(s, 1);
		out[sqlite3_column_int64(s, 0)] = lv < 1 ? 1u : (uint32_t)lv;
	}
	sqlite3_finalize(s);
	return out;
}

bool PlayerSessionStore::RaiseCharacterToLevel(int64_t character_id, uint32_t level) {
	sqlite3* db = Database::GetConnection();
	if (!db || character_id <= 0 || level <= 1) return false;
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db,
	        "UPDATE ga_characters SET xp = (SELECT MIN(l.xp_value) FROM asm_data_set_xp_levels l "
	        "                               WHERE l.level = ?2) "
	        "WHERE id = ?1 AND COALESCE(xp,0) < (SELECT MIN(l.xp_value) FROM asm_data_set_xp_levels l "
	        "                                    WHERE l.level = ?2)",
	        -1, &s, nullptr) != SQLITE_OK) {
		return false;
	}
	sqlite3_bind_int64(s, 1, character_id);
	sqlite3_bind_int(s, 2, (int)level);
	const bool ok = sqlite3_step(s) == SQLITE_DONE && sqlite3_changes(db) > 0;
	sqlite3_finalize(s);
	return ok;
}

PlayerSessionStore::AgentInfo PlayerSessionStore::GetAgentInfo(int64_t character_id) {
	AgentInfo out;
	sqlite3* db = Database::GetConnection();
	if (!db || character_id <= 0) return out;
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db,
	        "SELECT COALESCE((SELECT MAX(l.level) FROM asm_data_set_xp_levels l "
	        "                 WHERE l.xp_value <= COALESCE(c.xp,0)), 1), "
	        "       COALESCE(c.xp,0), COALESCE(c.credits,0), COALESCE(c.tokens,0), "
	        "       COALESCE((SELECT u.agenda_points FROM ga_users u WHERE u.id = c.user_id), 0) "
	        "FROM ga_characters c WHERE c.id = ?",
	        -1, &s, nullptr) != SQLITE_OK) {
		return out;
	}
	sqlite3_bind_int64(s, 1, character_id);
	if (sqlite3_step(s) == SQLITE_ROW) {
		out.found   = true;
		out.level   = static_cast<uint32_t>(sqlite3_column_int(s, 0));
		out.xp      = sqlite3_column_int64(s, 1);
		out.credits = sqlite3_column_int64(s, 2);
		out.tokens  = sqlite3_column_int64(s, 3);
		out.agenda_points = sqlite3_column_int64(s, 4);
	}
	sqlite3_finalize(s);
	if (out.level < 1) out.level = 1;
	return out;
}

namespace {
// AP rules: no AP of any kind below agent level 10
// except the daily login bonus; reaching level 10 pays 100, then 100 per level.
// Daily login streak (UTC calendar day, per account): day 1 = 25, +5 per
// consecutive day, cap 100; a missed day resets to 25.
constexpr int64_t kApPerLevel = 100, kApMinLevel = 10;
constexpr int64_t kApDailyBase = 25, kApDailyStep = 5, kApDailyCap = 100;
}  // namespace

int64_t PlayerSessionStore::ApForLevel(int64_t level, int64_t ap) {
	return level >= kApMinLevel ? ap : 0;
}

static std::mutex g_levelup_mu;
static std::map<int64_t, uint32_t> g_levelup_pending;

void PlayerSessionStore::NoteLevelUp(int64_t character_id, uint32_t level) {
	std::lock_guard<std::mutex> lk(g_levelup_mu);
	uint32_t& v = g_levelup_pending[character_id];
	if (level > v) v = level;
	ServiceHooks::Notify("level_up", {{"char_id", character_id}, {"new_level", level}});
}

uint32_t PlayerSessionStore::TakeLevelUp(int64_t character_id) {
	std::lock_guard<std::mutex> lk(g_levelup_mu);
	auto it = g_levelup_pending.find(character_id);
	if (it == g_levelup_pending.end()) return 0;
	const uint32_t v = it->second;
	g_levelup_pending.erase(it);
	return v;
}

int64_t PlayerSessionStore::SyncAgendaPoints(int64_t character_id, int64_t* daily_paid,
                                             int64_t* streak_out) {
	if (daily_paid) *daily_paid = 0;
	sqlite3* db = Database::GetConnection();
	if (!db || character_id <= 0) return 0;
	int64_t user_id = 0, awarded = 0, daily_day = 0, ap = 0, streak = 0;
	{
		sqlite3_stmt* s = nullptr;
		if (sqlite3_prepare_v2(db,
		        "SELECT c.user_id, c.ap_level_awarded, u.ap_daily_day, u.agenda_points, u.ap_daily_streak "
		        "FROM ga_characters c JOIN ga_users u ON u.id = c.user_id "
		        "WHERE c.id = ? AND c.deleted_at = 0",
		        -1, &s, nullptr) != SQLITE_OK) return 0;
		sqlite3_bind_int64(s, 1, character_id);
		if (sqlite3_step(s) == SQLITE_ROW) {
			user_id   = sqlite3_column_int64(s, 0);
			awarded   = sqlite3_column_int64(s, 1);
			daily_day = sqlite3_column_int64(s, 2);
			ap        = sqlite3_column_int64(s, 3);
			streak    = sqlite3_column_int64(s, 4);
		}
		sqlite3_finalize(s);
	}
	if (user_id == 0) return 0;

	const int64_t level = GetAgentInfo(character_id).level;
	const int64_t today = (int64_t)std::time(nullptr) / 86400;
	// Levels 1-9 pay nothing; level 10 is the first payout. Already-paid levels
	// (ap_level_awarded) are never paid again and nothing is clawed back.
	const int64_t paid_from = std::max<int64_t>(awarded, kApMinLevel - 1);
	const int64_t level_ap = level > paid_from ? (level - paid_from) * kApPerLevel : 0;
	const int64_t new_streak = daily_day == today - 1 ? std::max<int64_t>(streak, 1) + 1 : 1;  // pre-streak rows count as day 1
	const int64_t daily_ap = daily_day < today
		? std::min<int64_t>(kApDailyCap, kApDailyBase + (new_streak - 1) * kApDailyStep) : 0;
	if (level_ap == 0 && daily_ap == 0) return ap;

	// Guarded UPDATEs (old value in the WHERE) so a concurrent sync never pays twice.
	int64_t granted = 0;
	sqlite3_stmt* u = nullptr;
	if (level_ap > 0 && sqlite3_prepare_v2(db,
	        "UPDATE ga_characters SET ap_level_awarded = ?1 WHERE id = ?2 AND ap_level_awarded = ?3",
	        -1, &u, nullptr) == SQLITE_OK) {
		sqlite3_bind_int64(u, 1, level);
		sqlite3_bind_int64(u, 2, character_id);
		sqlite3_bind_int64(u, 3, awarded);
		if (sqlite3_step(u) == SQLITE_DONE && sqlite3_changes(db) == 1) granted += level_ap;
		sqlite3_finalize(u);
	}
	if (daily_ap > 0 && sqlite3_prepare_v2(db,
	        "UPDATE ga_users SET ap_daily_day = ?1, ap_daily_streak = ?4 WHERE id = ?2 AND ap_daily_day = ?3",
	        -1, &u, nullptr) == SQLITE_OK) {
		sqlite3_bind_int64(u, 1, today);
		sqlite3_bind_int64(u, 2, user_id);
		sqlite3_bind_int64(u, 3, daily_day);
		sqlite3_bind_int64(u, 4, new_streak);
		if (sqlite3_step(u) == SQLITE_DONE && sqlite3_changes(db) == 1) {
			granted += daily_ap;
			if (daily_paid) *daily_paid = daily_ap;
			if (streak_out) *streak_out = new_streak;
		}
		sqlite3_finalize(u);
	}
	if (granted > 0 && sqlite3_prepare_v2(db,
	        "UPDATE ga_users SET agenda_points = agenda_points + ?1 WHERE id = ?2",
	        -1, &u, nullptr) == SQLITE_OK) {
		sqlite3_bind_int64(u, 1, granted);
		sqlite3_bind_int64(u, 2, user_id);
		sqlite3_step(u);
		sqlite3_finalize(u);
		ap += granted;
		Logger::Log("rewards", "[AP] char=%lld user=%lld level %lld (paid to %lld) daily=%lld streak=%lld -> +%lld AP, wallet %lld\n",
			(long long)character_id, (long long)user_id, (long long)level, (long long)awarded,
			(long long)daily_ap, (long long)(daily_ap > 0 ? new_streak : streak), (long long)granted, (long long)ap);
	}
	return ap;
}

bool PlayerSessionStore::ValidateSkillAllocation(int64_t character_id,
                                                 const std::vector<SkillRow>& skills,
                                                 std::string& why) {
	std::vector<SkillRules::SkillPick> picks;
	for (const auto& r : skills) picks.push_back({r.skill_group_id, r.skill_id, r.points});
	return SkillRules::ValidateSkillAllocation(Database::GetConnection(), character_id, picks, why);
}

namespace {
// One asm_data_set_event_rewards row: {xp, tokens, credits} for a reward
// event (+ difficulty). Missing row = neutral (1.0 multipliers / 0 base).
struct RewardTriple { double xp, tokens, credits; bool found; };
RewardTriple LookupEventReward(sqlite3* db, int reward_event, int difficulty, double dflt) {
	RewardTriple r{dflt, dflt, dflt, false};
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db,
	        "SELECT xp_reward_value, token_reward_value, currency_reward_value "
	        "FROM asm_data_set_event_rewards WHERE reward_event_value_id = ? "
	        "AND difficulty_value_id = ? ORDER BY process_order LIMIT 1",
	        -1, &s, nullptr) != SQLITE_OK) return r;
	sqlite3_bind_int(s, 1, reward_event);
	sqlite3_bind_int(s, 2, difficulty);
	if (sqlite3_step(s) == SQLITE_ROW) {
		r = {sqlite3_column_double(s, 0), sqlite3_column_double(s, 1),
		     sqlite3_column_double(s, 2), true};
	}
	sqlite3_finalize(s);
	return r;
}
}  // namespace

// Map + start time of an instance, for reward hook payloads.
static std::pair<std::string, int64_t> InstanceStartInfo(sqlite3* db, int64_t instance_id) {
	std::pair<std::string, int64_t> r{std::string(), 0};
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db, "SELECT map_name, started_at FROM ga_instances WHERE instance_id = ?",
	        -1, &s, nullptr) != SQLITE_OK) return r;
	sqlite3_bind_int64(s, 1, instance_id);
	if (sqlite3_step(s) == SQLITE_ROW) {
		const unsigned char* m = sqlite3_column_text(s, 0);
		r.first = m ? (const char*)m : "";
		r.second = sqlite3_column_int64(s, 1);
	}
	sqlite3_finalize(s);
	return r;
}

// Optional "notes" in a mission_reward / pvp_reward hook reply =
// the bonus sources behind its numbers, listed on the results screen:
// [{"label": "...", "xp": N, "credits": N, "tokens": N}] (amounts, already in
// the reply) or [{"label": "...", "xp_percent": P, ...}] (percent of the given
// base). No notes = the reply is plain tuning, nothing listed.
static std::vector<PlayerSessionStore::PveRewardResult::Note> HookNotes(
		const std::optional<nlohmann::json>& d, int64_t bxp = 0, int64_t bcr = 0, int64_t btk = 0) {
	std::vector<PlayerSessionStore::PveRewardResult::Note> out;
	if (!d || !d->is_object() || !d->contains("notes") || !(*d)["notes"].is_array()) return out;
	for (const auto& n : (*d)["notes"]) {
		if (!n.is_object() || !n.contains("label") || !n["label"].is_string()) continue;
		PlayerSessionStore::PveRewardResult::Note note;
		note.label = n["label"].get<std::string>().substr(0, 48);
		const auto pct = [&](const char* k, int64_t base) {
			const int64_t p = ServiceHooks::ClampInt(n, k, 100, 0, 300);
			return base * (p - 100) / 100;
		};
		note.xp      = ServiceHooks::ClampInt(n, "xp", 0, -100000, 100000) + pct("xp_percent", bxp);
		note.credits = ServiceHooks::ClampInt(n, "credits", 0, -100000, 100000) + pct("credits_percent", bcr);
		note.tokens  = ServiceHooks::ClampInt(n, "tokens", 0, -100000, 100000) + pct("tokens_percent", btk);
		out.push_back(std::move(note));
		if (out.size() >= 6) break;
	}
	return out;
}

PlayerSessionStore::PveRewardResult PlayerSessionStore::AwardPveMissionRewards(
		int64_t instance_id, uint32_t difficulty_value_id, bool won,
		uint32_t reward_level_max, const std::string& queue_name, int players_tf) {
	PveRewardResult result;
	sqlite3* db = Database::GetConnection();
	if (!db || instance_id == 0 || difficulty_value_id == 0) return result;

	// Reward event value ids (asm valid_values group 107).
	constexpr int kPveMatchComplete = 1296, kPveDifficulty = 1195, kPveWin = 1303;
	constexpr int kPve1Player = 1459, kPve2Player = 1460, kPve3Player = 1461;
	constexpr int kPveChallengeBonus = 1261;

	std::vector<int64_t> chars;
	{
		sqlite3_stmt* s = nullptr;
		if (sqlite3_prepare_v2(db,
		        "SELECT DISTINCT character_id FROM ga_instance_players "
		        "WHERE instance_id = ? AND left_at IS NULL AND character_id > 0",
		        -1, &s, nullptr) != SQLITE_OK) return result;
		sqlite3_bind_int64(s, 1, instance_id);
		while (sqlite3_step(s) == SQLITE_ROW) chars.push_back(sqlite3_column_int64(s, 0));
		sqlite3_finalize(s);
	}
	// Queue reward cap (GA 1.35: e.g. Medium "rewards stop past 19").
	// Party size still counts them; they just get no mission-end rewards.
	const int n = (int)chars.size();
	if (reward_level_max < 200) {
		std::vector<int64_t> keep;
		for (int64_t cid : chars) {
			const uint32_t lvl = GetCharacterLevelXp(cid).level;
			if (lvl > reward_level_max) {
				result.over_level.push_back(cid);
				Logger::Log("rewards", "[Rewards] instance=%lld char=%lld level %u > cap %u -> no mission rewards\n",
					(long long)instance_id, (long long)cid, lvl, reward_level_max);
			} else keep.push_back(cid);
		}
		chars.swap(keep);
	}
	if (chars.empty()) return result;

	const RewardTriple base = LookupEventReward(db, kPveMatchComplete, 0, 0.0);
	const RewardTriple diff = LookupEventReward(db, kPveDifficulty, (int)difficulty_value_id, 1.0);
	const RewardTriple party = LookupEventReward(db,
		n <= 1 ? kPve1Player : (n == 2 ? kPve2Player : kPve3Player), 0, 1.0);
	const RewardTriple win = won
		? LookupEventReward(db, kPveWin, (int)difficulty_value_id, 1.0)
		: RewardTriple{1.0, 1.0, 1.0, false};

	int64_t xp      = (int64_t)(base.xp      * diff.xp      * party.xp      * win.xp      + 0.5);
	int64_t tokens  = (int64_t)(base.tokens  * diff.tokens  * party.tokens  * win.tokens  + 0.5);
	int64_t credits = (int64_t)(base.credits * diff.credits * party.credits * win.credits + 0.5);

	// Optional external adjustment of the mission-end amounts (same
	// for every agent in the instance). Without a subscriber: the data values.
	std::vector<PveRewardResult::Note> hook_notes;
	if (ServiceHooks::Enabled("mission_reward")) {
		const auto start = InstanceStartInfo(db, instance_id);
		auto d = ServiceHooks::Call("mission_reward", {
			{"instance_id", instance_id}, {"queue", queue_name},
			{"map_name", start.first}, {"started_at", start.second},
			{"difficulty_value_id", difficulty_value_id}, {"won", won},
			{"party_size", n}, {"xp", xp}, {"credits", credits}, {"tokens", tokens}});
		if (d) {
			xp      = ServiceHooks::ClampInt(*d, "xp",      xp,      0, 100000);
			credits = ServiceHooks::ClampInt(*d, "credits", credits, 0, 100000);
			tokens  = ServiceHooks::ClampInt(*d, "tokens",  tokens,  0, 100000);
			hook_notes = HookNotes(d);
		}
	}

	// PvE Challenge Bonus (1261, flat xp/credits) on a win with the
	// players' team deaths below the instance's victory_bonus_lives - the HUD
	// "PvE Challenge Bonus deaths / limit" counter (msg 34351/34352). Same rule
	// as the "Win with Victory Bonus" achievement metric.
	if (won) {
		sqlite3_stmt* s = nullptr;
		if (sqlite3_prepare_v2(db, "SELECT victory_bonus_lives, count_deaths_attackers, "
		        "count_deaths_defenders FROM ga_instances WHERE instance_id = ?",
		        -1, &s, nullptr) == SQLITE_OK) {
			sqlite3_bind_int64(s, 1, instance_id);
			if (sqlite3_step(s) == SQLITE_ROW) {
				const int lives = sqlite3_column_int(s, 0);
				const int deaths = sqlite3_column_int(s, players_tf == 2 ? 2 : 1);
				result.challenge_bonus = lives > 0 && deaths < lives;
			}
			sqlite3_finalize(s);
		}
		if (result.challenge_bonus) {
			const RewardTriple cb = LookupEventReward(db, kPveChallengeBonus, 0, 0.0);
			xp      += (int64_t)(cb.xp + 0.5);
			credits += (int64_t)(cb.credits + 0.5);
			tokens  += (int64_t)(cb.tokens + 0.5);
		}
	}

	sqlite3_stmt* u = nullptr;
	if (sqlite3_prepare_v2(db,
	        "UPDATE ga_characters SET "
	        "  xp = xp + ?1, "  /* cap + lv50 credits = trigger trg_lv50_xp_credits */
	        "  tokens = tokens + ?2, credits = credits + ?3 "
	        "WHERE id = ?4 AND deleted_at = 0",
	        -1, &u, nullptr) != SQLITE_OK) {
		Logger::Log("db", "[Rewards] prepare failed: %s\n", sqlite3_errmsg(db));
		return result;
	}
	// Agenda Points for a won mission, 25 (normal) .. 50 (hardest
	// difficulty multiplier 3.0), into the account wallet (level-up AP follows
	// via SyncAgendaPoints on the next Stats push).
	const int64_t ap = won ? std::min<int64_t>(50, std::max<int64_t>(25,
		(int64_t)(25.0 + (diff.xp - 1.0) * 12.5 + 0.5))) : 0;
	// An active account booster (ga_users.booster_until, bought
	// with AP - item type 1609 "Doubles all end-mission rewards") doubles this
	// agent's xp / credits / tokens. AP itself is not doubled.
	sqlite3_stmt* bq = nullptr;
	sqlite3_prepare_v2(db,
		"SELECT 1 FROM ga_users WHERE booster_until > strftime('%s','now') "
		"AND id = (SELECT user_id FROM ga_characters WHERE id = ?1)",
		-1, &bq, nullptr);
	for (int64_t cid : chars) {
		int boost = 1;
		if (bq) {
			sqlite3_bind_int64(bq, 1, cid);
			if (sqlite3_step(bq) == SQLITE_ROW) boost = 2;
			sqlite3_reset(bq);
		}
		const uint32_t before = GetCharacterLevelXp(cid).level;
		// AP: ApRewards ("ap_rewards" config, optional ap_grant service).
		const int64_t ap_c = ApRewards::Grant(cid, before, "pve_mission",
			ApForLevel(before, ap), true, true);
		PveRewardResult::Paid& paid = result.paid[cid];
		paid.xp = xp * boost;
		paid.tokens = LimitTokenGain(cid, tokens * boost);
		paid.credits = credits * boost;
		paid.agenda_points = ap_c;
		paid.notes = hook_notes;
		if (boost > 1)
			paid.notes.push_back({"Booster x2", xp * (boost - 1), credits * (boost - 1),
			                      std::max<int64_t>(0, paid.tokens - tokens)});
		sqlite3_bind_int64(u, 1, paid.xp);
		sqlite3_bind_int64(u, 2, paid.tokens);
		sqlite3_bind_int64(u, 3, paid.credits);
		sqlite3_bind_int64(u, 4, cid);
		sqlite3_step(u);
		sqlite3_reset(u);
		AgencyBank::TaxEarnings(cid, paid.credits);   // agency tax
		const LevelXp after = GetCharacterLevelXp(cid);
		if (after.level > before) NoteLevelUp(cid, after.level);
		Logger::Log("rewards",
			"[Rewards] instance=%lld char=%lld difficulty=%u won=%d players=%d "
			"-> +%lld xp +%lld credits +%lld tokens +%lld AP%s%s (level %u -> %u, xp now %lld)\n",
			(long long)instance_id, (long long)cid, difficulty_value_id, won ? 1 : 0, n,
			(long long)paid.xp, (long long)paid.credits, (long long)paid.tokens, (long long)ap_c,
			result.challenge_bonus ? " [challenge bonus]" : "",
			boost > 1 ? " [booster x2]" : "",
			before, after.level, (long long)after.xp);
	}
	sqlite3_finalize(u);
	if (bq) sqlite3_finalize(bq);
	result.agenda_points = ap;
	result.xp = xp;
	result.credits = credits;
	result.tokens = tokens;
	result.characters = chars;
	return result;
}

std::vector<PlayerSessionStore::PveRewardResult> PlayerSessionStore::AwardPvpMatchRewards(
		int64_t instance_id, int winning_task_force, int team_size) {
	std::vector<PveRewardResult> result;
	sqlite3* db = Database::GetConnection();
	if (!db || instance_id == 0) return result;
	const ApRewards::Settings& cfg = ApRewards::Get();

	// Agents still in the match at the end (leaving early = 0) and their final side.
	// window = seconds since the agent first joined this match (backfills join late).
	struct Agent { int tf = 0; int64_t rep = 0; int64_t activity = 0; double seconds = 0; double window = 0; };
	std::map<int64_t, Agent> agents;
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db,
	        "SELECT p.character_id, MAX(p.task_force_number), strftime('%s','now') - "
	        " (SELECT MIN(f.joined_at) FROM ga_instance_players f "
	        "  WHERE f.instance_id = p.instance_id AND f.character_id = p.character_id) "
	        "FROM ga_instance_players p "
	        "WHERE p.instance_id = ? AND p.left_at IS NULL AND p.character_id > 0 "
	        "GROUP BY p.character_id", -1, &s, nullptr) != SQLITE_OK) return result;
	sqlite3_bind_int64(s, 1, instance_id);
	while (sqlite3_step(s) == SQLITE_ROW) {
		Agent& a = agents[sqlite3_column_int64(s, 0)];
		a.tf = sqlite3_column_int(s, 1);
		a.window = sqlite3_column_double(s, 2);
	}
	sqlite3_finalize(s);
	if (agents.empty()) return result;

	// The game's own end-of-match scoreboard (MatchStats stints, all sides summed;
	// rank uses the rep points earned on the final side).
	double longest = 0;
	if (sqlite3_prepare_v2(db,
	        "SELECT character_id, task_force, rep_points, rep_points + kills + damage_dealt + healing"
	        " + obj_points + objective_captures, time_played_seconds "
	        "FROM ga_match_player_stats WHERE instance_id = ?", -1, &s, nullptr) != SQLITE_OK)
		return result;
	sqlite3_bind_int64(s, 1, instance_id);
	while (sqlite3_step(s) == SQLITE_ROW) {
		const double secs = sqlite3_column_double(s, 4);
		longest = std::max(longest, secs);
		auto it = agents.find(sqlite3_column_int64(s, 0));
		if (it == agents.end()) continue;
		if (sqlite3_column_int(s, 1) == it->second.tf) it->second.rep += sqlite3_column_int64(s, 2);
		it->second.activity += sqlite3_column_int64(s, 3);
		it->second.seconds  += secs;
	}
	sqlite3_finalize(s);

	// p0997c: real players are paid in bot matches, solo + bots included. The per-agent presence and
	// activity checks below are the rule; pvp_min_real_per_side > 0 turns the old side minimum back on.
	const int min_real = cfg.pvp_min_real_per_side <= 0 ? 0
		: std::max(1, std::min(cfg.pvp_min_real_per_side, std::max(1, team_size)));
	int side_count[3] = {0, 0, 0};
	for (const auto& [cid, a] : agents) if (a.tf == 1 || a.tf == 2) side_count[a.tf]++;
	if (side_count[1] < min_real || side_count[2] < min_real) {
		Logger::Log("ap", "[AP] pvp instance=%lld no AP: sides %d v %d < %d real\n",
			(long long)instance_id, side_count[1], side_count[2], min_real);
		return result;
	}

	// Rank on own team by rep points (ties: lower character id first, stable).
	std::vector<std::pair<int64_t, int64_t>> order;  // (cid, rep)
	for (const auto& [cid, a] : agents) order.emplace_back(cid, a.rep);
	std::stable_sort(order.begin(), order.end(),
		[](const auto& x, const auto& y) { return x.second > y.second; });
	std::map<int64_t, int> rank;
	int next[3] = {1, 1, 1};
	for (const auto& [cid, rep] : order) {
		const int tf = agents[cid].tf;
		if ((tf == 1 || tf == 2) && rep > 0) rank[cid] = next[tf]++;
	}

	// Reward event value ids (asm valid_values group 107).
	constexpr int kPvpMatchComplete = 1180, kPvpWin = 902, kPvpRanking = 880;
	const RewardTriple done = LookupEventReward(db, kPvpMatchComplete, 0, 0.0);
	const RewardTriple win  = LookupEventReward(db, kPvpWin, 0, 1.0);
	const RewardTriple rank_bonus = LookupEventReward(db, kPvpRanking, 0, 0.0);
	// Seats without an agent (character_id <= 0) outnumbering agents = half rewards.
	int64_t other_seats = 0;
	if (sqlite3_prepare_v2(db,
	        "SELECT COUNT(DISTINCT session_guid) FROM ga_instance_players "
	        "WHERE instance_id = ? AND left_at IS NULL AND character_id <= 0",
	        -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int64(s, 1, instance_id);
		if (sqlite3_step(s) == SQLITE_ROW) other_seats = sqlite3_column_int64(s, 0);
		sqlite3_finalize(s);
	}
	const double scale = other_seats > (int64_t)agents.size() ? 0.5 : 1.0;
	// Optional external token percent for the whole match.
	// + optional xp/credits percent. Without a subscriber: 100.
	int64_t tokens_percent = 100, xp_percent = 100, credits_percent = 100;
	int64_t tokens_flat[2] = {0, 0};  // optional flat PvP tokens {lost, won} from the hook (data has 0)
	std::optional<nlohmann::json> hook_reply;
	if (ServiceHooks::Enabled("pvp_reward")) {
		const auto start = InstanceStartInfo(db, instance_id);
		auto d = ServiceHooks::Call("pvp_reward", {
			{"instance_id", instance_id}, {"map_name", start.first}, {"started_at", start.second},
			{"winning_task_force", winning_task_force}, {"team_size", team_size}, {"mode", "match"}});
		hook_reply = d;
		if (d) {
			tokens_percent  = ServiceHooks::ClampInt(*d, "tokens_percent", 100, 0, 300);
			xp_percent      = ServiceHooks::ClampInt(*d, "xp_percent", 100, 0, 300);
			credits_percent = ServiceHooks::ClampInt(*d, "credits_percent", 100, 0, 300);
			tokens_flat[0] = ServiceHooks::ClampInt(*d, "tokens_flat_loss", 0, 0, 1000); tokens_flat[1] = ServiceHooks::ClampInt(*d, "tokens_flat_win", 0, 0, 1000);
		}
	}
	sqlite3_stmt* u = nullptr;
	sqlite3_prepare_v2(db,
		"UPDATE ga_characters SET "
		"  xp = xp + ?1, "  /* cap + lv50 credits = trigger trg_lv50_xp_credits */
		"  tokens = tokens + ?2, credits = credits + ?3 WHERE id = ?4 AND deleted_at = 0",
		-1, &u, nullptr);
	sqlite3_stmt* bq = nullptr;
	sqlite3_prepare_v2(db,
		"SELECT 1 FROM ga_users WHERE booster_until > strftime('%s','now') "
		"AND id = (SELECT user_id FROM ga_characters WHERE id = ?1)",
		-1, &bq, nullptr);

	// GA 1.37: backfilled players get the full match reward, so presence is
	// measured against the time since the agent first joined, not the whole match.
	for (const auto& [cid, a] : agents) {
		const double span = a.window > 0 ? std::min(longest, a.window) : longest;
		const double need = span * cfg.pvp_min_presence_percent / 100.0;
		if (a.activity <= 0 || a.seconds < need) {
			Logger::Log("ap", "[AP] pvp char=%lld no rewards: activity=%lld present=%.0fs/%.0fs\n",
				(long long)cid, (long long)a.activity, a.seconds, span);
			continue;
		}
		const bool won = winning_task_force != 0 && a.tf == winning_task_force;
		const bool top = rank.count(cid) && rank[cid] == 1;
		int boost = 1;
		if (bq) {
			sqlite3_bind_int64(bq, 1, cid);
			if (sqlite3_step(bq) == SQLITE_ROW) boost = 2;
			sqlite3_reset(bq);
		}
		const auto pay = [&](double b, double w, double r) {
			return (int64_t)((b * (won ? w : 1.0) + (top ? r : 0.0)) * scale + 0.5) * boost;
		};
		PveRewardResult rw;
		const int64_t bxp = pay(done.xp, win.xp, rank_bonus.xp);
		const int64_t btk = pay(done.tokens, win.tokens, rank_bonus.tokens) + (int64_t)(tokens_flat[won ? 1 : 0] * scale + 0.5) * boost;
		const int64_t bcr = pay(done.credits, win.credits, rank_bonus.credits);
		rw.xp      = bxp * xp_percent / 100;
		rw.tokens  = LimitTokenGain(cid, btk * tokens_percent / 100);
		rw.credits = bcr * credits_percent / 100;
		{
			auto& paid = rw.paid[cid];
			paid.xp = rw.xp; paid.credits = rw.credits; paid.tokens = rw.tokens;
			// bonus lines: booster on the data amounts, hook percents on the boosted ones
			if (boost > 1)
				paid.notes.push_back({"Booster x2", bxp / boost * (boost - 1), bcr / boost * (boost - 1),
				                      btk / boost * (boost - 1)});
			for (auto& n : HookNotes(hook_reply, bxp, bcr, btk)) paid.notes.push_back(std::move(n));
		}
		const uint32_t before = GetCharacterLevelXp(cid).level;
		if (u) {
			sqlite3_bind_int64(u, 1, rw.xp);
			sqlite3_bind_int64(u, 2, rw.tokens);
			sqlite3_bind_int64(u, 3, rw.credits);
			sqlite3_bind_int64(u, 4, cid);
			sqlite3_step(u);
			sqlite3_reset(u);
		}
		AgencyBank::TaxEarnings(cid, rw.credits);   // agency tax
		const LevelXp after = GetCharacterLevelXp(cid);
		if (after.level > before) NoteLevelUp(cid, after.level);
		int64_t base = cfg.pvp_finish;
		if (winning_task_force != 0 && a.tf == winning_task_force) base += cfg.pvp_win;
		auto r = rank.find(cid);
		if (r != rank.end())
			base += r->second == 1 ? cfg.pvp_rank1 : r->second == 2 ? cfg.pvp_rank2
			      : r->second == 3 ? cfg.pvp_rank3 : 0;
		const int64_t level = before;
		rw.agenda_points = ApRewards::Grant(cid, level, "pvp_match", ApForLevel(level, base), true, true);
		rw.paid[cid].agenda_points = rw.agenda_points;
		Logger::Log("rewards",
			"[PvP] instance=%lld char=%lld tf=%d winner_tf=%d rank1=%d scale=%.1f -> +%lld xp +%lld credits "
			"+%lld tokens +%lld AP%s (level %u -> %u)\n",
			(long long)instance_id, (long long)cid, a.tf, winning_task_force, top ? 1 : 0, scale,
			(long long)rw.xp, (long long)rw.credits, (long long)rw.tokens, (long long)rw.agenda_points,
			boost > 1 ? " [booster x2]" : "", before, after.level);
		rw.characters.push_back(cid);
		result.push_back(std::move(rw));
	}
	if (u) sqlite3_finalize(u);
	if (bq) sqlite3_finalize(bq);
	return result;
}

// F2P rework (d): raid / endgame clear AP on top of the mission AP.
// raid_min (Medium) .. raid_max (Ultra-Max) by difficulty; unknown special
// difficulties (super agent, pve attack) pay the High step. Once per agent per instance.
std::vector<std::pair<int64_t, int64_t>> PlayerSessionStore::GrantRaidClearAp(
		int64_t instance_id, int players_tf, uint32_t difficulty_value_id,
		bool raid_queue, const std::string& map_name) {
	std::vector<std::pair<int64_t, int64_t>> out;
	sqlite3* db = Database::GetConnection();
	if (!db || instance_id == 0) return out;
	const ApRewards::Settings& cfg = ApRewards::Get();
	constexpr uint32_t kMedium = 1029, kHigh = 1030, kMax = 1259, kUltraMax = 1471;
	// Solo tiers Adept / Advanced / Expert pay like Medium / High / Max.
	if (difficulty_value_id == 1468) difficulty_value_id = kMedium;
	else if (difficulty_value_id == 1469) difficulty_value_id = kHigh; else if (difficulty_value_id == 1470) difficulty_value_id = kMax;
	const int64_t step = (cfg.raid_max - cfg.raid_min) / 3;
	int64_t base = cfg.raid_min + step;  // High / special
	if (difficulty_value_id == kMedium) base = cfg.raid_min;
	else if (difficulty_value_id == kMax) base = cfg.raid_min + 2 * step;
	else if (difficulty_value_id == kUltraMax) base = cfg.raid_max;
	std::string lower = map_name;
	for (char& c : lower) c = (char)std::tolower((unsigned char)c);
	// Level-50 Dweller missions on High+ count as endgame clears too.
	const bool dweller = lower.find("dweller") != std::string::npos &&
		(difficulty_value_id == kHigh || difficulty_value_id == kMax || difficulty_value_id == kUltraMax);
	if (!raid_queue && !dweller) return out;

	std::vector<int64_t> chars;
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db,
	        "SELECT DISTINCT character_id FROM ga_instance_players WHERE instance_id = ? "
	        "AND left_at IS NULL AND character_id > 0 AND task_force_number = ?",
	        -1, &s, nullptr) != SQLITE_OK) return out;
	sqlite3_bind_int64(s, 1, instance_id);
	sqlite3_bind_int(s, 2, players_tf);
	while (sqlite3_step(s) == SQLITE_ROW) chars.push_back(sqlite3_column_int64(s, 0));
	sqlite3_finalize(s);
	for (int64_t cid : chars) {
		const int64_t level = GetCharacterLevelXp(cid).level;
		if (!raid_queue && level < 50) continue;
		if (sqlite3_prepare_v2(db,
		        "INSERT OR IGNORE INTO ga_ap_raid_clears (instance_id, character_id) VALUES (?, ?)",
		        -1, &s, nullptr) != SQLITE_OK) continue;
		sqlite3_bind_int64(s, 1, instance_id);
		sqlite3_bind_int64(s, 2, cid);
		sqlite3_step(s);
		sqlite3_finalize(s);
		if (sqlite3_changes(db) == 0) continue;  // already paid for this instance
		const int64_t ap = ApRewards::Grant(cid, level, "raid_clear", ApForLevel(level, base), true, true);
		if (ap > 0) out.emplace_back(cid, ap);
	}
	return out;
}

std::vector<PlayerSessionStore::PveRewardResult> PlayerSessionStore::ResolveAvaMatch(
		int64_t instance_id, int winning_task_force) {
	std::vector<PveRewardResult> result;
	sqlite3* db = Database::GetConnection();
	if (!db || instance_id == 0) return result;

	constexpr int kPvpMatchComplete = 1180, kPvpWin = 902;
	constexpr int64_t kSoloOwnerBase = 0x40000000;

	struct Seat { int64_t cid; int tf; };
	std::vector<Seat> seats;
	{
		sqlite3_stmt* s = nullptr;
		if (sqlite3_prepare_v2(db,
		        "SELECT character_id, MAX(task_force_number) FROM ga_instance_players "
		        "WHERE instance_id = ? AND left_at IS NULL AND character_id > 0 "
		        "GROUP BY character_id",
		        -1, &s, nullptr) != SQLITE_OK) return result;
		sqlite3_bind_int64(s, 1, instance_id);
		while (sqlite3_step(s) == SQLITE_ROW)
			seats.push_back({sqlite3_column_int64(s, 0), sqlite3_column_int(s, 1)});
		sqlite3_finalize(s);
	}
	if (seats.empty()) return result;

	AvaParity::OnAvaResult(instance_id, winning_task_force);  // lockdown + HQ cargo
	struct PushOnExit { ~PushOnExit() { TcpSession::AvaPushZoneToViewers(0); } } push_on_exit;  // open maps show the new owner
	int64_t ava_flat[2] = {0, 0};  // flat AvA tokens {lost, won} from the pvp_reward hook (by facility tier)
	const RewardTriple base = LookupEventReward(db, kPvpMatchComplete, 0, 0.0);
	const RewardTriple win  = LookupEventReward(db, kPvpWin, 0, 1.0);
	const int64_t now = (int64_t)std::time(nullptr);
	// Same optional pvp_reward percents as arena matches
	// (mode "ava"). Without a subscriber: 100.
	int64_t tokens_percent = 100, xp_percent = 100, credits_percent = 100;
	std::optional<nlohmann::json> hook_reply;
	if (ServiceHooks::Enabled("pvp_reward")) {
		const auto start = InstanceStartInfo(db, instance_id);
		auto d = ServiceHooks::Call("pvp_reward", {
			{"instance_id", instance_id}, {"map_name", start.first}, {"started_at", start.second},
			{"winning_task_force", winning_task_force}, {"team_size", (int64_t)seats.size()}, {"mode", "ava"},
			{"ava_tech", AvaParity::TechTier(instance_id)}});
		hook_reply = d;
		if (d) {
			tokens_percent  = ServiceHooks::ClampInt(*d, "tokens_percent", 100, 0, 300);
			xp_percent      = ServiceHooks::ClampInt(*d, "xp_percent", 100, 0, 300);
			credits_percent = ServiceHooks::ClampInt(*d, "credits_percent", 100, 0, 300);
			ava_flat[0] = ServiceHooks::ClampInt(*d, "tokens_flat_loss", 0, 0, 2000); ava_flat[1] = ServiceHooks::ClampInt(*d, "tokens_flat_win", 0, 0, 2000);
		}
	}
	// Attackers who joined at all (a lone defender's solo pop pays no defend AP).
	int64_t real_attackers = 0;
	{
		sqlite3_stmt* s = nullptr;
		if (sqlite3_prepare_v2(db,
		        "SELECT COUNT(DISTINCT character_id) FROM ga_instance_players "
		        "WHERE instance_id = ? AND character_id > 0 AND task_force_number = 1",
		        -1, &s, nullptr) == SQLITE_OK) {
			sqlite3_bind_int64(s, 1, instance_id);
			if (sqlite3_step(s) == SQLITE_ROW) real_attackers = sqlite3_column_int64(s, 0);
			sqlite3_finalize(s);
		}
	}

	for (const Seat& seat : seats) {
		const bool won = winning_task_force != 0 && seat.tf == winning_task_force;
		int boost = 1;
		sqlite3_stmt* s = nullptr;
		if (sqlite3_prepare_v2(db,
		        "SELECT 1 FROM ga_users WHERE booster_until > strftime('%s','now') "
		        "AND id = (SELECT user_id FROM ga_characters WHERE id = ?1)",
		        -1, &s, nullptr) == SQLITE_OK) {
			sqlite3_bind_int64(s, 1, seat.cid);
			if (sqlite3_step(s) == SQLITE_ROW) boost = 2;
			sqlite3_finalize(s);
		}
		const int64_t nxp = (int64_t)(base.xp      * (won ? win.xp      : 1.0) + 0.5);
		const int64_t ntk = (int64_t)(base.tokens  * (won ? win.tokens  : 1.0) + 0.5);
		const int64_t ncr = (int64_t)(base.credits * (won ? win.credits : 1.0) + 0.5);
		const int64_t xp      = nxp * boost * xp_percent / 100;
		const int64_t tokens  = ntk * boost * tokens_percent / 100 + ava_flat[won ? 1 : 0] * boost;
		const int64_t credits = ncr * boost * credits_percent / 100;
		PveRewardResult::Paid paid;
		paid.xp = xp; paid.credits = credits;
		if (boost > 1) paid.notes.push_back({"Booster x2", nxp * (boost - 1), ncr * (boost - 1), ntk * (boost - 1)});
		for (auto& n : HookNotes(hook_reply, nxp * boost, ncr * boost, ntk * boost)) paid.notes.push_back(std::move(n));
		const int64_t level = GetCharacterLevelXp(seat.cid).level;
		int64_t ap = won ? ApRewards::Grant(seat.cid, level, "ava_win",
			ApForLevel(level, ApRewards::Get().ava_win), true, true) : 0;
		if (sqlite3_prepare_v2(db,
		        "UPDATE ga_characters SET "
		        "  xp = xp + ?1, "  /* cap + lv50 credits = trigger trg_lv50_xp_credits */
		        "  tokens = tokens + ?2, credits = credits + ?3 WHERE id = ?4 AND deleted_at = 0",
		        -1, &s, nullptr) == SQLITE_OK) {
			paid.tokens = LimitTokenGain(seat.cid, tokens);
			sqlite3_bind_int64(s, 1, xp);
			sqlite3_bind_int64(s, 2, paid.tokens);
			sqlite3_bind_int64(s, 3, credits);
			sqlite3_bind_int64(s, 4, seat.cid);
			sqlite3_step(s);
			sqlite3_finalize(s);
		}
		AgencyBank::TaxEarnings(seat.cid, credits);   // agency tax

		// Territory this agent attacked (consumed either way).
		int64_t zone = -1, pos = -1, behalf = 0;  // behalf: owner of the opened won bid it ran under
		if (sqlite3_prepare_v2(db,
		        "DELETE FROM ga_ava_attacks WHERE character_id = ? RETURNING zone_id, position, on_behalf_of",
		        -1, &s, nullptr) == SQLITE_OK) {
			sqlite3_bind_int64(s, 1, seat.cid);
			if (sqlite3_step(s) == SQLITE_ROW) {
				zone = sqlite3_column_int64(s, 0);
				pos  = sqlite3_column_int64(s, 1);
				behalf = sqlite3_column_int64(s, 2);
			}
			while (sqlite3_step(s) == SQLITE_ROW) {}
			sqlite3_finalize(s);
		}
		const bool capture = won && pos >= 0 && seat.tf == 1 && !AvaParity::IsTheft(instance_id, seat.cid);  // theft
		if (capture) SettleAvaProduction();  // old owner keeps what it produced so far
		if (capture) ap += ApRewards::Grant(seat.cid, level, "ava_capture",
			ApForLevel(level, ApRewards::Get().ava_capture), true, true);
		// Defend: a forced tf-2 defender (no attack row) wins against at least one real attacker.
		// p0997c: or it fought bot attackers (no real attacker, but own match activity); an empty solo pop still pays none.
		bool fought = real_attackers > 0;
		if (!fought && won && seat.tf == 2 && pos < 0 && sqlite3_prepare_v2(db,
		        "SELECT 1 FROM ga_match_player_stats WHERE instance_id = ?1 AND character_id = ?2 AND "
		        "rep_points + kills + damage_dealt + healing + obj_points + objective_captures > 0 LIMIT 1",
		        -1, &s, nullptr) == SQLITE_OK) {
			sqlite3_bind_int64(s, 1, instance_id);
			sqlite3_bind_int64(s, 2, seat.cid);
			fought = sqlite3_step(s) == SQLITE_ROW;
			sqlite3_finalize(s);
		}
		const bool defend = won && seat.tf == 2 && pos < 0 && fought;
		if (defend) ap += ApRewards::Grant(seat.cid, level, "ava_defend",
			ApForLevel(level, ApRewards::Get().ava_defend), true, true);
		const int64_t bid_owner = behalf > 0 ? behalf : AvaParity::OwnerOf(seat.cid);
		if (bid_owner > 0 && pos >= 0 && sqlite3_prepare_v2(db,  // the won bid is spent by its attack's result
		        "DELETE FROM ga_ava_bids WHERE zone_id = ?1 AND position = ?2 AND owner_agency_id = ?3",
		        -1, &s, nullptr) == SQLITE_OK) {
			sqlite3_bind_int64(s, 1, zone);
			sqlite3_bind_int64(s, 2, pos);
			sqlite3_bind_int64(s, 3, bid_owner);
			sqlite3_step(s);
			sqlite3_finalize(s);
		}
		if (capture && behalf > 0) {
			if (sqlite3_prepare_v2(db,
			        "INSERT OR REPLACE INTO ga_ava_territories (zone_id, position, owner_agency_id,"
			        " owner_name, color_r, color_g, color_b, captured_at) "
			        "SELECT ?1, ?2, a.id, a.name, a.color_r, a.color_g, a.color_b, ?3 FROM ga_agencies a WHERE a.id = ?4",
			        -1, &s, nullptr) == SQLITE_OK) {
				sqlite3_bind_int64(s, 1, zone);
				sqlite3_bind_int64(s, 2, pos);
				sqlite3_bind_int64(s, 3, now);
				sqlite3_bind_int64(s, 4, behalf);
				sqlite3_step(s);
				sqlite3_finalize(s);
			}
		} else if (capture && sqlite3_prepare_v2(db,
		        "INSERT OR REPLACE INTO ga_ava_territories (zone_id, position, owner_agency_id,"
		        " owner_name, color_r, color_g, color_b, captured_at) "
		        "SELECT ?1, ?2, COALESCE(a.id, ?4 + c.id), COALESCE(a.name, u.username),"
		        " COALESCE(a.color_r, 0.9), COALESCE(a.color_g, 0.6), COALESCE(a.color_b, 0.1), ?5 "
		        "FROM ga_characters c JOIN ga_users u ON u.id = c.user_id "
		        "LEFT JOIN ga_agency_members m ON m.user_id = c.user_id "
		        "LEFT JOIN ga_agencies a ON a.id = m.agency_id WHERE c.id = ?3",
		        -1, &s, nullptr) == SQLITE_OK) {
			sqlite3_bind_int64(s, 1, zone);
			sqlite3_bind_int64(s, 2, pos);
			sqlite3_bind_int64(s, 3, seat.cid);
			sqlite3_bind_int64(s, 4, kSoloOwnerBase);
			sqlite3_bind_int64(s, 5, now);
			sqlite3_step(s);
			sqlite3_finalize(s);
		}
		if (sqlite3_prepare_v2(db,
		        "INSERT INTO ga_ava_records (character_id, wins, losses, captures) VALUES (?1, ?2, ?3, ?4) "
		        "ON CONFLICT(character_id) DO UPDATE SET wins = wins + ?2, losses = losses + ?3, "
		        "captures = captures + ?4",
		        -1, &s, nullptr) == SQLITE_OK) {
			sqlite3_bind_int64(s, 1, seat.cid);
			sqlite3_bind_int(s, 2, won ? 1 : 0);
			sqlite3_bind_int(s, 3, won ? 0 : 1);
			sqlite3_bind_int(s, 4, capture ? 1 : 0);
			sqlite3_step(s);
			sqlite3_finalize(s);
		}
		Logger::Log("rewards",
			"[AvA] instance=%lld char=%lld tf=%d winner_tf=%d -> +%lld xp +%lld credits +%lld tokens +%lld AP%s%s\n",
			(long long)instance_id, (long long)seat.cid, seat.tf, winning_task_force,
			(long long)xp, (long long)credits, (long long)tokens, (long long)ap,
			boost > 1 ? " [booster x2]" : "",
			capture ? (" captured pos " + std::to_string(pos)).c_str() : "");
		PveRewardResult r;
		r.xp = xp; r.credits = credits; r.tokens = paid.tokens; r.agenda_points = ap;
		paid.agenda_points = ap;
		r.paid[seat.cid] = std::move(paid);
		r.characters.push_back(seat.cid);
		result.push_back(std::move(r));
	}
	return result;
}

void PlayerSessionStore::SeedStarterWeapons(int64_t character_id) {
	// No lock: called next to PinClassDeviceSlot14 (callers may hold mutex_).
	sqlite3* db = Database::GetConnection();
	if (!db) return;

	int64_t user_id = 0;
	uint32_t profile_id = 0;
	{
		sqlite3_stmt* s = nullptr;
		if (sqlite3_prepare_v2(db,
		        "SELECT user_id, profile_id FROM ga_characters WHERE id = ?",
		        -1, &s, nullptr) == SQLITE_OK) {
			sqlite3_bind_int64(s, 1, character_id);
			if (sqlite3_step(s) == SQLITE_ROW) {
				user_id    = sqlite3_column_int64(s, 0);
				profile_id = static_cast<uint32_t>(sqlite3_column_int(s, 1));
			}
			sqlite3_finalize(s);
		}
	}
	if (user_id == 0) return;

	// Engine equip point <- allowed_slots value (see equip-slots.md).
	// Pick the class item with the lowest required_level (the game's own
	// asm_data_set_items data), common quality first, oldest row last.
	static const struct { int equip_point; const char* svid; } kSlots[] = {
		{ 1, "221" },  // melee
		{ 2, "198" },  // ranged
		{ 3, "200" },  // specialty
		{ 5, "201" },  // jetpack
	};
	const char* kPick =
		"SELECT inv.id FROM ga_players_inventory inv "
		"WHERE inv.user_id = ? AND inv.profile_id = ? AND inv.allowed_slots = ? "
		"ORDER BY COALESCE((SELECT MIN(a.required_level) FROM asm_data_set_items a "
		"                   WHERE a.item_id = inv.device_id), 99), "
		"         inv.quality DESC, inv.id "
		"LIMIT 1";
	const char* kInsert =
		"INSERT OR IGNORE INTO ga_character_devices "
		"(character_id, item_profile_id, inventory_id, equipped_slot) "
		"VALUES (?, ?, ?, ?)";
	sqlite3_stmt* pick = nullptr;
	sqlite3_stmt* ins  = nullptr;
	if (sqlite3_prepare_v2(db, kPick,   -1, &pick, nullptr) != SQLITE_OK ||
	    sqlite3_prepare_v2(db, kInsert, -1, &ins,  nullptr) != SQLITE_OK) {
		Logger::Log("db", "[PlayerSessionStore] SeedStarterWeapons: prepare failed: %s\n",
			sqlite3_errmsg(db));
		if (pick) sqlite3_finalize(pick);
		if (ins)  sqlite3_finalize(ins);
		return;
	}
	int added = 0;
	for (const auto& slot : kSlots) {
		sqlite3_reset(pick);
		sqlite3_bind_int64(pick, 1, user_id);
		sqlite3_bind_int  (pick, 2, (int)profile_id);
		sqlite3_bind_text (pick, 3, slot.svid, -1, SQLITE_STATIC);
		if (sqlite3_step(pick) != SQLITE_ROW) continue;
		const int inv_id = sqlite3_column_int(pick, 0);
		for (int item_profile_id = 1; item_profile_id <= 5; ++item_profile_id) {
			sqlite3_reset(ins);
			sqlite3_bind_int64(ins, 1, character_id);
			sqlite3_bind_int  (ins, 2, item_profile_id);
			sqlite3_bind_int  (ins, 3, inv_id);
			sqlite3_bind_int  (ins, 4, slot.equip_point);
			if (sqlite3_step(ins) == SQLITE_DONE && sqlite3_changes(db) > 0) ++added;
		}
	}
	sqlite3_finalize(pick);
	sqlite3_finalize(ins);
	Logger::Log("db",
		"[PlayerSessionStore] SeedStarterWeapons: char=%lld profile=%u newly-equipped=%d\n",
		(long long)character_id, profile_id, added);
}

std::vector<InventoryRow> PlayerSessionStore::GetInventoryForUser(int64_t user_id, uint32_t profile_id) {
	std::lock_guard<std::mutex> lock(mutex_);
	std::vector<InventoryRow> result;
	sqlite3* db = Database::GetConnection();
	if (!db) return result;

	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"SELECT id, profile_id, device_id, quality, mod_effect_group_ids, oc, allowed_slots, item_id, stock_n "
		"FROM ga_players_inventory "
		"WHERE user_id = ? AND (profile_id = 0 OR profile_id = ?) "
		"ORDER BY id",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[PlayerSessionStore] GetInventoryForUser prepare failed: %s\n", sqlite3_errmsg(db));
		return result;
	}
	sqlite3_bind_int64(stmt, 1, user_id);
	sqlite3_bind_int(stmt,   2, (int)profile_id);

	while (sqlite3_step(stmt) == SQLITE_ROW) {
		InventoryRow row;
		row.inventory_id  = sqlite3_column_int(stmt, 0);
		row.profile_id    = sqlite3_column_int(stmt, 1);
		row.device_id     = sqlite3_column_int(stmt, 2);
		row.quality       = sqlite3_column_int(stmt, 3);
		row.mods          = CsvToMods((const char*)sqlite3_column_text(stmt, 4));
		row.oc            = sqlite3_column_int(stmt, 5) != 0;
		row.allowed_slots = CsvToInts((const char*)sqlite3_column_text(stmt, 6));
		row.item_id       = sqlite3_column_int(stmt, 7);
		row.stock_n       = sqlite3_column_int(stmt, 8);
		result.push_back(row);
	}
	sqlite3_finalize(stmt);
	return result;
}

std::vector<DeviceRow> PlayerSessionStore::GetDevicesForCharacter(int64_t character_id,
                                                                  int item_profile_id) {
	std::lock_guard<std::mutex> lock(mutex_);
	std::vector<DeviceRow> result;
	sqlite3* db = Database::GetConnection();
	if (!db) return result;

	// JOIN: device descriptor comes from the inventory pool; the per-character
	// row contributes just the equipped slot. SVID is derived from the slot
	// rather than stored (it's a function of equip_point, not of the item).
	// Profile-scoped: only returns rows for the active loadout slot.
	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"SELECT d.equipped_slot, i.id, i.device_id, i.quality, "
		"       i.mod_effect_group_ids, i.oc, i.item_id "
		"FROM ga_character_devices d "
		"JOIN ga_players_inventory i ON i.id = d.inventory_id "
		"WHERE d.character_id = ? AND d.item_profile_id = ? "
		"ORDER BY d.equipped_slot",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[PlayerSessionStore] GetDevicesForCharacter prepare failed: %s\n", sqlite3_errmsg(db));
		return result;
	}
	sqlite3_bind_int64(stmt, 1, character_id);
	sqlite3_bind_int  (stmt, 2, item_profile_id);
	const int agentLevel = SlotUnlock::CharacterLevel(db, character_id);

	while (sqlite3_step(stmt) == SQLITE_ROW) {
		DeviceRow row;
		row.equip_slot      = sqlite3_column_int(stmt, 0);
		// A device in a slot the agent has not unlocked yet is
		// not equipped (the client shows it in the bag until the level).
		if (!IsArmorSvid(row.equip_slot) && row.equip_slot <= 24
		    && !SlotUnlock::IsEquipPointUnlocked(db, agentLevel,
		           CosmeticSlots::EngineSlotFor(row.equip_slot))) {
			continue;
		}
		row.inventory_id    = sqlite3_column_int(stmt, 1);
		row.device_id       = sqlite3_column_int(stmt, 2);
		row.quality         = sqlite3_column_int(stmt, 3);
		row.mods            = CsvToMods((const char*)sqlite3_column_text(stmt, 4));
		row.oc              = sqlite3_column_int(stmt, 5) != 0;
		row.item_id         = sqlite3_column_int(stmt, 6);
		// Under-level gear (e.g. written by an old/forged save)
		// is not equipped.
		if (!SlotUnlock::IsItemUsable(db, agentLevel, row.device_id, row.item_id)) continue;
		// SVID is computed from the LOGICAL engine slot. Cosmetic suit/helmet
		// rows are stored at DB slot 22/23 internally (see CosmeticSlots.hpp),
		// but on the wire the client expects them at their engine slot's SVID
		// (202 for ES6 Suit, 500 for ES12 Helmet) — otherwise the equip-screen
		// UI can't bind them to the right slot widget.
		//
		// Armor (group-126 SVIDs 1107/1109/1110/1113/1116/1119/1120) bypasses
		// the engine-equip-point remap entirely — the DB column already holds
		// the wire SVID. EquipPointToSvid only knows ES1..ES24; armor SVIDs
		// would fall through to 0 and the client would see every armor piece
		// as unequipped.
		row.slot_value_id   = IsArmorSvid(row.equip_slot)
			? row.equip_slot
			: EquipPointToSvid(CosmeticSlots::EngineSlotFor(row.equip_slot));
		row.effect_group_id = 0;  // legacy column — unused by readers, kept for struct layout
		result.push_back(row);
	}
	sqlite3_finalize(stmt);
	return result;
}

bool PlayerSessionStore::SaveEquippedDevices(int64_t character_id, int64_t user_id,
                                              uint32_t profile_id,
                                              int item_profile_id,
                                              const std::map<int, int>& slot_to_inventory,
                                              const std::map<int, int>& misc_items,
                                              const std::set<int>& cleared_cosmetic_slots) {
	if (item_profile_id < 1 || item_profile_id > 5) {
		Logger::Log("armor",
			"[Save] REJECT char=%lld: item_profile_id=%d out of range [1..5]\n",
			character_id, item_profile_id);
		return false;
	}
	sqlite3* db = Database::GetConnection();
	if (!db) return false;

	// Slot 14 (CLASS_DEVICE / HUMAN BASE ATTRIBUTES) is server-pinned — the
	// player can't un-equip or replace it. The client UI hides the slot, so
	// it always sends SlotIndices[14]=0. We look up the class device
	// inventory_id for this user+profile and FORCE it into the save here,
	// regardless of what the client sent. This way the regular
	// ga_character_devices path handles it like any other equipped device
	// (no synthetic invId), and the disconnect cleanup path doesn't see a
	// poisoned TMap entry like it did with the old 0x7FFFFFFE hack.
	//
	// Identification: allowed_slots='502' is unique to the class device
	// today (no other ClassLoadouts entry has SVID_CLASS_DEVICE). If we ever
	// add more slot-14-only items, switch this query to also gate on
	// device_id matching asm_data_set_bots_data_set_bot_devices for the
	// profile.
	std::map<int, int> save_map = slot_to_inventory;
	{
		sqlite3_stmt* clsStmt = nullptr;
		if (sqlite3_prepare_v2(db,
		    "SELECT id FROM ga_players_inventory "
		    "WHERE user_id = ? AND (profile_id = 0 OR profile_id = ?) "
		    "  AND allowed_slots = '502' "
		    "ORDER BY profile_id DESC "  // prefer profile-specific over shared
		    "LIMIT 1",
		    -1, &clsStmt, nullptr) == SQLITE_OK) {
			sqlite3_bind_int64(clsStmt, 1, user_id);
			sqlite3_bind_int(clsStmt,   2, (int)profile_id);
			if (sqlite3_step(clsStmt) == SQLITE_ROW) {
				const int class_inv_id = sqlite3_column_int(clsStmt, 0);
				save_map[14] = class_inv_id;  // overwrites whatever the client sent
				Logger::Log("armor",
					"[Save] pinning slot 14 to class-device inventory_id=%d (user=%lld profile=%u)\n",
					class_inv_id, user_id, profile_id);
			} else {
				Logger::Log("armor",
					"[Save] WARNING: no class device in inventory for user=%lld profile=%u — slot 14 will be empty\n",
					user_id, profile_id);
			}
			sqlite3_finalize(clsStmt);
		}
	}

	// Level-gated device slots (src/Shared/SlotUnlock.hpp). A
	// still-locked slot is dropped from the save (its DB row, if any, is left
	// alone and lights up at the unlock level); the rest of the save lands.
	{
		const int level = SlotUnlock::CharacterLevel(db, character_id);
		for (auto it = save_map.begin(); it != save_map.end(); ) {
			if (!SlotUnlock::IsEquipPointUnlocked(db, level, it->first)) {
				Logger::Log("armor",
					"[Save] char=%lld level=%d: slot %d (svid %d) locked until level %d - inventory_id=%d not equipped\n",
					character_id, level, it->first, SlotUnlock::EquipPointToSvid(it->first),
					SlotUnlock::RequiredLevelForSvid(db, SlotUnlock::EquipPointToSvid(it->first)), it->second);
				it = save_map.erase(it);
				continue;
			}
			// The item's own required level (game data). The client refuses
			// under-level gear itself; this stops a forged equip_save.
			int dev = 0, item = 0;
			sqlite3_stmt* q = nullptr;
			if (sqlite3_prepare_v2(db,
			        "SELECT device_id, item_id FROM ga_players_inventory WHERE id = ?",
			        -1, &q, nullptr) == SQLITE_OK && q) {
				sqlite3_bind_int(q, 1, it->second);
				if (sqlite3_step(q) == SQLITE_ROW) {
					dev  = sqlite3_column_int(q, 0);
					item = sqlite3_column_int(q, 1);
				}
				sqlite3_finalize(q);
			}
			const int need = SlotUnlock::RequiredLevelForItem(db, dev, item);
			if (level < need) {
				Logger::Log("armor",
					"[Save] REFUSED char=%lld level=%d: slot %d inventory_id=%d (device %d item %d) requires level %d - not equipped\n",
					character_id, level, it->first, it->second, dev, item, need);
				it = save_map.erase(it);
			} else {
				++it;
			}
		}
	}

	// Validate every (equip_point, inventory_id) BEFORE we mutate.
	// A bad slot is dropped on its own (its DB row is left as it
	// was) and the rest of the save lands - one refused item used to throw
	// away every other change (e.g. a Consume-pocket drag
	// lost the whole hotbar). The caller resyncs the client from the DB.
	//
	// Also resolve each inventory row's cosmetic subtype so the insert pass
	// below can remap engine slot → DB slot for cosmetic suit/helmet (see
	// src/Shared/CosmeticSlots.hpp for the rationale).
	sqlite3_stmt* vstmt = nullptr;
	if (sqlite3_prepare_v2(db,
	    "SELECT pi.user_id, pi.profile_id, pi.allowed_slots, pi.device_id, pi.item_id, "
	    "       COALESCE(ai.item_subtype_value_id, 0) "
	    "FROM ga_players_inventory pi "
	    "LEFT JOIN asm_data_set_items ai ON ai.item_id = pi.item_id "
	    "WHERE pi.id = ?",
	    -1, &vstmt, nullptr) != SQLITE_OK) {
		Logger::Log("armor", "[Save] validate prepare failed: %s\n", sqlite3_errmsg(db));
		return false;
	}

	// Capture per-row subtype to compute db_slot at insert time.
	std::map<int, int> inv_subtype;  // inventory_id → item_subtype_value_id (0 for non-cosmetic)
	std::set<int> clear_slots;

	for (auto kv_it = save_map.begin(); kv_it != save_map.end(); ) {
		const int equip_point = kv_it->first;
		const int inventory_id = kv_it->second;
		sqlite3_reset(vstmt);
		sqlite3_clear_bindings(vstmt);
		sqlite3_bind_int(vstmt, 1, inventory_id);
		if (sqlite3_step(vstmt) != SQLITE_ROW) {
			Logger::Log("armor",
				"[Save] DROP slot char=%lld: inventory_id=%d not found\n",
				character_id, inventory_id);
			kv_it = save_map.erase(kv_it);
			continue;
		}
		const int64_t inv_user    = sqlite3_column_int64(vstmt, 0);
		const int     inv_profile = sqlite3_column_int(vstmt, 1);
		const std::vector<int> allowed = CsvToInts((const char*)sqlite3_column_text(vstmt, 2));
		const int     inv_device  = sqlite3_column_int(vstmt, 3);
		const int     inv_itemId  = sqlite3_column_int(vstmt, 4);
		const int     inv_subt    = sqlite3_column_int(vstmt, 5);
		if (inv_user != user_id) {
			Logger::Log("armor",
				"[Save] DROP slot char=%lld: inventory_id=%d owned by user %lld, not %lld\n",
				character_id, inventory_id, inv_user, user_id);
			kv_it = save_map.erase(kv_it);
			continue;
		}
		if (inv_profile != 0 && inv_profile != (int)profile_id) {
			Logger::Log("armor",
				"[Save] DROP slot char=%lld: inventory_id=%d profile=%d, char profile=%u\n",
				character_id, inventory_id, inv_profile, profile_id);
			kv_it = save_map.erase(kv_it);
			continue;
		}
		const int target_svid = EquipPointToSvid(equip_point);
		bool slotAllowed = false;
		for (int s : allowed) { if (s == target_svid) { slotAllowed = true; break; } }
		if (!slotAllowed) {
			Logger::Log("armor",
				"[Save] DROP slot char=%lld: inventory_id=%d svid=%d not in allowed_slots\n",
				character_id, inventory_id, target_svid);
			kv_it = save_map.erase(kv_it);
			continue;
		}
		// Stash subtype for cosmetic rows (deviceId=0, itemId>0). Non-cosmetic
		// rows get subtype 0 so DbSlotFor passes through.
		inv_subtype[inventory_id] = (inv_device == 0 && inv_itemId > 0) ? inv_subt : 0;
		if (inv_device == 0 && IsDyeNoneSlot(inv_itemId, equip_point)) {
			clear_slots.insert(equip_point);
		}
		++kv_it;
	}
	sqlite3_finalize(vstmt);

	// A save that leaves out a weapon/jetpack slot (1/2/3/5)
	// this agent still has equipped comes from a client whose equip screen only
	// knows part of the loadout (observed: four
	// saves of 1-4 slots each wiped the rest, 12 -> 6 equipped). The UI never
	// blanks those slots, so such a save only writes what it carries: no
	// cosmetic clears and armor is replaced piece by piece. A full save may
	// still unequip one armor piece per Apply.
	bool partialSave = false;
	std::set<int> equippedArmorSvids;
	{
		sqlite3_stmt* cur = nullptr;
		if (sqlite3_prepare_v2(db,
		        "SELECT equipped_slot FROM ga_character_devices "
		        "WHERE character_id = ? AND item_profile_id = ?",
		        -1, &cur, nullptr) == SQLITE_OK && cur) {
			sqlite3_bind_int64(cur, 1, character_id);
			sqlite3_bind_int  (cur, 2, item_profile_id);
			while (sqlite3_step(cur) == SQLITE_ROW) {
				const int s = sqlite3_column_int(cur, 0);
				if ((s == 1 || s == 2 || s == 3 || s == 5) && slot_to_inventory.count(s) == 0)
					partialSave = true;
				if (IsArmorSvid(s)) equippedArmorSvids.insert(s);
			}
			sqlite3_finalize(cur);
		}
	}
	int armorOmitted = 0;
	for (int s : equippedArmorSvids)
		if (misc_items.count(s - 1128) == 0) ++armorOmitted;
	const bool armorFullWipe = !partialSave && armorOmitted <= 1;
	if (partialSave || (!misc_items.empty() && !armorFullWipe)) {
		Logger::Log("armor",
			"[Save] partial save char=%lld (weapon/jetpack slot missing=%d, armor omitted=%d) - only sent slots written\n",
			character_id, partialSave ? 1 : 0, armorOmitted);
	}

	// All validation passed. Update only the slots in this payload. The equip
	// UI can send gameplay slots plus the one cosmetic the player changed, so
	// absent cosmetic slots are not reliable "clear" signals.
	char* err = nullptr;
	sqlite3_exec(db, "BEGIN", nullptr, nullptr, &err);

	sqlite3_stmt* del = nullptr;
	if (sqlite3_prepare_v2(db,
	    "DELETE FROM ga_character_devices "
	    "WHERE character_id = ? AND item_profile_id = ? AND equipped_slot = ?",
	    -1, &del, nullptr) != SQLITE_OK) {
		sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, &err);
		return false;
	}
	for (const auto& kv : save_map) {
		const int engine_slot = kv.first;
		const int inv_id      = kv.second;
		const int subtype     = inv_subtype[inv_id];
		const int db_slot     = CosmeticSlots::DbSlotFor(engine_slot, subtype);
		sqlite3_reset(del);
		sqlite3_clear_bindings(del);
		sqlite3_bind_int64(del, 1, character_id);
		sqlite3_bind_int  (del, 2, item_profile_id);
		sqlite3_bind_int  (del, 3, db_slot);
		if (sqlite3_step(del) != SQLITE_DONE) {
			Logger::Log("armor",
				"[Save] DELETE FAILED char=%lld itemProf=%d dbSlot=%d engineSlot=%d: %s\n",
				character_id, item_profile_id, db_slot, engine_slot, sqlite3_errmsg(db));
			sqlite3_finalize(del);
			sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, &err);
			return false;
		}
	}
	sqlite3_finalize(del);

	sqlite3_stmt* ins = nullptr;
	if (sqlite3_prepare_v2(db,
	    "INSERT INTO ga_character_devices "
	    "(character_id, item_profile_id, inventory_id, equipped_slot) "
	    "VALUES (?, ?, ?, ?)",
	    -1, &ins, nullptr) != SQLITE_OK) {
		sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, &err);
		return false;
	}
	for (const auto& kv : save_map) {
		const int engine_slot = kv.first;
		const int inv_id      = kv.second;
		const int subtype     = inv_subtype[inv_id];
		const int db_slot     = CosmeticSlots::DbSlotFor(engine_slot, subtype);
		if (clear_slots.count(engine_slot) > 0) {
			Logger::Log("armor",
				"[Save] cosmetic clear: item %d cleared slot %d (dbSlot=%d)\n",
				kDyeNoneItemId, engine_slot, db_slot);
			continue;
		}
		sqlite3_reset(ins);
		sqlite3_clear_bindings(ins);
		sqlite3_bind_int64(ins, 1, character_id);
		sqlite3_bind_int  (ins, 2, item_profile_id);
		sqlite3_bind_int  (ins, 3, inv_id);
		sqlite3_bind_int  (ins, 4, db_slot);
		if (sqlite3_step(ins) != SQLITE_DONE) {
			Logger::Log("armor",
				"[Save] INSERT FAILED char=%lld itemProf=%d dbSlot=%d engineSlot=%d invId=%d: %s\n",
				character_id, item_profile_id, db_slot, engine_slot, inv_id, sqlite3_errmsg(db));
			sqlite3_finalize(ins);
			sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, &err);
			return false;
		}
	}
	sqlite3_finalize(ins);

	// Armor pass — FTGEQUIP_SLOTS_STRUCT.MiscItems[] carries the Armor tab's
	// equip selections, keyed by `index = slot_value_id - 1128` (group-129
	// SVID space, see ArmorSlot in EquipSlot.hpp). The client always ships
	// the FULL armor state on Apply (a slot the user unequipped reads as 0),
	// so we wipe all 7 armor rows for the character first and then INSERT
	// every non-zero misc_items entry. Skipped entirely when misc_items is
	// empty — that's the "client didn't touch armor" case (e.g. weapon-only
	// Apply), and we keep the existing armor untouched.
	int armorEquipped = 0;
	if (!misc_items.empty()) {
		sqlite3_stmt* delArmor = nullptr;
		if (armorFullWipe && sqlite3_prepare_v2(db,
		    "DELETE FROM ga_character_devices "
		    "WHERE character_id = ? AND item_profile_id = ? "
		    "  AND equipped_slot IN (1130, 1132, 1133, 1136, 1139, 1142, 1143)",
		    -1, &delArmor, nullptr) == SQLITE_OK) {
			sqlite3_bind_int64(delArmor, 1, character_id);
			sqlite3_bind_int  (delArmor, 2, item_profile_id);
			sqlite3_step(delArmor);
			sqlite3_finalize(delArmor);
		} else if (!armorFullWipe && sqlite3_prepare_v2(db,
		    "DELETE FROM ga_character_devices "
		    "WHERE character_id = ? AND item_profile_id = ? AND equipped_slot = ?",
		    -1, &delArmor, nullptr) == SQLITE_OK) {
			// Partial save - replace only the pieces it carries.
			for (const auto& kv : misc_items) {
				sqlite3_reset(delArmor);
				sqlite3_clear_bindings(delArmor);
				sqlite3_bind_int64(delArmor, 1, character_id);
				sqlite3_bind_int  (delArmor, 2, item_profile_id);
				sqlite3_bind_int  (delArmor, 3, kv.first + 1128);
				sqlite3_step(delArmor);
			}
			sqlite3_finalize(delArmor);
		}

		sqlite3_stmt* insArmor = nullptr;
		if (sqlite3_prepare_v2(db,
		    "INSERT INTO ga_character_devices "
		    "(character_id, item_profile_id, inventory_id, equipped_slot) "
		    "VALUES (?, ?, ?, ?)",
		    -1, &insArmor, nullptr) == SQLITE_OK) {
			for (const auto& kv : misc_items) {
				const int misc_index = kv.first;
				const int inv_id     = kv.second;
				const int armor_svid = misc_index + 1128;  // index → group-129 SVID
				// Only accept indices that correspond to one of the 7 visible
				// armor slots. Anything else (Core/Implant/Title slots in the
				// 16-slot underlying group-129 structure) is ignored — the
				// shipped UI doesn't expose them.
				if (armor_svid != 1130 && armor_svid != 1132 && armor_svid != 1133 &&
				    armor_svid != 1136 && armor_svid != 1139 && armor_svid != 1142 &&
				    armor_svid != 1143) {
					Logger::Log("armor",
						"[Save] armor: ignoring MiscItems[%d]=inv_id %d (not a visible armor slot)\n",
						misc_index, inv_id);
					continue;
				}
				// The armor row must belong to this user (+ profile)
				// and its required level must be reached (forged equip_save).
				{
					int64_t owner = -1; int prof = -1, dev = 0, item = 0;
					sqlite3_stmt* q = nullptr;
					if (sqlite3_prepare_v2(db,
					        "SELECT user_id, profile_id, device_id, item_id FROM ga_players_inventory WHERE id = ?",
					        -1, &q, nullptr) == SQLITE_OK && q) {
						sqlite3_bind_int(q, 1, inv_id);
						if (sqlite3_step(q) == SQLITE_ROW) {
							owner = sqlite3_column_int64(q, 0);
							prof  = sqlite3_column_int(q, 1);
							dev   = sqlite3_column_int(q, 2);
							item  = sqlite3_column_int(q, 3);
						}
						sqlite3_finalize(q);
					}
					if (owner != user_id || (prof != 0 && prof != (int)profile_id)) {
						Logger::Log("armor",
							"[Save] REFUSED char=%lld: armor inv_id %d not owned by user %lld / profile %u\n",
							character_id, inv_id, user_id, profile_id);
						continue;
					}
					const int level = SlotUnlock::CharacterLevel(db, character_id);
					const int need  = SlotUnlock::RequiredLevelForItem(db, dev, item);
					if (level < need) {
						Logger::Log("armor",
							"[Save] REFUSED char=%lld level=%d: armor inv_id %d (device %d item %d) requires level %d - not equipped\n",
							character_id, level, inv_id, dev, item, need);
						continue;
					}
				}
				sqlite3_reset(insArmor);
				sqlite3_clear_bindings(insArmor);
				sqlite3_bind_int64(insArmor, 1, character_id);
				sqlite3_bind_int  (insArmor, 2, item_profile_id);
				sqlite3_bind_int  (insArmor, 3, inv_id);
				sqlite3_bind_int  (insArmor, 4, armor_svid);
				if (sqlite3_step(insArmor) == SQLITE_DONE) {
					++armorEquipped;
					Logger::Log("armor",
						"[Save] armor: MiscItems[%d] inv_id %d → equipped_slot %d\n",
						misc_index, inv_id, armor_svid);
				}
			}
			sqlite3_finalize(insArmor);
		}
	}

	// Cosmetic suit/helmet clear. `cleared_cosmetic_slots` carries engine
	// slots (6/12) the client explicitly blanked (SlotIndices[slot]==0) —
	// see CosmeticSlots.hpp for the engine→DB remap rationale. Independent
	// of slot_to_inventory since there's no inventory row to validate when
	// clearing.
	// A partial save's "blank" cosmetic slots are just unknown to the client.
	if (!cleared_cosmetic_slots.empty() && !partialSave) {
		sqlite3_stmt* delCosmetic = nullptr;
		if (sqlite3_prepare_v2(db,
		    "DELETE FROM ga_character_devices "
		    "WHERE character_id = ? AND item_profile_id = ? AND equipped_slot = ?",
		    -1, &delCosmetic, nullptr) == SQLITE_OK) {
			for (int engine_slot : cleared_cosmetic_slots) {
				const int db_slot = (engine_slot == 6)  ? CosmeticSlots::kCosmeticSuitDbSlot
				                  : (engine_slot == 12) ? CosmeticSlots::kCosmeticHelmetDbSlot
				                                         : engine_slot;
				sqlite3_reset(delCosmetic);
				sqlite3_clear_bindings(delCosmetic);
				sqlite3_bind_int64(delCosmetic, 1, character_id);
				sqlite3_bind_int  (delCosmetic, 2, item_profile_id);
				sqlite3_bind_int  (delCosmetic, 3, db_slot);
				if (sqlite3_step(delCosmetic) != SQLITE_DONE) {
					Logger::Log("armor",
						"[Save] cosmetic clear FAILED char=%lld engineSlot=%d dbSlot=%d: %s\n",
						character_id, engine_slot, db_slot, sqlite3_errmsg(db));
				} else {
					Logger::Log("armor",
						"[Save] cosmetic clear: engineSlot=%d dbSlot=%d cleared\n",
						engine_slot, db_slot);
				}
			}
			sqlite3_finalize(delCosmetic);
		}
	}

	sqlite3_exec(db, "COMMIT", nullptr, nullptr, &err);
	Logger::Log("armor",
		"[Save] OK char=%lld user=%lld profile=%u itemProf=%d entries=%zu armor=%d\n",
		character_id, user_id, profile_id, item_profile_id, save_map.size(), armorEquipped);
	return true;
}

std::vector<SkillRow> PlayerSessionStore::GetSkillsForCharacter(int64_t character_id,
                                                                int item_profile_id) {
	std::lock_guard<std::mutex> lock(mutex_);
	sqlite3* db = Database::GetConnection();
	std::vector<SkillRow> result;

	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"SELECT skill_group_id, skill_id, points "
		"FROM ga_character_skills "
		"WHERE character_id = ? AND item_profile_id = ? AND points > 0 "
		"ORDER BY skill_group_id, skill_id",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[PlayerSessionStore] GetSkillsForCharacter prepare failed: %s\n", sqlite3_errmsg(db));
		return result;
	}
	sqlite3_bind_int64(stmt, 1, character_id);
	sqlite3_bind_int  (stmt, 2, item_profile_id);

	while (sqlite3_step(stmt) == SQLITE_ROW) {
		SkillRow row;
		row.skill_group_id = sqlite3_column_int(stmt, 0);
		row.skill_id       = sqlite3_column_int(stmt, 1);
		row.points         = sqlite3_column_int(stmt, 2);
		row.item_profile_id = item_profile_id;  // caller-known
		result.push_back(row);
	}
	sqlite3_finalize(stmt);
	return result;
}

std::vector<SkillRow> PlayerSessionStore::GetAllSkillsForCharacter(int64_t character_id) {
	std::lock_guard<std::mutex> lock(mutex_);
	sqlite3* db = Database::GetConnection();
	std::vector<SkillRow> result;
	if (!db) return result;

	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"SELECT skill_group_id, skill_id, points, item_profile_id "
		"FROM ga_character_skills "
		"WHERE character_id = ? AND points > 0 "
		"ORDER BY item_profile_id, skill_group_id, skill_id",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[PlayerSessionStore] GetAllSkillsForCharacter prepare failed: %s\n",
			sqlite3_errmsg(db));
		return result;
	}
	sqlite3_bind_int64(stmt, 1, character_id);

	while (sqlite3_step(stmt) == SQLITE_ROW) {
		SkillRow row;
		row.skill_group_id  = sqlite3_column_int(stmt, 0);
		row.skill_id        = sqlite3_column_int(stmt, 1);
		row.points          = sqlite3_column_int(stmt, 2);
		row.item_profile_id = sqlite3_column_int(stmt, 3);
		result.push_back(row);
	}
	sqlite3_finalize(stmt);
	return result;
}

void PlayerSessionStore::SetSkillsForCharacter(int64_t character_id, int item_profile_id,
                                               const std::vector<SkillRow>& skills) {
	std::lock_guard<std::mutex> lock(mutex_);
	sqlite3* db = Database::GetConnection();

	char* err = nullptr;
	if (sqlite3_exec(db, "BEGIN", nullptr, nullptr, &err) != SQLITE_OK) {
		Logger::Log("db", "[PlayerSessionStore] SetSkillsForCharacter begin failed: %s\n", err);
		sqlite3_free(err);
		return;
	}

	sqlite3_stmt* del = nullptr;
	sqlite3_prepare_v2(db,
		"DELETE FROM ga_character_skills "
		"WHERE character_id = ? AND item_profile_id = ?",
		-1, &del, nullptr);
	if (del) {
		sqlite3_bind_int64(del, 1, character_id);
		sqlite3_bind_int  (del, 2, item_profile_id);
		sqlite3_step(del);
		sqlite3_finalize(del);
	}

	sqlite3_stmt* ins = nullptr;
	sqlite3_prepare_v2(db,
		"INSERT INTO ga_character_skills "
		"(character_id, item_profile_id, skill_group_id, skill_id, points) "
		"VALUES (?, ?, ?, ?, ?)",
		-1, &ins, nullptr);
	if (ins) {
		for (const auto& s : skills) {
			if (s.points <= 0) continue;  // skip zero rows — not worth persisting
			sqlite3_bind_int64(ins, 1, character_id);
			sqlite3_bind_int  (ins, 2, item_profile_id);
			sqlite3_bind_int  (ins, 3, s.skill_group_id);
			sqlite3_bind_int  (ins, 4, s.skill_id);
			sqlite3_bind_int  (ins, 5, s.points);
			sqlite3_step(ins);
			sqlite3_reset(ins);
		}
		sqlite3_finalize(ins);
	}

	sqlite3_stmt* bump = nullptr;
	sqlite3_prepare_v2(db,
		"UPDATE ga_characters SET last_respec_at = strftime('%s','now') WHERE id = ?",
		-1, &bump, nullptr);
	if (bump) {
		sqlite3_bind_int64(bump, 1, character_id);
		sqlite3_step(bump);
		sqlite3_finalize(bump);
	}

	if (sqlite3_exec(db, "COMMIT", nullptr, nullptr, &err) != SQLITE_OK) {
		Logger::Log("db", "[PlayerSessionStore] SetSkillsForCharacter commit failed: %s\n", err);
		sqlite3_free(err);
		sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
		return;
	}

	Logger::Log("db", "[PlayerSessionStore] Saved %d skill rows for character %lld itemProf=%d\n",
		(int)skills.size(), (long long)character_id, item_profile_id);
}

void PlayerSessionStore::ClearSkillsForCharacter(int64_t character_id, int item_profile_id) {
	SetSkillsForCharacter(character_id, item_profile_id, {});
}

int PlayerSessionStore::GetCurrentItemProfile(int64_t character_id) {
	std::lock_guard<std::mutex> lock(mutex_);
	sqlite3* db = Database::GetConnection();
	int v = 1;
	sqlite3_stmt* st = nullptr;
	if (sqlite3_prepare_v2(db,
			"SELECT current_item_profile_id FROM ga_characters WHERE id = ?",
			-1, &st, nullptr) == SQLITE_OK) {
		sqlite3_bind_int64(st, 1, character_id);
		if (sqlite3_step(st) == SQLITE_ROW) v = sqlite3_column_int(st, 0);
		sqlite3_finalize(st);
	}
	if (v < 1 || v > 5) v = 1;
	return v;
}

void PlayerSessionStore::SetCurrentItemProfile(int64_t character_id, int item_profile_id) {
	if (item_profile_id < 1 || item_profile_id > 5) return;
	std::lock_guard<std::mutex> lock(mutex_);
	sqlite3* db = Database::GetConnection();
	sqlite3_stmt* st = nullptr;
	if (sqlite3_prepare_v2(db,
			"UPDATE ga_characters SET current_item_profile_id = ? WHERE id = ?",
			-1, &st, nullptr) == SQLITE_OK) {
		sqlite3_bind_int  (st, 1, item_profile_id);
		sqlite3_bind_int64(st, 2, character_id);
		sqlite3_step(st);
		sqlite3_finalize(st);
	}
	Logger::Log("loadout", "[SetCurrentItemProfile] char=%lld -> %d\n",
		(long long)character_id, item_profile_id);
}

std::map<int, std::array<int,6>>
PlayerSessionStore::GetPerProfileSlotMap(int64_t character_id) {
	std::lock_guard<std::mutex> lock(mutex_);
	std::map<int, std::array<int,6>> out;
	sqlite3* db = Database::GetConnection();
	if (!db) return out;
	sqlite3_stmt* st = nullptr;
	if (sqlite3_prepare_v2(db,
			"SELECT inventory_id, item_profile_id, equipped_slot "
			"FROM ga_character_devices WHERE character_id = ?",
			-1, &st, nullptr) != SQLITE_OK) {
		return out;
	}
	sqlite3_bind_int64(st, 1, character_id);
	while (sqlite3_step(st) == SQLITE_ROW) {
		const int invId   = sqlite3_column_int(st, 0);
		const int profile = sqlite3_column_int(st, 1);
		const int raw     = sqlite3_column_int(st, 2);
		if (profile < 1 || profile > 5) continue;
		// Resolve to client-facing slot_value_id. Armor SVIDs pass through;
		// engine equip-points 1..24 go through the cosmetic-remap-aware
		// path (same translation GetDevicesForCharacter does at row time).
		const int svid = IsArmorSvid(raw)
			? raw
			: EquipPointToSvid(CosmeticSlots::EngineSlotFor(raw));
		auto& arr = out[invId];
		if (arr[profile] == 0) arr[profile] = svid;  // first wins; UNIQUE prevents dupes
	}
	sqlite3_finalize(st);
	return out;
}

int64_t PlayerSessionStore::GetLastRespecAt(int64_t character_id) {
	std::lock_guard<std::mutex> lock(mutex_);
	sqlite3* db = Database::GetConnection();

	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"SELECT last_respec_at FROM ga_characters WHERE id = ?",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) return 0;

	sqlite3_bind_int64(stmt, 1, character_id);
	int64_t out = 0;
	if (sqlite3_step(stmt) == SQLITE_ROW) {
		out = sqlite3_column_int64(stmt, 0);
	}
	sqlite3_finalize(stmt);
	return out;
}
