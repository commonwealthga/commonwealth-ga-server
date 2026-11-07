#include "src/ControlServer/AvaParity/AvaParity.hpp"
#include "src/ControlServer/AvaZones/AvaZones.hpp"

#include <algorithm>
#include <cctype>
#include <map>
#include <ctime>
#include <mutex>
#include <random>
#include <set>
#include <string>
#include <vector>

#include "sqlite3.h"

#include "src/ControlServer/Database/Database.hpp"
#include "src/ControlServer/Logger.hpp"
#include "src/ControlServer/PlayerSessionStore/PlayerSessionStore.hpp"  // SettleAvaProduction
#include "src/ControlServer/ChatSession/ChatSession.hpp"                // bid window results
#include "src/ControlServer/MailService/MailService.hpp"
#include "src/ControlServer/ServiceAgents/ServiceAgents.hpp"
#include <cstdio>
#include "src/ControlServer/TcpSession/TcpSession.hpp"

namespace AvaParity {

namespace {

constexpr int64_t kSoloOwnerBase = 0x40000000;

void EnsureSchema(sqlite3* db) {
	static std::once_flag once;
	std::call_once(once, [db] {
		sqlite3_exec(db,
			"CREATE TABLE IF NOT EXISTS ga_ava_lockdown (zone_id INTEGER NOT NULL, position INTEGER NOT NULL,"
			" locked_until INTEGER NOT NULL, reason TEXT NOT NULL DEFAULT '', PRIMARY KEY (zone_id, position));"
			"CREATE TABLE IF NOT EXISTS ga_ava_season_items (season INTEGER NOT NULL, owner_agency_id INTEGER NOT NULL,"
			" rank INTEGER NOT NULL, loot_table_id INTEGER NOT NULL, items INTEGER NOT NULL DEFAULT 0,"
			" granted_at INTEGER NOT NULL, PRIMARY KEY (season, owner_agency_id));"
			// Attack type of a launched attack (from the spent bid), season networth.
			"CREATE TABLE IF NOT EXISTS ga_ava_attack_types (character_id INTEGER PRIMARY KEY, position INTEGER NOT NULL,"
			" attack_type INTEGER NOT NULL, set_at INTEGER NOT NULL);"
			"CREATE TABLE IF NOT EXISTS ga_ava_networth (season INTEGER NOT NULL, owner_agency_id INTEGER NOT NULL,"
			" networth REAL NOT NULL DEFAULT 0, PRIMARY KEY (season, owner_agency_id));",
			nullptr, nullptr, nullptr);
		sqlite3_exec(db, "ALTER TABLE ga_ava_seasons ADD COLUMN networth REAL NOT NULL DEFAULT 0;",  // no-op error when the column already exists
			nullptr, nullptr, nullptr);
		sqlite3_exec(db, "ALTER TABLE ga_ava_seasons ADD COLUMN zone INTEGER NOT NULL DEFAULT 1;",  // p1140, same
			nullptr, nullptr, nullptr);
	});
}

std::string Setting(sqlite3* db, const char* key, const std::string& def) {
	std::string v = def;
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db, "SELECT value FROM cs_settings WHERE key = ?1", -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_text(s, 1, key, -1, SQLITE_STATIC);
		if (sqlite3_step(s) == SQLITE_ROW && sqlite3_column_text(s, 0))
			v = (const char*)sqlite3_column_text(s, 0);
		sqlite3_finalize(s);
	}
	return v;
}

double SettingNum(sqlite3* db, const char* key, double def) {
	const std::string v = Setting(db, key, "");
	if (v.empty()) return def;
	try { return std::stod(v); } catch (...) { return def; }
}

int64_t OwnerOf(sqlite3* db, uint32_t position) {
	int64_t owner = 0;
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db, "SELECT owner_agency_id FROM ga_ava_territories WHERE zone_id = 1 AND position = ?1",
	        -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int(s, 1, (int)position);
		if (sqlite3_step(s) == SQLITE_ROW) owner = sqlite3_column_int64(s, 0);
		sqlite3_finalize(s);
	}
	return owner;
}

uint32_t SubtypeOf(sqlite3* db, uint32_t building_id) {
	uint32_t sub = 0;
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db, "SELECT building_subtype_value_id FROM asm_data_set_hex_buildings WHERE hex_building_id = ?1",
	        -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int(s, 1, (int)building_id);
		if (sqlite3_step(s) == SQLITE_ROW) sub = (uint32_t)sqlite3_column_int(s, 0);
		sqlite3_finalize(s);
	}
	return sub;
}

// Rolls a loot table into an owner's AvA inventory (ga_ava_items). drop_chance <= 0 or >= 100
// = always (season/HQ tables), else a percent roll per row. Returns the number of item units.
// pick_one (theft tables, chances sum to 100): exactly one row, weighted by drop_chance.
int RollIntoAvaItems(sqlite3* db, int64_t owner, int loot_table_id, double* value = nullptr, bool pick_one = false) {
	static std::mt19937 rng{(unsigned)std::time(nullptr)};
	std::uniform_real_distribution<double> pct(0.0, 100.0);
	std::vector<std::pair<int, int>> got;
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db, "SELECT item_id, quantity, drop_chance FROM asm_data_set_loot_table_items "
	        "WHERE loot_table_id = ?1 AND item_id > 0 ORDER BY sort_order, id", -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int(s, 1, loot_table_id);
		std::vector<std::pair<double, std::pair<int, int>>> rows;
		while (sqlite3_step(s) == SQLITE_ROW) {
			const double chance = sqlite3_column_double(s, 2);
			const std::pair<int, int> row{sqlite3_column_int(s, 0), std::max(1, sqlite3_column_int(s, 1))};
			if (pick_one) { rows.push_back({std::max(0.0, chance), row}); continue; }
			if (chance > 0.0 && chance < 100.0 && pct(rng) >= chance) continue;
			got.push_back(row);
		}
		sqlite3_finalize(s);
		double sum = 0;
		for (const auto& r : rows) sum += r.first;
		if (pick_one && sum > 0) {
			double x = std::uniform_real_distribution<double>(0.0, sum)(rng);
			for (const auto& r : rows) { if (x < r.first) { got.push_back(r.second); break; } x -= r.first; }
			if (got.empty()) got.push_back(rows.back().second);
		}
	}
	int units = 0;
	for (const auto& [item, qty] : got) {
		if (sqlite3_prepare_v2(db, "INSERT INTO ga_ava_items (owner_agency_id, item_id, count) VALUES (?1, ?2, ?3) "
		        "ON CONFLICT(owner_agency_id, item_id) DO UPDATE SET count = count + ?3", -1, &s, nullptr) == SQLITE_OK) {
			sqlite3_bind_int64(s, 1, owner);
			sqlite3_bind_int(s, 2, item);
			sqlite3_bind_int(s, 3, qty);
			if (sqlite3_step(s) == SQLITE_DONE) units += qty;
			sqlite3_finalize(s);
		}
		if (value && sqlite3_prepare_v2(db, "SELECT production_cost FROM asm_data_set_items WHERE item_id = ?1",
		        -1, &s, nullptr) == SQLITE_OK) {
			sqlite3_bind_int(s, 1, item);
			if (sqlite3_step(s) == SQLITE_ROW) *value += sqlite3_column_double(s, 0) * qty;
			sqlite3_finalize(s);
		}
	}
	return units;
}


// Agency id of the character, or the solo owner id (0x40000000 + character id).
int64_t OwnerOfCharacter(sqlite3* db, int64_t character_id) {
	int64_t owner = 0;
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db, "SELECT COALESCE(m.agency_id, ?2 + c.id) FROM ga_characters c "
	        "LEFT JOIN ga_agency_members m ON m.user_id = c.user_id WHERE c.id = ?1", -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int64(s, 1, character_id);
		sqlite3_bind_int64(s, 2, kSoloOwnerBase);
		if (sqlite3_step(s) == SQLITE_ROW) owner = sqlite3_column_int64(s, 0);
		sqlite3_finalize(s);
	}
	return owner;
}

// Facility tech tier of a hex: 1-3 from tech_level 1335/1336/1337, HQ/base 1, none 0.
int TierAt(sqlite3* db, uint32_t pos) {
	const uint32_t b = AvaParityBuildingAt(pos);
	int t = 0;
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db, "SELECT tech_level_value_id, building_type_value_id FROM asm_data_set_hex_buildings "
	        "WHERE hex_building_id = ?1", -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int(s, 1, (int)b);
		if (sqlite3_step(s) == SQLITE_ROW) {
			const int tl = sqlite3_column_int(s, 0), type = sqlite3_column_int(s, 1);
			t = tl == 1337 ? 3 : tl == 1336 ? 2 : tl == 1335 ? 1 : type == 676 ? 1 : 0;  // 676 = HQ/base
		}
		sqlite3_finalize(s);
	}
	return t;
}

void AddNetworth(sqlite3* db, int64_t owner, uint32_t season, double delta) {
	sqlite3_stmt* s = nullptr;
	if (owner == 0 || delta == 0.0) return;
	if (sqlite3_prepare_v2(db, "INSERT INTO ga_ava_networth (season, owner_agency_id, networth) VALUES (?1, ?2, MAX(0, ?3)) "
	        "ON CONFLICT(season, owner_agency_id) DO UPDATE SET networth = MAX(0, networth + ?3)", -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int(s, 1, (int)season);
		sqlite3_bind_int64(s, 2, owner);
		sqlite3_bind_double(s, 3, delta);
		sqlite3_step(s);
		sqlite3_finalize(s);
	}
}

// Instance -> attackers whose win was a theft (filled by OnAvaResult, read by IsTheft).
std::mutex g_theft_mu;
std::map<int64_t, std::set<int64_t>> g_theft;

void DestroyFacility(sqlite3* db, uint32_t pos) {
	for (const char* q : {"DELETE FROM ga_ava_buildings WHERE zone_id = 1 AND position = ?1", "DELETE FROM ga_ava_mine_resources WHERE zone_id = 1 AND position = ?1",
	        "DELETE FROM ga_ava_hq WHERE zone_id = 1 AND position = ?1"}) {
		sqlite3_stmt* s = nullptr;
		if (sqlite3_prepare_v2(db, q, -1, &s, nullptr) != SQLITE_OK) continue;
		sqlite3_bind_int(s, 1, (int)pos);
		sqlite3_step(s);
		sqlite3_finalize(s);
	}
}

void ClearHexAssets(sqlite3* db, uint32_t pos) {
	static const char* kSql[] = {
		"DELETE FROM ga_ava_buildings WHERE zone_id = 1 AND position = ?1",
		"DELETE FROM ga_ava_mine_resources WHERE zone_id = 1 AND position = ?1",
		"DELETE FROM ga_ava_hq WHERE zone_id = 1 AND position = ?1",
		"DELETE FROM ga_ava_lockdown WHERE zone_id = 1 AND position = ?1",
	};
	for (const char* q : kSql) {
		sqlite3_stmt* s = nullptr;
		if (sqlite3_prepare_v2(db, q, -1, &s, nullptr) == SQLITE_OK) {
			sqlite3_bind_int(s, 1, (int)pos);
			sqlite3_step(s);
			sqlite3_finalize(s);
		}
	}
}

// Shared checks of give / abandon. Returns false (and fills r) when refused.
bool TradeAllowed(sqlite3* db, int64_t character_id, uint32_t pos, bool give, TradeResult& r, int64_t& owner) {
	owner = OwnerOf(db, pos);
	const int64_t me = OwnerOfCharacter(db, character_id);
	if (owner == 0 || owner != me) { r.msg_id = 17494; return false; }  // "Territory is not controlled by your agency."
	if (me < kSoloOwnerBase) {
		int level = 0, perms = -1;
		sqlite3_stmt* s = nullptr;
		if (sqlite3_prepare_v2(db, "SELECT COALESCE(r.rank_level, m.rank_id), COALESCE(r.permissions, 0) FROM ga_agency_members m "
		        "JOIN ga_characters c ON c.user_id = m.user_id LEFT JOIN ga_agency_ranks r ON r.agency_id = m.agency_id "
		        "AND r.rank_id = m.rank_id WHERE c.id = ?1", -1, &s, nullptr) == SQLITE_OK) {
			sqlite3_bind_int64(s, 1, character_id);
			if (sqlite3_step(s) == SQLITE_ROW) { level = sqlite3_column_int(s, 0); perms = sqlite3_column_int(s, 1); }
			sqlite3_finalize(s);
		}
		if (level != 0 && !(perms & 0x00020000)) { r.msg_id = 43914; return false; }  // leader or AGENCY_PERM_FACILITY_MGMT
	}
	int64_t secs = 0;
	if (Setting(db, "ava_zone_schedule", "always") == "original" && ZoneOpen((int64_t)std::time(nullptr), &secs)) {
		r.text = "AvA: territories can only be traded while the war zone is closed (closes in " +
			std::to_string(secs / 3600) + " h " + std::to_string((secs % 3600) / 60) + " min).";
		return false;
	}
	bool attacked = false;
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db, "SELECT 1 FROM ga_ava_attacks WHERE zone_id = 1 AND position = ?1 LIMIT 1",
	        -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int(s, 1, (int)pos);
		attacked = sqlite3_step(s) == SQLITE_ROW;
		sqlite3_finalize(s);
	}
	if (attacked) {
		if (give) r.text = "AvA: you cannot give away a territory that is under attack.";
		else r.msg_id = 52182;  // "You cannot abandon a territory that is under attack."
		return false;
	}
	return true;
}

std::string Lower(std::string v) {
	std::transform(v.begin(), v.end(), v.begin(), [](unsigned char c) { return (char)std::tolower(c); });
	return v;
}
}  // namespace

bool ZoneOpen(int64_t now, int64_t* secs_to_change) {
	if (secs_to_change) *secs_to_change = 0;
	sqlite3* db = Database::GetConnection();
	if (!db || Setting(db, "ava_zone_schedule", "always") != "original") return true;
	// 1.36 (news 4190/4391): NA zones open 8:30 PM - 1:30 AM ET; weekday zone Mon-Thu,
	// weekend zone Sat-Sun (no window starts on a Friday). One zone here = both windows.
	const int64_t off = (int64_t)(SettingNum(db, "ava_zone_utc_offset_hours", -5.0) * 3600.0);
	const int64_t start = (int64_t)(SettingNum(db, "ava_zone_open_minute", 20 * 60 + 30) * 60.0);
	const int64_t len = (int64_t)(SettingNum(db, "ava_zone_open_hours", 5.0) * 3600.0);
	const int64_t local = now + off;
	auto window_day_ok = [](int64_t day) {  // day index since 1970-01-01 (a Thursday)
		const int wday = (int)((day + 4) % 7);  // 0 = Sunday
		return wday != 5;                       // no window starts on a Friday
	};
	const int64_t day = local / 86400;
	for (int64_t d = day - 1; d <= day; ++d) {
		const int64_t ws = d * 86400 + start, we = ws + len;
		if (window_day_ok(d) && local >= ws && local < we) {
			if (secs_to_change) *secs_to_change = we - local;
			return true;
		}
	}
	for (int64_t d = day; d <= day + 8; ++d) {
		const int64_t ws = d * 86400 + start;
		if (window_day_ok(d) && ws > local) {
			if (secs_to_change) *secs_to_change = ws - local;
			break;
		}
	}
	return false;
}

bool AttackNeedsWonBid(sqlite3* db, int64_t character_id, uint32_t position, Gate& g);  // defined below
bool RegionGate(sqlite3* db, int64_t character_id, uint32_t position, Action action, Gate& g);  // defined below

Gate Check(int64_t character_id, uint32_t position, Action action) {
	Gate g;
	sqlite3* db = Database::GetConnection();
	if (!db) return g;
	EnsureSchema(db);
	ResolveBidWindows((int64_t)std::time(nullptr));
	if (Setting(db, "ava_solo_owners", "on") == "off") {
		bool member = false;
		sqlite3_stmt* s = nullptr;
		if (sqlite3_prepare_v2(db, "SELECT 1 FROM ga_agency_members m JOIN ga_characters c ON c.user_id = m.user_id "
		        "WHERE c.id = ?1", -1, &s, nullptr) == SQLITE_OK) {
			sqlite3_bind_int64(s, 1, character_id);
			member = sqlite3_step(s) == SQLITE_ROW;
			sqlite3_finalize(s);
		}
		if (!member) {
			g.refuse = true;
			g.text = "AvA: territory war is for agencies on this server - join or found an agency first.";
			Logger::Log("tcp", "[AvaParity] char %lld pos %u action %d refused: no agency (ava_solo_owners=off)\n",
				(long long)character_id, position, (int)action);
			return g;
		}
	}
	int64_t secs = 0;
	if (!ZoneOpen((int64_t)std::time(nullptr), &secs)) {
		g.refuse = true;
		g.msg_id = 30739;  // "The zone is not open for attack."
		g.text = "AvA: the war zone opens in " + std::to_string(secs / 3600) + " h " +
			std::to_string((secs % 3600) / 60) + " min.";
		Logger::Log("tcp", "[AvaParity] char %lld pos %u action %d refused: zone closed, opens in %llds\n",
			(long long)character_id, position, (int)action, (long long)secs);
		return g;
	}
	sqlite3_stmt* s = nullptr;
	int64_t until = 0;
	if (sqlite3_prepare_v2(db, "SELECT locked_until FROM ga_ava_lockdown WHERE zone_id = 1 AND position = ?1",
	        -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int(s, 1, (int)position);
		if (sqlite3_step(s) == SQLITE_ROW) until = sqlite3_column_int64(s, 0);
		sqlite3_finalize(s);
	}
	const int64_t now = (int64_t)std::time(nullptr);
	if (until > now) {
		g.refuse = true;
		g.msg_id = action == kBid ? 39922 : action == kDefend ? 17462 : 17408;
		Logger::Log("tcp", "[AvaParity] char %lld pos %u action %d refused: lockdown %llds left\n",
			(long long)character_id, position, (int)action, (long long)(until - now));
	}
	if (!g.refuse && action == kDefend && Setting(db, "ava_defend_needs_attack", "on") != "off") {
		// Defend only answers a live attack on this hex (client text 39919).
		bool attacked = false;
		if (sqlite3_prepare_v2(db, "SELECT 1 FROM ga_ava_attacks WHERE zone_id = 1 AND position = ?1 AND queued_at >= ?2 LIMIT 1",
		        -1, &s, nullptr) == SQLITE_OK) {
			sqlite3_bind_int(s, 1, (int)position);
			sqlite3_bind_int64(s, 2, now - 1800);
			attacked = sqlite3_step(s) == SQLITE_ROW;
			sqlite3_finalize(s);
		}
		if (!attacked) {
			g.refuse = true;
			g.msg_id = 39919;  // "The territory is not under attack."
			Logger::Log("tcp", "[AvaParity] char %lld pos %u defend refused: not under attack\n", (long long)character_id, position);
		}
	}
	if (!g.refuse && action != kDefend && OpenBidOwnerFor(position, character_id) == 0) {  // one zone per owner (client text 53753)
		sqlite3_stmt* z = nullptr;
		if (sqlite3_prepare_v2(db, "SELECT 1 FROM ga_ava_territories WHERE zone_id = 1 AND owner_agency_id = ?1 AND position / ?2 <> ?3 LIMIT 1",
		        -1, &z, nullptr) == SQLITE_OK) {
			sqlite3_bind_int64(z, 1, OwnerOfCharacter(db, character_id));
			sqlite3_bind_int(z, 2, (int)AvaZones::kStride);
			sqlite3_bind_int(z, 3, (int)(position / AvaZones::kStride));
			if (sqlite3_step(z) == SQLITE_ROW) { g.refuse = true; g.msg_id = 53753; }
			sqlite3_finalize(z);
		}
		if (g.refuse) return g;
	}
	if (!g.refuse && RegionGate(db, character_id, position, action, g)) return g;  // regions
	if (!g.refuse && action == kAttack && AttackNeedsWonBid(db, character_id, position, g)) return g;
	if (!g.refuse && action == kAttack) {
		// Remember the attack type of the attacker's own standing bid (spent on launch).
		uint32_t type = kAttackCapture;
		if (sqlite3_prepare_v2(db, "SELECT attack_type FROM ga_ava_bids WHERE zone_id = 1 AND position = ?1 "
		        "AND owner_agency_id = ?2", -1, &s, nullptr) == SQLITE_OK) {
			sqlite3_bind_int(s, 1, (int)position);
			sqlite3_bind_int64(s, 2, OwnerOfCharacter(db, character_id));
			if (sqlite3_step(s) == SQLITE_ROW && sqlite3_column_int(s, 0) == (int)kAttackTheft) type = kAttackTheft;
			sqlite3_finalize(s);
		}
		if (sqlite3_prepare_v2(db, "INSERT OR REPLACE INTO ga_ava_attack_types (character_id, position, attack_type, set_at) "
		        "VALUES (?1, ?2, ?3, ?4)", -1, &s, nullptr) == SQLITE_OK) {
			sqlite3_bind_int64(s, 1, character_id);
			sqlite3_bind_int(s, 2, (int)position);
			sqlite3_bind_int(s, 3, (int)type);
			sqlite3_bind_int64(s, 4, now);
			sqlite3_step(s);
			sqlite3_finalize(s);
		}
		if (type == kAttackTheft)
			Logger::Log("tcp", "[AvaParity] char %lld pos %u: theft attack (bid type 2)\n", (long long)character_id, position);
	}
	return g;
}

void OnAvaResult(int64_t instance_id, int winning_task_force) {
	sqlite3* db = Database::GetConnection();
	if (!db || instance_id == 0) return;
	EnsureSchema(db);
	std::set<uint32_t> positions;
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db, "SELECT DISTINCT a.position FROM ga_ava_attacks a JOIN ga_instance_players ip "
	        "ON ip.character_id = a.character_id WHERE ip.instance_id = ?1 AND ip.task_force_number = 1 AND a.zone_id = 1",
	        -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int64(s, 1, instance_id);
		while (sqlite3_step(s) == SQLITE_ROW) positions.insert((uint32_t)sqlite3_column_int(s, 0));
		sqlite3_finalize(s);
	}
	const int64_t now = (int64_t)std::time(nullptr);
	{
		std::lock_guard<std::mutex> lk(g_theft_mu);
		while (g_theft.size() > 16) g_theft.erase(g_theft.begin());
		g_theft.erase(instance_id);
	}
	for (uint32_t pos : positions) {
		const int64_t owner = OwnerOf(db, pos);
		const uint32_t building = AvaParityBuildingAt(pos);
		const uint32_t sub = SubtypeOf(db, building);
		const bool production = sub == 1320 || sub == 1321 || sub == 1322;
		// Theft attackers of this hex (their bid said Steal); no theft on an empty hex.
		std::vector<int64_t> thieves;
		if (owner != 0 && Setting(db, "ava_theft", "on") != "off" &&
		    sqlite3_prepare_v2(db, "SELECT a.character_id FROM ga_ava_attacks a JOIN ga_instance_players ip "
		        "ON ip.character_id = a.character_id JOIN ga_ava_attack_types t ON t.character_id = a.character_id "
		        "AND t.position = a.position WHERE ip.instance_id = ?1 AND ip.task_force_number = 1 AND a.zone_id = 1 "
		        "AND a.position = ?2 AND t.attack_type = ?3 GROUP BY a.character_id", -1, &s, nullptr) == SQLITE_OK) {
			sqlite3_bind_int64(s, 1, instance_id);
			sqlite3_bind_int(s, 2, (int)pos);
			sqlite3_bind_int(s, 3, (int)kAttackTheft);
			while (sqlite3_step(s) == SQLITE_ROW) thieves.push_back(sqlite3_column_int64(s, 0));
			sqlite3_finalize(s);
		}
		int64_t lock = 0;
		const char* why = "";
		std::string theft_note;
		if (winning_task_force == 1 && !thieves.empty()) {
			lock = production ? 600 : 120;
			why = "thefted";
			{
				std::lock_guard<std::mutex> lk(g_theft_mu);
				g_theft[instance_id].insert(thieves.begin(), thieves.end());
			}
			PlayerSessionStore::SettleAvaProduction();  // the defender keeps what it produced so far
			const int tier = TierAt(db, pos);
			const int table = tier >= 3 ? 254 : tier == 2 ? 255 : 252;  // *AvA_Theft_Items_Tech1/2/3
			const int64_t thief_owner = OwnerOfCharacter(db, thieves.front());
			double value = 0;
			const int units = RollIntoAvaItems(db, thief_owner, table, &value, true);
			const double cut = 3.0 * value;  // 1.36: theft cuts the defender's networth 3x
			AddNetworth(db, owner, CurrentSeason(now), -cut);
			if (sqlite3_prepare_v2(db, "UPDATE ga_ava_territories SET settled_at = ?2 WHERE zone_id = 1 AND position = ?1",
			        -1, &s, nullptr) == SQLITE_OK) {  // production stops 2 ticks (2 h here)
				sqlite3_bind_int(s, 1, (int)pos);
				sqlite3_bind_int64(s, 2, now + 2 * 3600);
				sqlite3_step(s);
				sqlite3_finalize(s);
			}
			theft_note = " THEFT by owner " + std::to_string(thief_owner) + ": table " + std::to_string(table) + " +" +
				std::to_string(units) + " units (value " + std::to_string((long long)value) + "), defender networth -" +
				std::to_string((long long)cut) + ", production paused 2 h";
		} else if (winning_task_force == 1) {
			if (owner != 0) { lock = 120; why = "captured"; }  // empty claim: no lockdown
			DestroyFacility(db, pos);
		} else if (owner != 0) {
			lock = production ? 600 : 120;
			why = production ? "defended production" : "defended";
		}
		if (lock > 0 && sqlite3_prepare_v2(db, "INSERT OR REPLACE INTO ga_ava_lockdown (zone_id, position, locked_until, reason) "
		        "VALUES (1, ?1, ?2, ?3)", -1, &s, nullptr) == SQLITE_OK) {
			sqlite3_bind_int(s, 1, (int)pos);
			sqlite3_bind_int64(s, 2, now + lock);
			sqlite3_bind_text(s, 3, why, -1, SQLITE_STATIC);
			sqlite3_step(s);
			sqlite3_finalize(s);
		}
		// HQ bonus cargo: the owner's HQ held against a real attack.
		int units = 0;
		if (winning_task_force == 2 && owner != 0) {
			bool hq = false;
			if (sqlite3_prepare_v2(db, "SELECT 1 FROM ga_ava_hq WHERE owner_agency_id = ?1 AND zone_id = 1 AND position = ?2",
			        -1, &s, nullptr) == SQLITE_OK) {
				sqlite3_bind_int64(s, 1, owner);
				sqlite3_bind_int(s, 2, (int)pos);
				hq = sqlite3_step(s) == SQLITE_ROW;
				sqlite3_finalize(s);
			}
			if (hq && HqProgress(pos) < 100) {
				hq = false;
				theft_note = " (HQ still building: " + std::to_string(HqProgress(pos)) + "%, no cargo)";
			}
			if (hq) units = RollIntoAvaItems(db, owner, 136);
		}
		{  // Combat Log row: the first attacker names the attacking side
			sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS ga_ava_combat_log (id INTEGER PRIMARY KEY AUTOINCREMENT, at INTEGER NOT NULL, "
				"position INTEGER NOT NULL, building INTEGER NOT NULL, owner TEXT NOT NULL, attacker TEXT NOT NULL, outcome INTEGER NOT NULL)", nullptr, nullptr, nullptr);
			int64_t attacker_owner = 0;
			if (sqlite3_prepare_v2(db, "SELECT a.character_id, a.on_behalf_of FROM ga_ava_attacks a JOIN ga_instance_players ip ON ip.character_id = a.character_id "
			        "WHERE ip.instance_id = ?1 AND ip.task_force_number = 1 AND a.position = ?2 LIMIT 1", -1, &s, nullptr) == SQLITE_OK) {
				sqlite3_bind_int64(s, 1, instance_id);
				sqlite3_bind_int(s, 2, (int)pos);
				if (sqlite3_step(s) == SQLITE_ROW)
					attacker_owner = sqlite3_column_int64(s, 1) ? sqlite3_column_int64(s, 1) : OwnerOfCharacter(db, sqlite3_column_int64(s, 0));
				sqlite3_finalize(s);
			}
			const uint32_t outcome = winning_task_force != 1 ? 1357 : !thieves.empty() ? 1466 : 1355;
			if (sqlite3_prepare_v2(db, "INSERT INTO ga_ava_combat_log (at, position, building, owner, attacker, outcome) VALUES (?1, ?2, ?3, ?4, ?5, ?6)", -1, &s, nullptr) == SQLITE_OK) {
				sqlite3_bind_int64(s, 1, now);
				sqlite3_bind_int(s, 2, (int)pos);
				sqlite3_bind_int(s, 3, (int)building);
				const std::string owner_name = owner ? OwnerName(owner) : "", attacker_name = attacker_owner ? OwnerName(attacker_owner) : "";
				sqlite3_bind_text(s, 4, owner_name.c_str(), -1, SQLITE_TRANSIENT);
				sqlite3_bind_text(s, 5, attacker_name.c_str(), -1, SQLITE_TRANSIENT);
				sqlite3_bind_int(s, 6, (int)outcome);
				sqlite3_step(s);
				sqlite3_finalize(s);
			}
		}
		Logger::Log("rewards", "[AvaParity] instance=%lld pos %u owner %lld building %u winner_tf=%d -> lockdown %llds (%s)%s%s\n",
			(long long)instance_id, pos, (long long)owner, building, winning_task_force, (long long)lock, why,
			units ? (" HQ cargo table 136 +" + std::to_string(units)).c_str() : "", theft_note.c_str());
	}
}

std::vector<std::string> AttackAlertRecipients(uint32_t position, int64_t attacker_character_id) {
	std::vector<std::string> names;
	sqlite3* db = Database::GetConnection();
	if (!db) return names;
	const int64_t owner = OwnerOf(db, position);
	if (owner == 0) return names;
	sqlite3_stmt* s = nullptr;
	const char* sql = owner >= kSoloOwnerBase
		? "SELECT u.username FROM ga_characters c JOIN ga_users u ON u.id = c.user_id WHERE c.id = ?1 - ?3 "
		  "AND c.user_id <> (SELECT user_id FROM ga_characters WHERE id = ?2)"
		: "SELECT u.username FROM ga_agency_members m JOIN ga_users u ON u.id = m.user_id WHERE m.agency_id = ?1 "
		  "AND NOT EXISTS (SELECT 1 FROM ga_agency_members x JOIN ga_characters c ON c.user_id = x.user_id "
		  "WHERE c.id = ?2 AND x.agency_id = ?1)";
	if (sqlite3_prepare_v2(db, sql, -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int64(s, 1, owner);
		sqlite3_bind_int64(s, 2, attacker_character_id);
		if (owner >= kSoloOwnerBase) sqlite3_bind_int64(s, 3, kSoloOwnerBase);
		while (sqlite3_step(s) == SQLITE_ROW)
			if (const char* n = (const char*)sqlite3_column_text(s, 0)) names.emplace_back(n);
		sqlite3_finalize(s);
	}
	if (!names.empty())
		Logger::Log("tcp", "[AvaParity] pos %u attacked by char %lld -> alert %zu member(s) of owner %lld\n",
			position, (long long)attacker_character_id, names.size(), (long long)owner);
	return names;
}

int TechTier(int64_t instance_id) {
	sqlite3* db = Database::GetConnection();
	if (!db || instance_id == 0) return 0;
	int tier = 0;
	sqlite3_stmt* s = nullptr;
	std::vector<uint32_t> positions;
	if (sqlite3_prepare_v2(db, "SELECT DISTINCT a.position FROM ga_ava_attacks a JOIN ga_instance_players ip "
	        "ON ip.character_id = a.character_id WHERE ip.instance_id = ?1 AND ip.task_force_number = 1 AND a.zone_id = 1",
	        -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int64(s, 1, instance_id);
		while (sqlite3_step(s) == SQLITE_ROW) positions.push_back((uint32_t)sqlite3_column_int(s, 0));
		sqlite3_finalize(s);
	}
	for (uint32_t pos : positions) tier = std::max(tier, TierAt(db, pos));
	return tier;
}

std::map<int64_t, double> SoftCapFactors(sqlite3* db) {
	std::map<int64_t, double> f;
	if (!db || Setting(db, "ava_softcap", "on") == "off") return f;
	const double freeN = SettingNum(db, "ava_softcap_free", 5.0);
	const double peak = SettingNum(db, "ava_softcap_peak", 13.0);
	if (peak <= freeN) return f;
	const double k = 1.0 / (2.0 * (peak - freeN));
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db, "SELECT owner_agency_id, COUNT(*) FROM ga_ava_territories WHERE zone_id = 1 GROUP BY 1",
	        -1, &s, nullptr) == SQLITE_OK) {
		while (sqlite3_step(s) == SQLITE_ROW) {
			const double n = sqlite3_column_double(s, 1);
			if (n <= freeN) continue;
			const double total = n - k * (n - freeN) * (n - freeN);
			f[sqlite3_column_int64(s, 0)] = std::max(0.1, total / n);
		}
		sqlite3_finalize(s);
	}
	return f;
}

int64_t SeasonMemberSeconds() {
	sqlite3* db = Database::GetConnection();
	if (!db) return 0;
	return (int64_t)(std::max(0.0, SettingNum(db, "ava_season_member_days", 7.0)) * 86400.0);
}

void GrantSeasonItems(int season) {
	sqlite3* db = Database::GetConnection();
	if (!db) return;
	EnsureSchema(db);
	const std::string set = Setting(db, "ava_season_reward_set", "WD");
	if (set == "off") return;
	const bool we = set == "WE";
	// 1.39 brackets: Winner, Runner-up, Honorable mention (top 10). *_1 tables = first zone.
	const int winner = we ? 461 : 342, runner = we ? 463 : 456, mention = we ? 465 : 460;
	std::vector<std::pair<int64_t, std::string>> top;  // top 10 of each zone (each zone has its own winners)
	std::vector<size_t> ranks;
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db, "SELECT owner_agency_id, owner_name, zone FROM ga_ava_seasons WHERE season = ?1 "
	        "AND territories > 0 ORDER BY zone, networth DESC, territories DESC, owner_agency_id", -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int(s, 1, season);
		int zone = -1;
		size_t in_zone = 0;
		while (sqlite3_step(s) == SQLITE_ROW) {
			const char* n = (const char*)sqlite3_column_text(s, 1);
			if (sqlite3_column_int(s, 2) != zone) { zone = sqlite3_column_int(s, 2); in_zone = 0; }
			if (in_zone < 10) { top.emplace_back(sqlite3_column_int64(s, 0), n ? n : "?"); ranks.push_back(in_zone); }
			++in_zone;
		}
		sqlite3_finalize(s);
	}
	for (size_t i = 0; i < top.size(); ++i) {
		const size_t rank = ranks[i];
		const int table = rank == 0 ? winner : rank == 1 ? runner : mention;
		const int64_t owner = top[i].first;
		if (sqlite3_prepare_v2(db, "INSERT OR IGNORE INTO ga_ava_season_items (season, owner_agency_id, rank, loot_table_id, granted_at) "
		        "VALUES (?1, ?2, ?3, ?4, strftime('%s','now'))", -1, &s, nullptr) != SQLITE_OK) continue;
		sqlite3_bind_int(s, 1, season);
		sqlite3_bind_int64(s, 2, owner);
		sqlite3_bind_int(s, 3, (int)rank + 1);
		sqlite3_bind_int(s, 4, table);
		sqlite3_step(s);
		sqlite3_finalize(s);
		if (sqlite3_changes(db) == 0) continue;  // already granted
		const int units = RollIntoAvaItems(db, owner, table);
		if (sqlite3_prepare_v2(db, "UPDATE ga_ava_season_items SET items = ?3 WHERE season = ?1 AND owner_agency_id = ?2",
		        -1, &s, nullptr) == SQLITE_OK) {
			sqlite3_bind_int(s, 1, season);
			sqlite3_bind_int64(s, 2, owner);
			sqlite3_bind_int(s, 3, units);
			sqlite3_step(s);
			sqlite3_finalize(s);
		}
		Logger::Log("rewards", "[AvaParity] AvA season %d rank %zu %s (owner %lld%s): table %d -> %d AvA item units\n",
			season, rank + 1, top[i].second.c_str(), (long long)owner, owner >= kSoloOwnerBase ? ", solo" : "",
			table, units);
	}
}


bool IsTheft(int64_t instance_id, int64_t character_id) {
	std::lock_guard<std::mutex> lk(g_theft_mu);
	const auto it = g_theft.find(instance_id);
	return it != g_theft.end() && it->second.count(character_id) > 0;
}

TradeResult GiveTerritory(int64_t character_id, uint32_t position, const std::string& target,
	const std::vector<std::string>& online_names) {
	TradeResult r;
	sqlite3* db = Database::GetConnection();
	if (!db) return r;
	EnsureSchema(db);
	int64_t owner = 0;
	if (!TradeAllowed(db, character_id, position, true, r, owner)) return r;
	// Target: an online agent (by account name); its agency, or itself as a solo owner.
	std::string name;
	for (const auto& n : online_names) if (Lower(n) == Lower(target)) name = n;
	if (name.empty() && !target.empty() && PlayerSessionStore::GetByPlayerName(target)) name = target;  // logged in, chat not up yet
	if (name.empty()) { r.msg_id = 43915; return r; }  // "Player to transfer territory to is not found or is not online."
	int64_t to = 0;
	std::string to_name;
	double cr = 0.9, cg = 0.6, cb = 0.1;
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db, "SELECT a.id, a.name, a.color_r, a.color_g, a.color_b, u.username, "
	        "(SELECT MAX(c.id) FROM ga_characters c WHERE c.user_id = u.id AND c.deleted_at = 0) "
	        "FROM ga_users u LEFT JOIN ga_agency_members m ON m.user_id = u.id LEFT JOIN ga_agencies a ON a.id = m.agency_id "
	        "WHERE u.username = ?1 COLLATE NOCASE", -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_text(s, 1, name.c_str(), -1, SQLITE_TRANSIENT);
		if (sqlite3_step(s) == SQLITE_ROW) {
			if (sqlite3_column_type(s, 0) != SQLITE_NULL) {
				to = sqlite3_column_int64(s, 0);
				if (const char* n = (const char*)sqlite3_column_text(s, 1)) to_name = n;
				cr = sqlite3_column_double(s, 2); cg = sqlite3_column_double(s, 3); cb = sqlite3_column_double(s, 4);
			} else if (Setting(db, "ava_solo_owners", "on") != "off" && sqlite3_column_type(s, 6) != SQLITE_NULL) {
				to = kSoloOwnerBase + sqlite3_column_int64(s, 6);
				if (const char* n = (const char*)sqlite3_column_text(s, 5)) to_name = n;
			} else {
				r.msg_id = 59835;  // "Player to transfer territory to is not a member of an agency."
			}
		} else {
			r.msg_id = 43915;
		}
		sqlite3_finalize(s);
	}
	if (to == 0) { if (!r.msg_id) r.msg_id = 43915; return r; }
	if (to == owner) { r.text = "AvA: " + name + " already holds this territory with you."; return r; }
	PlayerSessionStore::SettleAvaProduction();  // the giver keeps what it produced so far
	const int64_t now = (int64_t)std::time(nullptr);
	ClearHexAssets(db, position);  // the facility is destroyed on a transfer (wiki "Hex Trading")
	if (sqlite3_prepare_v2(db, "UPDATE ga_ava_territories SET owner_agency_id = ?2, owner_name = ?3, color_r = ?4, "
	        "color_g = ?5, color_b = ?6, captured_at = ?7, settled_at = ?7 WHERE zone_id = 1 AND position = ?1",
	        -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int(s, 1, (int)position);
		sqlite3_bind_int64(s, 2, to);
		sqlite3_bind_text(s, 3, to_name.c_str(), -1, SQLITE_TRANSIENT);
		sqlite3_bind_double(s, 4, cr);
		sqlite3_bind_double(s, 5, cg);
		sqlite3_bind_double(s, 6, cb);
		sqlite3_bind_int64(s, 7, now);
		r.ok = sqlite3_step(s) == SQLITE_DONE && sqlite3_changes(db) == 1;
		sqlite3_finalize(s);
	}
	if (!r.ok) { r.text = "AvA: the transfer failed."; return r; }
	std::string giver = "?";
	if (sqlite3_prepare_v2(db, "SELECT u.username FROM ga_characters c JOIN ga_users u ON u.id = c.user_id WHERE c.id = ?1",
	        -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int64(s, 1, character_id);
		if (sqlite3_step(s) == SQLITE_ROW && sqlite3_column_text(s, 0)) giver = (const char*)sqlite3_column_text(s, 0);
		sqlite3_finalize(s);
	}
	r.text = "AvA: territory " + AvaZones::Label(position) + " now belongs to " + to_name + " (its facility was dismantled).";
	r.notify_name = name;
	// client text 43889 "Player '@@player_name@@' is transferring ownership of a territory to you."
	r.notify_text = "Player '" + giver + "' transferred ownership of territory " + AvaZones::Label(position) +
		" to you (" + to_name + ").";
	Logger::Log("tcp", "[AvaParity] HEX_GIVE_OWNERSHIP pos %u owner %lld -> %lld (%s, target %s) by char %lld\n",
		position, (long long)owner, (long long)to, to_name.c_str(), name.c_str(), (long long)character_id);
	return r;
}

TradeResult AbandonTerritory(int64_t character_id, uint32_t position) {
	TradeResult r;
	sqlite3* db = Database::GetConnection();
	if (!db) return r;
	EnsureSchema(db);
	int64_t owner = 0;
	if (!TradeAllowed(db, character_id, position, false, r, owner)) return r;
	PlayerSessionStore::SettleAvaProduction();
	ClearHexAssets(db, position);
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db, "DELETE FROM ga_ava_territories WHERE zone_id = 1 AND position = ?1",
	        -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int(s, 1, (int)position);
		r.ok = sqlite3_step(s) == SQLITE_DONE && sqlite3_changes(db) == 1;
		sqlite3_finalize(s);
	}
	r.text = r.ok ? "AvA: territory " + AvaZones::Label(position) + " abandoned; its assets are forfeit."
	              : "AvA: the territory could not be abandoned.";
	Logger::Log("tcp", "[AvaParity] HEX_ABANDON pos %u owner %lld by char %lld -> %s\n",
		position, (long long)owner, (long long)character_id, r.ok ? "ok" : "failed");
	return r;
}

int64_t SeasonLength() {
	sqlite3* db = Database::GetConnection();
	const double d = db ? SettingNum(db, "ava_season_days", 14) : 14;
	return (int64_t)std::clamp(d, 1.0, 60.0) * 86400;
}

uint32_t CurrentSeason(int64_t now) {  // = TcpSession ava_season()
	return (uint32_t)((now > kSeasonEpoch ? now - kSeasonEpoch : 0) / SeasonLength() + 1);
}

void SeasonEnd(int season, int64_t from, int64_t to) {
	sqlite3* db = Database::GetConnection();
	if (!db) return;
	EnsureSchema(db);
	constexpr int64_t kSolo = 0x40000000;
	// 1.37 (WIKI-Patch_1.37): 1 token per 2 networth, max 15,000, to the active members (same rule as the AP payout).
	std::vector<std::pair<int64_t, double>> owners;
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db, "SELECT owner_agency_id, networth FROM ga_ava_seasons WHERE season = ?1 AND networth >= 2",
	        -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int(s, 1, season);
		while (sqlite3_step(s) == SQLITE_ROW) owners.emplace_back(sqlite3_column_int64(s, 0), sqlite3_column_double(s, 1));
		sqlite3_finalize(s);
	}
	for (const auto& [owner, worth] : owners) {
		const int64_t tokens = std::min<int64_t>(15000, (int64_t)(worth / 2));
		const char* sql = owner >= kSolo
			? "SELECT ip.character_id FROM ga_instance_players ip JOIN ga_characters c ON c.id = ip.character_id AND c.deleted_at = 0 "
			  "WHERE ip.character_id = ?1 - ?4 AND ip.joined_at >= ?2 AND ip.joined_at < ?3 LIMIT 1"
			: "SELECT c.id, MAX(ip.joined_at) FROM ga_agency_members m JOIN ga_characters c ON c.user_id = m.user_id AND c.deleted_at = 0 "
			  "JOIN ga_instance_players ip ON ip.character_id = c.id "
			  "WHERE m.agency_id = ?1 AND ip.joined_at >= ?2 AND ip.joined_at < ?3 AND m.joined_at <= ?3 - ?5 GROUP BY m.user_id";
		std::vector<int64_t> chars;
		if (sqlite3_prepare_v2(db, sql, -1, &s, nullptr) != SQLITE_OK) continue;
		sqlite3_bind_int64(s, 1, owner); sqlite3_bind_int64(s, 2, from); sqlite3_bind_int64(s, 3, to);
		if (owner >= kSolo) sqlite3_bind_int64(s, 4, kSolo); else sqlite3_bind_int64(s, 5, SeasonMemberSeconds());
		while (sqlite3_step(s) == SQLITE_ROW) chars.push_back(sqlite3_column_int64(s, 0));
		sqlite3_finalize(s);
		for (int64_t cid : chars) {
			const int64_t paid = PlayerSessionStore::LimitTokenGain(cid, tokens);
			if (paid > 0 && sqlite3_prepare_v2(db, "UPDATE ga_characters SET tokens = tokens + ?1 WHERE id = ?2 AND deleted_at = 0",
			        -1, &s, nullptr) == SQLITE_OK) {
				sqlite3_bind_int64(s, 1, paid); sqlite3_bind_int64(s, 2, cid); sqlite3_step(s); sqlite3_finalize(s);
			}
			Logger::Log("rewards", "[AvaParity] season %d networth %.0f owner %lld char %lld -> +%lld tokens (of %lld)\n",
				season, worth, (long long)owner, (long long)cid, (long long)paid, (long long)tokens);
		}
	}
	if (Lower(Setting(db, "ava_season_wipe", "on")) != "on") return;
	// wiki AVA "Seasons": "the zone is wiped clean, networth is reset and all hexes are reverted to unclaimed status"
	static const char* kWipe[] = {
		"DELETE FROM ga_ava_territories WHERE zone_id = 1", "DELETE FROM ga_ava_buildings WHERE zone_id = 1",
		"DELETE FROM ga_ava_hq WHERE zone_id = 1", "DELETE FROM ga_ava_shields WHERE zone_id = 1",
		"DELETE FROM ga_ava_mine_resources WHERE zone_id = 1", "DELETE FROM ga_ava_bids WHERE zone_id = 1",
		"DELETE FROM ga_ava_sealed_bids", "DELETE FROM ga_ava_bid_windows", "DELETE FROM ga_ava_lockdown WHERE zone_id = 1",
		"DELETE FROM ga_ava_shield_breaches WHERE zone_id = 1", "DELETE FROM ga_ava_missile_picks",
		// non-combat items go with the season, combat items (type 1298) stay with the agency
		"DELETE FROM ga_ava_resources", "DELETE FROM ga_ava_stock",
		"DELETE FROM ga_ava_items WHERE item_id IN (SELECT item_id FROM asm_data_set_items WHERE item_type_value_id != 1298)",
		"DELETE FROM ga_players_inventory WHERE item_id IN (SELECT item_id FROM asm_data_set_items WHERE item_type_value_id = 1312) "
		"OR item_id IN (SELECT item_id FROM asm_data_set_loot_table_items WHERE loot_table_id IN (153, 154, 155))"};
	int rows = 0;
	sqlite3_exec(db, "BEGIN", nullptr, nullptr, nullptr);
	for (const char* q : kWipe)
		if (sqlite3_exec(db, q, nullptr, nullptr, nullptr) == SQLITE_OK) rows += sqlite3_changes(db);
	sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr);
	Logger::Log("rewards", "[AvaParity] season %d ended: War Zone wiped (%d rows), every hex unclaimed\n", season, rows);
}

void BookNetworth(int64_t owner, double delta) {
	sqlite3* db = Database::GetConnection();
	if (!db) return;
	EnsureSchema(db);
	const uint32_t season = CurrentSeason((int64_t)std::time(nullptr));
	AddNetworth(db, owner, season, delta);
	Logger::Log("tcp", "[AvaParity] networth owner %lld season %u %+.0f\n", (long long)owner, season, delta);
}

void SnapshotNetworth(sqlite3* db, uint32_t season) {
	if (!db) return;
	EnsureSchema(db);
	const bool on = Setting(db, "ava_rank_by", "networth") != "territories";
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db, on
	        ? "UPDATE ga_ava_seasons SET networth = COALESCE((SELECT n.networth FROM ga_ava_networth n WHERE n.season = "
	          "ga_ava_seasons.season AND n.owner_agency_id = ga_ava_seasons.owner_agency_id), 0) WHERE season = ?1"
	        : "UPDATE ga_ava_seasons SET networth = 0 WHERE season = ?1", -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int(s, 1, (int)season);
		sqlite3_step(s);
		sqlite3_finalize(s);
	}
}

double Networth(int64_t owner, uint32_t season) {
	sqlite3* db = Database::GetConnection();
	if (!db) return 0;
	EnsureSchema(db);
	if (Setting(db, "ava_rank_by", "networth") == "territories") return 0;
	double v = 0;
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db, "SELECT networth FROM ga_ava_networth WHERE season = ?1 AND owner_agency_id = ?2",
	        -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int(s, 1, (int)season);
		sqlite3_bind_int64(s, 2, owner);
		if (sqlite3_step(s) == SQLITE_ROW) v = sqlite3_column_double(s, 0);
		sqlite3_finalize(s);
	}
	return v;
}

int HqProgress(uint32_t position) {
	sqlite3* db = Database::GetConnection();
	if (!db) return -1;
	int64_t set_at = -1;
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db, "SELECT h.set_at FROM ga_ava_hq h JOIN ga_ava_territories t ON t.zone_id = h.zone_id "
	        "AND t.position = h.position AND t.owner_agency_id = h.owner_agency_id WHERE h.zone_id = 1 AND h.position = ?1",
	        -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int(s, 1, (int)position);
		if (sqlite3_step(s) == SQLITE_ROW) set_at = sqlite3_column_int64(s, 0);
		sqlite3_finalize(s);
	}
	if (set_at < 0) return -1;
	const double mins = std::max(0.0, SettingNum(db, "ava_hq_build_minutes", 120.0));
	if (mins <= 0.0) return 100;
	const double done = (double)((int64_t)std::time(nullptr) - set_at) / (mins * 60.0);
	return (int)std::clamp(done * 100.0, 0.0, 100.0);
}

// ---------------------------------------------------------------------------------------------
// Bidding (wiki AVA "Bidding", news 4190/4559). Original: sealed (blind) credit
// bids, an agency may raise its own bid every 15 s (client text 40885), bidding closes about
// 4 min after the first bid at a hidden random moment, the highest bid wins (36326 "You won the
// auction to attack the territory."), losers are outbid (41219). An unclaimed hex goes to the
// winner at once (40713). One bid per agency at a time (36570); no bids on a hex under attack
// (41217). cs_settings: ava_bid_window original (default) | instant (= our old standing top bid,
// launch at once); ava_bid_window_seconds 240, ava_bid_window_jitter 60, ava_rebid_seconds 15,
// ava_won_bid_minutes 30 (ours: a won bid not launched in time is refunded by mail).
// Cancelling a sealed bid forfeits it (client confirm text 40906). Results are resolved lazily
// (any bid/attack/zone map request + the 5-min season tick), so no new timer.
namespace {

void EnsureBidSchema(sqlite3* db) {
	static std::once_flag once;
	std::call_once(once, [db] {
		sqlite3_exec(db,
			"CREATE TABLE IF NOT EXISTS ga_ava_bid_windows (position INTEGER PRIMARY KEY, opened_at INTEGER NOT NULL,"
			" closes_at INTEGER NOT NULL);"
			"CREATE TABLE IF NOT EXISTS ga_ava_sealed_bids (position INTEGER NOT NULL, owner_agency_id INTEGER NOT NULL,"
			" character_id INTEGER NOT NULL, amount INTEGER NOT NULL, attack_type INTEGER NOT NULL DEFAULT 1,"
			" placed_at INTEGER NOT NULL, PRIMARY KEY (position, owner_agency_id));",
			nullptr, nullptr, nullptr);
	});
}

bool WindowMode(sqlite3* db) { return Lower(Setting(db, "ava_bid_window", "original")) != "instant"; }

std::string AccountOf(sqlite3* db, int64_t character_id) {
	std::string n;
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db, "SELECT u.username FROM ga_characters c JOIN ga_users u ON u.id = c.user_id WHERE c.id = ?1",
	        -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int64(s, 1, character_id);
		if (sqlite3_step(s) == SQLITE_ROW && sqlite3_column_text(s, 0)) n = (const char*)sqlite3_column_text(s, 0);
		sqlite3_finalize(s);
	}
	return n;
}

int64_t Scalar(sqlite3* db, const char* sql, int64_t a, int64_t b = 0, int64_t def = -1) {
	int64_t v = def;
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db, sql, -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int64(s, 1, a);
		sqlite3_bind_int64(s, 2, b);
		if (sqlite3_step(s) == SQLITE_ROW && sqlite3_column_type(s, 0) != SQLITE_NULL) v = sqlite3_column_int64(s, 0);
		sqlite3_finalize(s);
	}
	return v;
}

bool Exec2(sqlite3* db, const char* sql, int64_t a, int64_t b = 0, int64_t c = 0) {
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db, sql, -1, &s, nullptr) != SQLITE_OK) return false;
	sqlite3_bind_int64(s, 1, a);
	sqlite3_bind_int64(s, 2, b);
	sqlite3_bind_int64(s, 3, c);
	const bool ok = sqlite3_step(s) == SQLITE_DONE;
	sqlite3_finalize(s);
	return ok;
}

void Refund(sqlite3* db, int64_t character_id, uint32_t pos, int64_t amount, const std::string& why,
	std::vector<int64_t>& mailed) {
	const std::string body = "Your bid of " + std::to_string(amount) + " credits on AvA territory " +
		AvaZones::Label(pos) + " was returned: " + why;
	if (MailService::DeliverFromAuction(0, character_id, "AvA bid refund - territory " + AvaZones::Label(pos), body, amount))
		mailed.push_back(character_id);
	(void)db;
}

}  // namespace

bool BidWindowOpen(uint32_t position, int64_t* secs_left) {
	sqlite3* db = Database::GetConnection();
	if (!db) return false;
	EnsureBidSchema(db);
	const int64_t closes = Scalar(db, "SELECT closes_at FROM ga_ava_bid_windows WHERE position = ?1", position);
	if (closes < 0) return false;
	if (secs_left) *secs_left = std::max<int64_t>(0, closes - (int64_t)std::time(nullptr));
	return true;
}

void ResolveBidWindows(int64_t now) {
	sqlite3* db = Database::GetConnection();
	if (!db) return;
	EnsureBidSchema(db);
	static std::mutex mu;
	std::unique_lock<std::mutex> lock(mu);
	std::vector<uint32_t> due;
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db, "SELECT position FROM ga_ava_bid_windows WHERE closes_at <= ?1", -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int64(s, 1, now);
		while (sqlite3_step(s) == SQLITE_ROW) due.push_back((uint32_t)sqlite3_column_int(s, 0));
		sqlite3_finalize(s);
	}
	struct Msg { std::string name, text; };
	std::vector<Msg> msgs;
	std::vector<int64_t> mailed;
	for (uint32_t pos : due) {
		struct B { int64_t owner, cid, amount; int type; };
		std::vector<B> bids;
		if (sqlite3_prepare_v2(db, "SELECT owner_agency_id, character_id, amount, attack_type FROM ga_ava_sealed_bids "
		        "WHERE position = ?1 ORDER BY amount DESC, placed_at ASC", -1, &s, nullptr) == SQLITE_OK) {
			sqlite3_bind_int(s, 1, (int)pos);
			while (sqlite3_step(s) == SQLITE_ROW)
				bids.push_back({sqlite3_column_int64(s, 0), sqlite3_column_int64(s, 1), sqlite3_column_int64(s, 2),
					sqlite3_column_int(s, 3)});
			sqlite3_finalize(s);
		}
		sqlite3_exec(db, "BEGIN IMMEDIATE", nullptr, nullptr, nullptr);
		Exec2(db, "DELETE FROM ga_ava_bid_windows WHERE position = ?1", pos);
		Exec2(db, "DELETE FROM ga_ava_sealed_bids WHERE position = ?1", pos);
		if (!bids.empty()) {
			const B& w = bids.front();
			const bool unclaimed = Scalar(db, "SELECT 1 FROM ga_ava_territories WHERE zone_id = 1 AND position = ?1", pos) < 0;
			if (unclaimed) {
				// Unclaimed hex: the winner takes it at once (no match), the bid is spent.
				sqlite3_stmt* t = nullptr;
				if (sqlite3_prepare_v2(db,
				        "INSERT OR REPLACE INTO ga_ava_territories (zone_id, position, owner_agency_id,"
				        " owner_name, color_r, color_g, color_b, captured_at) "
				        "SELECT 1, ?1, COALESCE(a.id, ?3 + c.id), COALESCE(a.name, u.username),"
				        " COALESCE(a.color_r, 0.9), COALESCE(a.color_g, 0.6), COALESCE(a.color_b, 0.1), ?4 "
				        "FROM ga_characters c JOIN ga_users u ON u.id = c.user_id "
				        "LEFT JOIN ga_agency_members m ON m.user_id = c.user_id "
				        "LEFT JOIN ga_agencies a ON a.id = m.agency_id WHERE c.id = ?2", -1, &t, nullptr) == SQLITE_OK) {
					sqlite3_bind_int(t, 1, (int)pos);
					sqlite3_bind_int64(t, 2, w.cid);
					sqlite3_bind_int64(t, 3, kSoloOwnerBase);
					sqlite3_bind_int64(t, 4, now);
					sqlite3_step(t);
					sqlite3_finalize(t);
				}
				msgs.push_back({AccountOf(db, w.cid), "Your agency has ownership of a new territory. (AvA territory " +
					AvaZones::Label(pos) + ", your sealed bid of " + std::to_string(w.amount) + " credits won)"});
			} else {
				sqlite3_stmt* t = nullptr;
				if (sqlite3_prepare_v2(db, "INSERT OR REPLACE INTO ga_ava_bids (zone_id, position, owner_agency_id, character_id, "
				        "amount, attack_type, placed_at) VALUES (1, ?1, ?2, ?3, ?4, ?5, ?6)", -1, &t, nullptr) == SQLITE_OK) {
					sqlite3_bind_int(t, 1, (int)pos);
					sqlite3_bind_int64(t, 2, w.owner);
					sqlite3_bind_int64(t, 3, w.cid);
					sqlite3_bind_int64(t, 4, w.amount);
					sqlite3_bind_int(t, 5, w.type);
					sqlite3_bind_int64(t, 6, now);
					sqlite3_step(t);
					sqlite3_finalize(t);
				}
				const int mins = (int)SettingNum(db, "ava_won_bid_minutes", 30.0);
				msgs.push_back({AccountOf(db, w.cid), "You won the auction to attack the territory. (AvA territory " +
					AvaZones::Label(pos) + ", " + std::to_string(w.amount) + " credits) Launch the attack within " +
					std::to_string(mins) + " min."});
			}
			for (size_t i = 1; i < bids.size(); ++i) {
				Refund(db, bids[i].cid, pos, bids[i].amount, "Your agency has been outbid on the territory.", mailed);
				msgs.push_back({AccountOf(db, bids[i].cid), "Your agency has been outbid on the territory. (AvA territory " +
					AvaZones::Label(pos) + ", your bid came back by mail)"});
			}
			Logger::Log("tcp", "[AvaParity] bid window pos %u closed: %zu bid(s), winner owner %lld char %lld %lld credits%s\n",
				pos, bids.size(), (long long)w.owner, (long long)w.cid, (long long)w.amount, unclaimed ? " -> unclaimed hex granted" : "");
		}
		sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr);
	}
	// Won bids that were never launched (window mode only; ours): refund by mail.
	if (WindowMode(db)) {
		const int64_t cutoff = now - (int64_t)(SettingNum(db, "ava_won_bid_minutes", 30.0) * 60.0);
		struct E { uint32_t pos; int64_t cid, amount; };
		std::vector<E> exp;
		if (sqlite3_prepare_v2(db, "SELECT position, character_id, amount FROM ga_ava_bids b WHERE zone_id = 1 AND placed_at < ?1 "
		        "AND NOT EXISTS (SELECT 1 FROM ga_ava_attacks a WHERE a.zone_id = 1 AND a.position = b.position AND a.queued_at >= ?2)",  // launched: spent at the result
		        -1, &s, nullptr) == SQLITE_OK) {
			sqlite3_bind_int64(s, 1, cutoff);
			sqlite3_bind_int64(s, 2, now - 3600);
			while (sqlite3_step(s) == SQLITE_ROW)
				exp.push_back({(uint32_t)sqlite3_column_int(s, 0), sqlite3_column_int64(s, 1), sqlite3_column_int64(s, 2)});
			sqlite3_finalize(s);
		}
		for (const E& e : exp) {
			sqlite3_exec(db, "BEGIN IMMEDIATE", nullptr, nullptr, nullptr);
			if (Exec2(db, "DELETE FROM ga_ava_bids WHERE zone_id = 1 AND position = ?1 AND character_id = ?2", e.pos, e.cid) &&
			    sqlite3_changes(db) == 1)
				Refund(db, e.cid, e.pos, e.amount, "the attack was not launched in time.", mailed);
			sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr);
			Logger::Log("tcp", "[AvaParity] won bid pos %u char %lld expired (not launched), refunded\n", e.pos, (long long)e.cid);
		}
	}
	lock.unlock();  // the zone push comes back here
	for (const Msg& m : msgs) if (!m.name.empty()) ChatSession::SystemMessageTo(m.name, "AvA: " + m.text);
	for (int64_t c : mailed) TcpSession::NotifyNewMail(c);
	if (!due.empty()) TcpSession::AvaPushZoneToViewers(0);
}

// With the bid window on, HEX_CAPTURE needs this owner's won bid on the hex (wiki: the auction
// winner attacks); while bidding is open nobody can launch.
int OpenBid(int64_t owner, uint32_t position, int64_t secs) {
	sqlite3* db = Database::GetConnection();
	if (!db || owner == 0) return 0;
	sqlite3_stmt* s = nullptr;
	int n = 0;
	if (sqlite3_prepare_v2(db, "UPDATE ga_ava_bids SET open_until = ?1 WHERE zone_id = 1 AND position = ?2 "
	        "AND owner_agency_id = ?3", -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int64(s, 1, secs > 0 ? (int64_t)std::time(nullptr) + secs : 0);
		sqlite3_bind_int(s, 2, (int)position);
		sqlite3_bind_int64(s, 3, owner);
		if (sqlite3_step(s) == SQLITE_DONE) n = sqlite3_changes(db);
		sqlite3_finalize(s);
	}
	Logger::Log("tcp", "[AvaParity] won bid pos %u owner %lld opened for %llds (%d row)\n", position, (long long)owner,
		(long long)secs, n);
	return n;
}

int64_t OpenBidOwnerFor(uint32_t position, int64_t character_id) {
	sqlite3* db = Database::GetConnection();
	if (!db) return 0;
	const int64_t o = Scalar(db, "SELECT owner_agency_id FROM ga_ava_bids WHERE zone_id = 1 AND position = ?1 "
		"AND open_until > ?2", position, (int64_t)std::time(nullptr), 0);
	return o > 0 && o != OwnerOfCharacter(db, character_id) ? o : 0;
}

std::vector<OpenFight> OpenFights(uint32_t zone_id, int64_t character_id) {
	std::vector<OpenFight> out;
	sqlite3* db = Database::GetConnection();
	if (!db) return out;
	const int64_t now = (int64_t)std::time(nullptr), me = OwnerOfCharacter(db, character_id);
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db, "SELECT b.position, b.owner_agency_id, COALESCE(t.owner_agency_id, 0), b.open_until "
	        "FROM ga_ava_bids b LEFT JOIN ga_ava_territories t ON t.zone_id = 1 AND t.position = b.position "
	        "WHERE b.zone_id = 1 AND b.open_until > ?1 ORDER BY b.open_until", -1, &s, nullptr) != SQLITE_OK) return out;
	sqlite3_bind_int64(s, 1, now);
	while (sqlite3_step(s) == SQLITE_ROW) {
		const uint32_t p = (uint32_t)sqlite3_column_int(s, 0);
		const int64_t att = sqlite3_column_int64(s, 1), def = sqlite3_column_int64(s, 2);
		if (att == me || def == me || (zone_id && AvaZones::ZoneOf(p) != zone_id)) continue;
		out.push_back({p, att, def, sqlite3_column_int64(s, 3) - now});
	}
	sqlite3_finalize(s);
	return out;
}

int64_t OwnerOf(int64_t character_id) {
	sqlite3* db = Database::GetConnection();
	return db ? OwnerOfCharacter(db, character_id) : 0;
}

std::string OwnerName(int64_t owner) {
	std::string out;
	sqlite3* db = Database::GetConnection();
	sqlite3_stmt* s = nullptr;
	if (db && sqlite3_prepare_v2(db, "SELECT name FROM ga_agencies WHERE id = ?1 UNION ALL "
	        "SELECT owner_name FROM ga_ava_territories WHERE owner_agency_id = ?1 LIMIT 1", -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int64(s, 1, owner);
		if (sqlite3_step(s) == SQLITE_ROW)
			if (const unsigned char* n = sqlite3_column_text(s, 0)) out = (const char*)n;
		sqlite3_finalize(s);
	}
	return out;
}

std::vector<CombatLogRow> CombatLog(uint32_t zone, const std::string& agency_like, uint32_t outcome, int64_t from, int64_t to) {
	std::vector<CombatLogRow> out;
	sqlite3* db = Database::GetConnection();
	sqlite3_stmt* s = nullptr;
	if (!db || sqlite3_prepare_v2(db, "SELECT at, position, building, owner, attacker, outcome FROM ga_ava_combat_log WHERE position >= ?1 AND position < ?2 "
	        "AND at >= ?3 AND at < ?4 AND (?5 = 0 OR outcome = ?5) AND (owner LIKE ?6 OR attacker LIKE ?6) ORDER BY id DESC LIMIT 200", -1, &s, nullptr) != SQLITE_OK) return out;
	sqlite3_bind_int64(s, 1, AvaZones::Base(zone));
	sqlite3_bind_int64(s, 2, (int64_t)AvaZones::Base(zone) + 1000);
	sqlite3_bind_int64(s, 3, from);
	sqlite3_bind_int64(s, 4, to);
	sqlite3_bind_int(s, 5, (int)outcome);
	sqlite3_bind_text(s, 6, agency_like.c_str(), -1, SQLITE_TRANSIENT);
	while (sqlite3_step(s) == SQLITE_ROW) {
		const char* o = (const char*)sqlite3_column_text(s, 3); const char* a = (const char*)sqlite3_column_text(s, 4);
		out.push_back({sqlite3_column_int64(s, 0), (uint32_t)sqlite3_column_int(s, 1), (uint32_t)sqlite3_column_int(s, 2), o ? o : "", a ? a : "", (uint32_t)sqlite3_column_int(s, 5)});
	}
	sqlite3_finalize(s);
	return out;
}

std::vector<OpenFight> OwnFights(uint32_t zone_id, int64_t character_id) {
	std::vector<OpenFight> out;
	sqlite3* db = Database::GetConnection();
	if (!db) return out;
	const int64_t now = (int64_t)std::time(nullptr), me = OwnerOfCharacter(db, character_id);
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db, "SELECT b.position, b.owner_agency_id, COALESCE(t.owner_agency_id, 0), b.open_until "
	        "FROM ga_ava_bids b LEFT JOIN ga_ava_territories t ON t.zone_id = 1 AND t.position = b.position "
	        "WHERE b.zone_id = 1 AND (b.owner_agency_id = ?1 OR t.owner_agency_id = ?1)", -1, &s, nullptr) != SQLITE_OK) return out;
	sqlite3_bind_int64(s, 1, me);
	while (sqlite3_step(s) == SQLITE_ROW) {
		const uint32_t p = (uint32_t)sqlite3_column_int(s, 0);
		if (zone_id && AvaZones::ZoneOf(p) != zone_id) continue;
		out.push_back({p, sqlite3_column_int64(s, 1), sqlite3_column_int64(s, 2), sqlite3_column_int64(s, 3) - now});
	}
	sqlite3_finalize(s);
	return out;
}

bool AttackNeedsWonBid(sqlite3* db, int64_t character_id, uint32_t position, Gate& g) {
	if (!WindowMode(db)) return false;
	const std::string P = AvaZones::Label(position);
	int64_t left = 0;
	if (BidWindowOpen(position, &left)) {
		g.refuse = true;
		g.text = "AvA: bidding on territory " + P + " is still open - the winner can attack when it closes.";
	} else if (Scalar(db, "SELECT 1 FROM ga_ava_bids WHERE zone_id = 1 AND position = ?1 AND owner_agency_id = ?2",
	               position, OwnerOfCharacter(db, character_id)) < 0 && OpenBidOwnerFor(position, character_id) == 0) {
		g.refuse = true;
		g.text = "AvA: win the auction for territory " + P + " first - place a bid on the map (bids are sealed, "
			"bidding closes about 4 minutes after the first bid).";
	}
	if (g.refuse) Logger::Log("tcp", "[AvaParity] char %lld pos %u attack refused: %s\n", (long long)character_id, position,
		g.text.c_str());
	return g.refuse;
}

int64_t OwnSealedBid(int64_t character_id, uint32_t pos) {
	sqlite3* db = Database::GetConnection();
	if (!db) return -1;
	EnsureBidSchema(db);
	return Scalar(db, "SELECT amount FROM ga_ava_sealed_bids WHERE position = ?1 AND owner_agency_id = ?2", pos, OwnerOfCharacter(db, character_id));
}

uint32_t AttackTypesFor(uint32_t pos) {  // bit 0 Capture, bit 1 Steal: no theft on an empty hex
	sqlite3* db = Database::GetConnection();
	return db && OwnerOf(db, pos) != 0 && Setting(db, "ava_theft", "on") != "off" ? 3u : 1u;
}

BidResult SealedBid(int64_t character_id, uint32_t pos, uint32_t amount, uint32_t attack_type, bool cancel) {
	BidResult r;
	sqlite3* db = Database::GetConnection();
	if (!db) return r;
	EnsureBidSchema(db);
	const int64_t now = (int64_t)std::time(nullptr);
	ResolveBidWindows(now);
	if (!WindowMode(db)) return r;
	const int64_t owner = OwnerOfCharacter(db, character_id);
	if (owner == 0) return r;
	const std::string P = AvaZones::Label(pos);
	const int64_t mine = Scalar(db, "SELECT amount FROM ga_ava_sealed_bids WHERE position = ?1 AND owner_agency_id = ?2", pos, owner);
	if (cancel) {
		if (mine < 0) return r;  // no sealed bid: the old code cancels a won bid (refund) or says "no bid"
		r.handled = true;
		Exec2(db, "DELETE FROM ga_ava_sealed_bids WHERE position = ?1 AND owner_agency_id = ?2", pos, owner);
		if (Scalar(db, "SELECT COUNT(*) FROM ga_ava_sealed_bids WHERE position = ?1", pos, 0, 0) == 0)
			Exec2(db, "DELETE FROM ga_ava_bid_windows WHERE position = ?1", pos);
		r.text = "AvA: bid on territory " + P + " cancelled - the bid money (" + std::to_string(mine) + " credits) is forfeit.";
		Logger::Log("tcp", "[AvaParity] char %lld cancelled sealed bid pos %u (%lld forfeit)\n", (long long)character_id, pos, (long long)mine);
		return r;
	}
	r.handled = true;
	if (Scalar(db, "SELECT 1 FROM ga_ava_territories WHERE zone_id = 1 AND position = ?1 AND owner_agency_id = ?2", pos, owner) > 0) {
		r.text = "AvA: your agency already holds territory " + P + ".";
		return r;
	}
	if (Scalar(db, "SELECT 1 FROM ga_ava_attacks WHERE zone_id = 1 AND position = ?1 AND queued_at >= ?2 LIMIT 1", pos, now - 1800) > 0) {
		r.msg_id = 41217;  // "Territory is under attack, no bidding allowed."
		return r;
	}
	if (Scalar(db, "SELECT 1 FROM ga_ava_bids WHERE zone_id = 1 AND position = ?1", pos) > 0) {
		r.text = "AvA: territory " + P + " is reserved by a won bid until its attack is launched.";
		return r;
	}
	if (Scalar(db, "SELECT position FROM ga_ava_sealed_bids WHERE owner_agency_id = ?1 AND position <> ?2 "
	        "UNION ALL SELECT position FROM ga_ava_bids WHERE zone_id = 1 AND owner_agency_id = ?1 AND position <> ?2 LIMIT 1",
	        owner, pos) >= 0) {
		r.msg_id = 36570;  // "Your strikeforce already bid on another territory."
		return r;
	}
	const int64_t min_bid = 1000;
	if ((int64_t)amount < min_bid || amount > 100000000u || (mine >= 0 && (int64_t)amount <= mine)) {
		r.msg_id = 40951;  // "Your bid was too low."
		r.text = mine >= 0 ? "AvA: a new bid must be higher than your current bid of " + std::to_string(mine) + " credits."
		                   : "AvA: the minimum bid is 1000 credits.";
		return r;
	}
	if (mine >= 0) {
		const int64_t placed = Scalar(db, "SELECT placed_at FROM ga_ava_sealed_bids WHERE position = ?1 AND owner_agency_id = ?2", pos, owner);
		const int64_t wait = (int64_t)SettingNum(db, "ava_rebid_seconds", 15.0) - (now - placed);
		if (wait > 0) {
			r.msg_id = 40885;  // "You need to wait a bit before placing another bid."
			return r;
		}
	}
	const int64_t cost = mine >= 0 ? (int64_t)amount - mine : (int64_t)amount;
	sqlite3_exec(db, "BEGIN IMMEDIATE", nullptr, nullptr, nullptr);
	bool ok = Exec2(db, "UPDATE ga_characters SET credits = credits - ?1 WHERE id = ?2 AND deleted_at = 0 AND credits >= ?1",
		cost, character_id) && sqlite3_changes(db) == 1;
	if (!ok) {
		sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
		r.text = "AvA: not enough credits for a " + std::to_string(amount) + " credit bid.";
		return r;
	}
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db, "INSERT OR REPLACE INTO ga_ava_sealed_bids (position, owner_agency_id, character_id, amount, "
	        "attack_type, placed_at) VALUES (?1, ?2, ?3, ?4, ?5, ?6)", -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int(s, 1, (int)pos);
		sqlite3_bind_int64(s, 2, owner);
		sqlite3_bind_int64(s, 3, character_id);
		sqlite3_bind_int64(s, 4, amount);
		sqlite3_bind_int(s, 5, attack_type == kAttackTheft ? (int)kAttackTheft : (int)kAttackCapture);
		sqlite3_bind_int64(s, 6, now);
		ok = sqlite3_step(s) == SQLITE_DONE;
		sqlite3_finalize(s);
	} else ok = false;
	int64_t closes = Scalar(db, "SELECT closes_at FROM ga_ava_bid_windows WHERE position = ?1", pos);
	const bool opened = closes < 0;
	if (ok && opened) {
		static std::mt19937 rng{std::random_device{}()};
		const int jit = std::max(0, (int)SettingNum(db, "ava_bid_window_jitter", 60.0));
		closes = now + (int64_t)std::max(0.0, SettingNum(db, "ava_bid_window_seconds", 240.0)) +
			(jit > 0 ? std::uniform_int_distribution<int>(0, jit)(rng) : 0);
		ok = Exec2(db, "INSERT INTO ga_ava_bid_windows (position, opened_at, closes_at) VALUES (?1, ?2, ?3)", pos, now, closes);
	}
	if (!ok) {
		sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
		r.text = "AvA: the bid could not be placed.";
		return r;
	}
	sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr);
	r.wallet_changed = true;
	r.text = "AvA: sealed bid of " + std::to_string(amount) + " credits placed on territory " + P + " (other bids stay hidden). " +
		(opened ? "Bidding closes in about 4-5 minutes" : "Bidding closes in a few minutes") +
		"; the highest bid wins. You can raise your bid every 15 s.";
	Logger::Log("tcp", "[AvaParity] sealed bid pos %u owner %lld char %lld %u credits (type %u)%s, closes_at %lld\n", pos,
		(long long)owner, (long long)character_id, amount, attack_type, opened ? " [window opened]" : "", (long long)closes);
	return r;
}

// ---------------------------------------------------------------------------------------------
// Lab blueprints and Factory recipes (wiki AVA "Facilities", news 4190).
// Original: Labs research AvA blueprints (loot tables AVA_LAB_TECH1/2/3 = 153/154/155, item type
// 686), Factories turn a blueprint plus mined resources (asm_data_set_blueprint_items) into the
// AvA item. Ours: a Lab's research points buy blueprint rolls (cost = the blueprint's
// production_cost x quantity; tier picked by the held Labs' tiers); a Factory build consumes one
// blueprint whose created_item_id is the item plus its components from ga_ava_resources (Mines
// with an assigned resource), on top of the production points. Scrapping gives both back (ours).
// cs_settings ava_lab_blueprints on|off, ava_factory_recipes on|off (off = old points-only build).
namespace {
bool On(sqlite3* db, const char* key) { return Lower(Setting(db, key, "on")) != "off"; }
}  // namespace

int LabBlueprints(sqlite3* db) {
	if (!db || !On(db, "ava_lab_blueprints")) return 0;
	struct O { double research = 0; std::vector<int> tiers; };
	std::map<int64_t, O> owners;
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db, "SELECT t.position, t.owner_agency_id, COALESCE(k.research, 0) FROM ga_ava_territories t "
	        "LEFT JOIN ga_ava_stock k ON k.owner_agency_id = t.owner_agency_id WHERE t.zone_id = 1", -1, &s, nullptr) == SQLITE_OK) {
		while (sqlite3_step(s) == SQLITE_ROW) {
			const uint32_t pos = (uint32_t)sqlite3_column_int(s, 0);
			if (SubtypeOf(db, AvaParityBuildingAt(pos)) != 1321) continue;
			O& o = owners[sqlite3_column_int64(s, 1)];
			o.research = sqlite3_column_double(s, 2);
			o.tiers.push_back(std::clamp(TierAt(db, pos), 1, 3));
		}
		sqlite3_finalize(s);
	}
	static std::mt19937 rng{(unsigned)std::time(nullptr) ^ 0x549u};
	int total = 0;
	for (auto& [owner, o] : owners) {
		double left = o.research;
		int made = 0;
		while (made < 50) {
			// tier weighted by the held Labs (a T3 Lab counts 3x a T1 Lab, like its output 30 vs 10)
			int wsum = 0;
			for (int t : o.tiers) wsum += t;
			int x = std::uniform_int_distribution<int>(1, std::max(1, wsum))(rng), tier = o.tiers.front();
			for (int t : o.tiers) { if (x <= t) { tier = t; break; } x -= t; }
			struct R { int item, qty; double chance, cost; };
			std::vector<R> rows;
			double sum = 0;
			if (sqlite3_prepare_v2(db, "SELECT l.item_id, MAX(1, l.quantity), l.drop_chance, i.production_cost "
			        "FROM asm_data_set_loot_table_items l JOIN asm_data_set_items i ON i.item_id = l.item_id "
			        "WHERE l.loot_table_id = ?1 AND i.item_type_value_id = 686", -1, &s, nullptr) == SQLITE_OK) {
				sqlite3_bind_int(s, 1, 152 + tier);
				while (sqlite3_step(s) == SQLITE_ROW) {
					rows.push_back({sqlite3_column_int(s, 0), sqlite3_column_int(s, 1), std::max(0.0, sqlite3_column_double(s, 2)),
						std::max(1.0, sqlite3_column_double(s, 3))});
					sum += rows.back().chance;
				}
				sqlite3_finalize(s);
			}
			// A Lab set to a blueprint (Add Task) researches only those.
			std::vector<R> chosen;
			if (sqlite3_prepare_v2(db, "SELECT l.item_id, MAX(1, MIN(l.quantity)), i.production_cost FROM ga_ava_mine_resources r "
			        "JOIN ga_ava_territories t ON t.zone_id = r.zone_id AND t.position = r.position AND t.owner_agency_id = r.owner_agency_id "
			        "JOIN asm_data_set_loot_table_items l ON l.item_id = r.item_id AND l.loot_table_id IN (153, 154, 155) "
			        "JOIN asm_data_set_items i ON i.item_id = l.item_id WHERE r.owner_agency_id = ?1 AND i.item_type_value_id = 686 GROUP BY l.item_id",
			        -1, &s, nullptr) == SQLITE_OK) {
				sqlite3_bind_int64(s, 1, owner);
				while (sqlite3_step(s) == SQLITE_ROW)
					chosen.push_back({sqlite3_column_int(s, 0), sqlite3_column_int(s, 1), 1.0, std::max(1.0, sqlite3_column_double(s, 2))});
				sqlite3_finalize(s);
			}
			if (!chosen.empty()) { rows = chosen; sum = (double)chosen.size(); }
			if (rows.empty() || sum <= 0) break;
			double y = std::uniform_real_distribution<double>(0.0, sum)(rng);
			const R* pick = &rows.back();
			for (const R& r : rows) { if (y < r.chance) { pick = &r; break; } y -= r.chance; }
			const double cost = pick->cost * pick->qty;
			if (left + 1e-6 < cost) break;  // not enough research yet: keep it for the next settle
			if (sqlite3_prepare_v2(db, "INSERT INTO ga_ava_items (owner_agency_id, item_id, count) VALUES (?1, ?2, ?3) "
			        "ON CONFLICT(owner_agency_id, item_id) DO UPDATE SET count = count + ?3", -1, &s, nullptr) == SQLITE_OK) {
				sqlite3_bind_int64(s, 1, owner);
				sqlite3_bind_int(s, 2, pick->item);
				sqlite3_bind_int(s, 3, pick->qty);
				sqlite3_step(s);
				sqlite3_finalize(s);
			}
			left -= cost;
			++made;
			Logger::Log("tcp", "[AvaParity] Lab owner %lld: blueprint %d x%d (T%d table %d) for %.0f research\n",
				(long long)owner, pick->item, pick->qty, tier, 152 + tier, cost);
		}
		if (made) {
			if (sqlite3_prepare_v2(db, "UPDATE ga_ava_stock SET research = ?2 WHERE owner_agency_id = ?1", -1, &s, nullptr) == SQLITE_OK) {
				sqlite3_bind_int64(s, 1, owner);
				sqlite3_bind_double(s, 2, left);
				sqlite3_step(s);
				sqlite3_finalize(s);
			}
			total += made;
		}
	}
	return total;
}

uint32_t FactoryTarget(uint32_t item_id) {
	sqlite3* db = Database::GetConnection();
	if (!db) return item_id;
	// A blueprint id (the client may send the blueprint row it picked) -> the item it creates.
	const int64_t made = Scalar(db, "SELECT b.created_item_id FROM asm_data_set_blueprints b JOIN asm_data_set_items i "
		"ON i.item_id = b.blueprint_id WHERE b.blueprint_id = ?1 AND i.item_type_value_id = 686", item_id);
	return made > 0 ? (uint32_t)made : item_id;
}

Gate FactoryRecipe(int64_t owner, uint32_t item_id, uint32_t count, bool remove) {
	Gate g;
	sqlite3* db = Database::GetConnection();
	if (!db || !On(db, "ava_factory_recipes") || count == 0) return g;
	// Which blueprint: one the owner holds (most first) among those creating this item; scrap returns
	// the first AvA blueprint (lowest id) of the item.
	sqlite3_stmt* s = nullptr;
	int64_t bp = -1, have_bp = 0;
	if (sqlite3_prepare_v2(db, "SELECT b.blueprint_id, COALESCE(a.count, 0) FROM asm_data_set_blueprints b "
	        "JOIN asm_data_set_items i ON i.item_id = b.blueprint_id AND i.item_type_value_id = 686 "
	        "LEFT JOIN ga_ava_items a ON a.owner_agency_id = ?2 AND a.item_id = b.blueprint_id "
	        "WHERE b.created_item_id = ?1 ORDER BY COALESCE(a.count, 0) DESC, b.blueprint_id LIMIT 1", -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int(s, 1, (int)item_id);
		sqlite3_bind_int64(s, 2, owner);
		if (sqlite3_step(s) == SQLITE_ROW) { bp = sqlite3_column_int64(s, 0); have_bp = sqlite3_column_int64(s, 1); }
		sqlite3_finalize(s);
	}
	if (bp < 0) return g;  // no blueprint exists for this item (e.g. basic gear): points only, as before
	const bool vendor_bp = Scalar(db, "SELECT item_type_value_id FROM asm_data_set_items WHERE item_id = ?1", item_id) == 1312;  // deed/missile: vendor blueprint
	std::vector<std::pair<int, double>> parts;  // component item, quantity per build
	if (sqlite3_prepare_v2(db, "SELECT item_id, quantity FROM asm_data_set_blueprint_items WHERE blueprint_id = ?1",
	        -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int64(s, 1, bp);
		while (sqlite3_step(s) == SQLITE_ROW) parts.push_back({sqlite3_column_int(s, 0), sqlite3_column_double(s, 1)});
		sqlite3_finalize(s);
	}
	auto name = [&](int64_t id) {
		sqlite3_stmt* q = nullptr;
		std::string n = "item " + std::to_string(id);
		if (sqlite3_prepare_v2(db, "SELECT m.message FROM asm_data_set_items i JOIN asm_data_set_msg_translations m "
		        "ON m.msg_id = i.name_msg_id WHERE i.item_id = ?1", -1, &q, nullptr) == SQLITE_OK) {
			sqlite3_bind_int64(q, 1, id);
			if (sqlite3_step(q) == SQLITE_ROW && sqlite3_column_text(q, 0)) n = (const char*)sqlite3_column_text(q, 0);
			sqlite3_finalize(q);
		}
		return n;
	};
	if (!remove) {
		std::string missing;
		if (have_bp < (int64_t)count && !vendor_bp) missing = name(bp) + " x" + std::to_string(count - have_bp);
		for (const auto& [it, q] : parts) {
			double have = 0;
			if (sqlite3_prepare_v2(db, "SELECT amount FROM ga_ava_resources WHERE owner_agency_id = ?1 AND item_id = ?2",
			        -1, &s, nullptr) == SQLITE_OK) {
				sqlite3_bind_int64(s, 1, owner);
				sqlite3_bind_int(s, 2, it);
				if (sqlite3_step(s) == SQLITE_ROW) have = sqlite3_column_double(s, 0);
				sqlite3_finalize(s);
			}
			if (have + 1e-6 < q * count)
				missing += (missing.empty() ? "" : ", ") + name(it) + " " + std::to_string((long long)(q * count - have + 0.999));
		}
		if (!missing.empty()) {
			g.refuse = true;
			g.text = "AvA Factory: " + name(item_id) + " needs its blueprint (from a Lab) and mined resources. Missing: " + missing + ".";
			Logger::Log("tcp", "[AvaParity] Factory owner %lld item %u x%u refused: missing %s\n", (long long)owner, item_id, count,
				missing.c_str());
			return g;
		}
	}
	const int sign = remove ? 1 : -1;
	sqlite3_exec(db, "SAVEPOINT p0549", nullptr, nullptr, nullptr);
	if (!vendor_bp && sqlite3_prepare_v2(db, "INSERT INTO ga_ava_items (owner_agency_id, item_id, count) VALUES (?1, ?2, ?3) "
	        "ON CONFLICT(owner_agency_id, item_id) DO UPDATE SET count = count + ?3", -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_int64(s, 1, owner);
		sqlite3_bind_int64(s, 2, bp);
		sqlite3_bind_int(s, 3, sign * (int)count);
		sqlite3_step(s);
		sqlite3_finalize(s);
	}
	for (const auto& [it, q] : parts)
		if (sqlite3_prepare_v2(db, "INSERT INTO ga_ava_resources (owner_agency_id, item_id, amount) VALUES (?1, ?2, ?3) "
		        "ON CONFLICT(owner_agency_id, item_id) DO UPDATE SET amount = amount + ?3", -1, &s, nullptr) == SQLITE_OK) {
			sqlite3_bind_int64(s, 1, owner);
			sqlite3_bind_int(s, 2, it);
			sqlite3_bind_double(s, 3, sign * q * count);
			sqlite3_step(s);
			sqlite3_finalize(s);
		}
	sqlite3_exec(db, "DELETE FROM ga_ava_items WHERE count <= 0; DELETE FROM ga_ava_resources WHERE amount <= 1e-9;"
		"RELEASE p0549", nullptr, nullptr, nullptr);
	Logger::Log("tcp", "[AvaParity] Factory owner %lld item %u x%u: blueprint %lld + %zu component kind(s) %s\n", (long long)owner,
		item_id, count, (long long)bp, parts.size(), remove ? "returned (scrap)" : "consumed");
	return g;
}

// ---------------------------------------------------------------------------------------------
// Regions (wiki AVA "Region", news 4190): a zone was split into 5-6 regions and one
// region was open per hour; bids only in the open region. Client: DATA_SET_ZONE_REGIONS rows
// {ZONE_OBJECT_ID, HEX_ZONE_REGION_ID, NAME_MSG_ID} (texts 59671-59675 "Region 1-5"), territory
// row HEX_ZONE_REGION_ID (+0x8c) and REGION_SECONDS (+0x6c, row parse 0x10927888). The original
// hex -> region map is lost; ours: 5 regions = the 5 rows of our 7x5 map, region (hour % 5) + 1
// open. cs_settings ava_regions off (default, low-pop rule like the always-open zone) | on.
// A won bid may still launch after its region closed.
namespace {
constexpr int kRegions = 5;
bool RegionsOn(sqlite3* db) { return Lower(Setting(db, "ava_regions", "off")) == "on"; }
}  // namespace

int RegionCount() {
	sqlite3* db = Database::GetConnection();
	return db && RegionsOn(db) ? kRegions : 0;
}

int RegionOf(uint32_t position) {
	return RegionCount() ? AvaZones::Region(AvaZones::ZoneOf(position), AvaZones::Local(position)) : 0;
}

int OpenRegion(int64_t now, int64_t* secs_left) {
	if (!RegionCount()) return 0;
	if (secs_left) *secs_left = 3600 - now % 3600;
	return (int)((now / 3600) % kRegions) + 1;
}

int64_t RegionSeconds(uint32_t position) {
	const int r = RegionOf(position);
	if (!r) return -1;
	int64_t left = 0;
	const int64_t now = (int64_t)std::time(nullptr);
	const int open = OpenRegion(now, &left);
	if (r == open) return left;                              // open: seconds until it closes
	return left + (int64_t)(((r - open - 1 + kRegions) % kRegions)) * 3600;  // closed: seconds until it opens
}

namespace {
constexpr const char* kZoneLayoutMark = "12x30";

bool TableExists(sqlite3* db, const char* table) {
	sqlite3_stmt* s = nullptr;
	bool yes = false;
	if (sqlite3_prepare_v2(db, "SELECT 1 FROM sqlite_master WHERE type = 'table' AND name = ?1", -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_text(s, 1, table, -1, SQLITE_STATIC);
		yes = sqlite3_step(s) == SQLITE_ROW;
		sqlite3_finalize(s);
	}
	return yes;
}

// Position-based rows that belong to a player: an owner that is not an agency of service agents only, or a character on a
// player account. -> one text per hit, empty = only service agents hold anything.
std::vector<std::string> PlayerHoldings(sqlite3* db) {
	static const char* kOwned[] = {"ga_ava_territories", "ga_ava_buildings", "ga_ava_shields", "ga_ava_mine_resources", "ga_ava_bids",
		"ga_ava_hq", "ga_ava_shield_breaches", "ga_ava_sealed_bids"};
	static const char* kByCharacter[] = {"ga_ava_bids", "ga_ava_sealed_bids", "ga_ava_attacks", "ga_ava_attack_types", "ga_ava_missile_picks"};
	std::vector<std::string> out;
	const auto collect = [&](const std::string& q, const char* table) {
		sqlite3_stmt* s = nullptr;
		if (sqlite3_prepare_v2(db, q.c_str(), -1, &s, nullptr) != SQLITE_OK) {
			out.push_back(std::string(table) + ": cannot check (" + sqlite3_errmsg(db) + ")");
			return;
		}
		while (sqlite3_step(s) == SQLITE_ROW)
			out.push_back(std::string(table) + ": position " + std::to_string(sqlite3_column_int64(s, 0)) + " of " + std::to_string(sqlite3_column_int64(s, 1)));
		sqlite3_finalize(s);
	};
	const std::string solo = std::to_string(kSoloOwnerBase);
	for (const char* t : kOwned) {
		if (!TableExists(db, t)) continue;
		collect(std::string("SELECT x.position, x.owner_agency_id FROM ") + t + " x WHERE x.owner_agency_id != 0 AND CASE "
			"WHEN x.owner_agency_id >= " + solo + " THEN COALESCE((SELECT u.service_account FROM ga_characters c JOIN ga_users u ON u.id = c.user_id "
			"WHERE c.id = x.owner_agency_id - " + solo + "), 0) != 1 "
			"WHEN NOT EXISTS (SELECT 1 FROM ga_agencies a JOIN ga_users u ON u.id = a.leader_user_id WHERE a.id = x.owner_agency_id AND u.service_account = 1) THEN 1 "
			"ELSE EXISTS (SELECT 1 FROM ga_agency_members m JOIN ga_users u ON u.id = m.user_id WHERE m.agency_id = x.owner_agency_id AND u.service_account != 1) END", t);
	}
	for (const char* t : kByCharacter) {
		if (!TableExists(db, t)) continue;
		collect(std::string("SELECT x.position, x.character_id FROM ") + t + " x WHERE COALESCE((SELECT u.service_account FROM ga_characters c "
			"JOIN ga_users u ON u.id = c.user_id WHERE c.id = x.character_id), 0) != 1", t);
	}
	return out;
}

std::string DbFile(sqlite3* db) {
	const char* f = sqlite3_db_filename(db, "main");
	return f ? f : "";
}

// -> false: nothing was changed, the caller must not start the server.
bool ClearForNewBoards(sqlite3* db) {
	const auto holdings = PlayerHoldings(db);
	if (!holdings.empty()) {
		for (size_t i = 0; i < holdings.size() && i < 20; ++i) Logger::Log("tcp", "[AvaParity] p1213 player holding: %s\n", holdings[i].c_str());
		Logger::Log("tcp", "[AvaParity] p1213 AVA LAYOUT ABORT: %zu position rows belong to players; nothing cleared, server not started\n", holdings.size());
		std::fprintf(stderr, "AVA LAYOUT ABORT: %zu AvA position rows belong to players; nothing cleared, server not started\n", holdings.size());
		return false;
	}
	// Full copy next to the DB, taken before the first delete (rollback: tools in the patch notes).
	const std::string backup = DbFile(db) + ".pre-ava-12x30";
	std::remove(backup.c_str());  // a copy from an attempt that never cleared
	sqlite3_stmt* s = nullptr;
	bool saved = false;
	if (sqlite3_prepare_v2(db, "VACUUM INTO ?1", -1, &s, nullptr) == SQLITE_OK) {
		sqlite3_bind_text(s, 1, backup.c_str(), -1, SQLITE_TRANSIENT);
		saved = sqlite3_step(s) == SQLITE_DONE;
		sqlite3_finalize(s);
	}
	if (!saved) {
		Logger::Log("tcp", "[AvaParity] p1213 AVA LAYOUT ABORT: backup %s failed (%s); nothing cleared, server not started\n", backup.c_str(), sqlite3_errmsg(db));
		std::fprintf(stderr, "AVA LAYOUT ABORT: backup failed; nothing cleared, server not started\n");
		return false;
	}
	struct OpenBid { int64_t cid; uint32_t pos; int64_t amount; };
	std::vector<OpenBid> bids;
	for (const char* t : {"ga_ava_bids", "ga_ava_sealed_bids"}) {
		if (!TableExists(db, t)) continue;
		if (sqlite3_prepare_v2(db, (std::string("SELECT character_id, position, amount FROM ") + t).c_str(), -1, &s, nullptr) != SQLITE_OK) continue;
		while (sqlite3_step(s) == SQLITE_ROW) bids.push_back({sqlite3_column_int64(s, 0), (uint32_t)sqlite3_column_int(s, 1), sqlite3_column_int64(s, 2)});
		sqlite3_finalize(s);
	}
	static const char* kClear[] = {"ga_ava_territories", "ga_ava_buildings", "ga_ava_shields", "ga_ava_mine_resources", "ga_ava_bids",
		"ga_ava_lockdown", "ga_ava_shield_breaches", "ga_ava_hq", "ga_ava_attacks", "ga_ava_sealed_bids", "ga_ava_bid_windows",
		"ga_ava_missile_picks", "ga_ava_attack_types"};
	if (sqlite3_exec(db, "BEGIN IMMEDIATE", nullptr, nullptr, nullptr) != SQLITE_OK) return false;
	int rows = 0;
	for (const char* t : kClear) {
		if (!TableExists(db, t)) continue;
		if (sqlite3_exec(db, (std::string("DELETE FROM ") + t).c_str(), nullptr, nullptr, nullptr) != SQLITE_OK) {
			Logger::Log("tcp", "[AvaParity] p1213 AVA LAYOUT ABORT: clearing %s failed (%s); rolled back, server not started\n", t, sqlite3_errmsg(db));
			sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
			return false;
		}
		rows += sqlite3_changes(db);
	}
	sqlite3_exec(db, "INSERT OR REPLACE INTO cs_settings (key, value) VALUES ('ava_zone_layout', '12x30');"
		"INSERT OR IGNORE INTO cs_settings (key, value) VALUES ('ava_regions', 'on');", nullptr, nullptr, nullptr);
	if (sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr) != SQLITE_OK) {
		sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
		return false;
	}
	MailService::EnsureSchema();
	std::vector<int64_t> mailed;
	for (const auto& b : bids)
		if (b.amount > 0) Refund(db, b.cid, b.pos, b.amount, "the war map was redrawn.", mailed);
	Logger::Log("tcp", "[AvaParity] p1213 War Zone boards 12x30: %d position rows cleared, %zu bids refunded, backup %s\n", rows, mailed.size(), backup.c_str());
	return true;
}
}  // namespace

bool MigrateZoneLayout() {
	sqlite3* db = Database::GetConnection();
	if (!db) return true;
	EnsureSchema(db);
	ServiceAgents::EnsureSchema();
	sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS ga_ava_layout (zone_id INTEGER NOT NULL, position INTEGER NOT NULL, "
		"hex_building_id INTEGER NOT NULL, region INTEGER NOT NULL, PRIMARY KEY (zone_id, position))", nullptr, nullptr, nullptr);
	if (Setting(db, "ava_zone_layout", "") != kZoneLayoutMark && !ClearForNewBoards(db)) return false;
	if (sqlite3_exec(db, "BEGIN IMMEDIATE", nullptr, nullptr, nullptr) != SQLITE_OK) return true;
	sqlite3_exec(db, "DELETE FROM ga_ava_layout", nullptr, nullptr, nullptr);
	sqlite3_stmt* s = nullptr;
	if (sqlite3_prepare_v2(db, "INSERT INTO ga_ava_layout VALUES (?4, ?1, ?2, ?3)", -1, &s, nullptr) == SQLITE_OK) {
		for (int z = 1; z <= AvaZones::Count(); ++z) for (uint32_t l = 0; l < kZoneW * kZoneH; ++l) {
			const uint32_t p = AvaZones::Base((uint32_t)z) + l;
			sqlite3_bind_int(s, 4, z);
			sqlite3_bind_int(s, 1, (int)p); sqlite3_bind_int(s, 2, (int)AvaParityLayoutAt(p));
			sqlite3_bind_int(s, 3, AvaZones::Region((uint32_t)z, l));
			sqlite3_step(s); sqlite3_reset(s);
		}
		sqlite3_finalize(s);
	}
	sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr);
	return true;
}

bool RegionGate(sqlite3* db, int64_t character_id, uint32_t position, Action action, Gate& g) {
	if (action == kDefend || !RegionsOn(db)) return false;
	int64_t left = 0;
	const int open = OpenRegion((int64_t)std::time(nullptr), &left), mine = RegionOf(position);
	if (mine == open) return false;
	if (action == kAttack && Scalar(db, "SELECT 1 FROM ga_ava_bids WHERE zone_id = 1 AND position = ?1 AND owner_agency_id = ?2",
	        position, OwnerOfCharacter(db, character_id)) > 0)
		return false;  // a won bid may still launch
	if (action == kAttack && OpenBidOwnerFor(position, character_id) > 0) return false;  // opened won bid
	g.refuse = true;
	const int64_t opens = RegionSeconds(position);
	g.text = "AvA: territory " + AvaZones::Label(position) + " is in Region " + std::to_string(mine) + ". Only Region " +
		std::to_string(open) + " is open now (one region per hour); Region " + std::to_string(mine) + " opens in " +
		std::to_string(opens / 3600) + " h " + std::to_string((opens % 3600) / 60) + " min.";
	Logger::Log("tcp", "[AvaParity] char %lld pos %u action %d refused: region %d closed (open %d)\n", (long long)character_id,
		position, (int)action, mine, open);
	return true;
}

}  // namespace AvaParity
