#include "src/ControlServer/PlayerSessionStore/PlayerSessionStore.hpp"
#include "src/ControlServer/Database/Database.hpp"
#include "src/ControlServer/Loadouts/ArmorLoadouts.hpp"
#include "src/ControlServer/Loadouts/ClassLoadouts.hpp"
#include "src/ControlServer/Loadouts/CosmeticLoadouts.hpp"
#include "src/ControlServer/Loadouts/ModResolver.hpp"
#include "src/ControlServer/Logger.hpp"
#include "src/Shared/CosmeticSlots.hpp"
#include "sqlite3.h"
#include <cstdio>
#include <cstdlib>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <tuple>

std::mutex PlayerSessionStore::mutex_;
std::map<std::string, SessionInfo> PlayerSessionStore::by_guid_;
std::map<std::string, SessionInfo> PlayerSessionStore::by_ip_;

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

	by_guid_.clear();
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

int64_t PlayerSessionStore::InsertCharacter(int64_t user_id, uint32_t profile_id,
                                             uint32_t head_asm_id, uint32_t gender_type_value_id,
                                             const std::vector<uint8_t>& morph_data) {
	std::lock_guard<std::mutex> lock(mutex_);
	sqlite3* db = Database::GetConnection();

	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"INSERT INTO ga_characters (user_id, profile_id, head_asm_id, gender_type_value_id, morph_data) "
		"VALUES (?, ?, ?, ?, ?)",
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
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);

	int64_t newId = sqlite3_last_insert_rowid(db);
	// Seed (or top up) this user's account-scoped inventory pool — both for
	// this profile and any already-existing siblings. Also drops an opening
	// equipped loadout into ga_character_devices for the brand-new character.
	SeedInventoryFromLoadouts(user_id);
	return newId;
}

std::vector<CharacterInfo> PlayerSessionStore::GetCharactersByUserId(int64_t user_id) {
	std::lock_guard<std::mutex> lock(mutex_);
	sqlite3* db = Database::GetConnection();
	std::vector<CharacterInfo> result;

	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"SELECT id, profile_id, head_asm_id, gender_type_value_id, morph_data "
		"FROM ga_characters WHERE user_id = ?",
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
		"SELECT id, user_id, profile_id, head_asm_id, gender_type_value_id, morph_data "
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
		result = std::move(c);
	}
	sqlite3_finalize(stmt);
	return result;
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
			: "SELECT MIN(blueprint_id) FROM asm_data_set_blueprints "
			  "WHERE created_item_id = ?";
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

void PlayerSessionStore::SeedInventoryFromLoadouts(int64_t user_id) {
	sqlite3* db = Database::GetConnection();
	if (!db) return;

	// v76: seed cosmetic ownership (idempotent) before the device pass.
	// Cosmetic rows have item_id > 0 and are scoped out of the device
	// delete pass below via the `AND item_id = 0` clause.
	SeedCosmetics(user_id);

	// Armor seed — same shape as SeedCosmetics. Armor rows also have
	// item_id > 0 so they're scoped out of the device delete pass below by
	// the same `AND item_id = 0` clause; no separate guard needed.
	SeedArmor(user_id);

	// Build the "what should exist" set up front. Keyed by
	// (profile_id, device_id, mods_csv, quality, oc) — same tuple the insert
	// pass uses for its dedupe check. Used twice below: insert pass skips
	// tuples already in inventory, delete pass removes inventory rows whose
	// tuple isn't here anymore (= ClassLoadouts entry was deleted).
	struct ProfileLoadout { uint32_t profile_id; const std::vector<Loadouts::GearSlot>* loadout; };
	const ProfileLoadout profiles[] = {
		{ Loadouts::PROFILE_MEDIC,    &Loadouts::GetLoadout(Loadouts::PROFILE_MEDIC) },
		{ Loadouts::PROFILE_ROBOTICS, &Loadouts::GetLoadout(Loadouts::PROFILE_ROBOTICS) },
		{ Loadouts::PROFILE_ASSAULT,  &Loadouts::GetLoadout(Loadouts::PROFILE_ASSAULT) },
		{ Loadouts::PROFILE_RECON,    &Loadouts::GetLoadout(Loadouts::PROFILE_RECON) },
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

	// (2) For every character on this user that has ZERO rows in
	//     ga_character_devices, drop in an opening loadout pulled from this
	//     user's ga_players_inventory for that character's profile. The
	//     "no rows" gate keeps user equip choices sticky across logins.
	sqlite3_stmt* charsStmt = nullptr;
	rc = sqlite3_prepare_v2(db,
		"SELECT c.id, c.profile_id "
		"FROM ga_characters c "
		"WHERE c.user_id = ? AND NOT EXISTS ("
		"  SELECT 1 FROM ga_character_devices d WHERE d.character_id = c.id"
		")",
		-1, &charsStmt, nullptr);
	if (rc != SQLITE_OK || !charsStmt) {
		Logger::Log("db", "[PlayerSessionStore] Seed chars prepare failed: %s\n", sqlite3_errmsg(db));
		Logger::Log("db",
			"[PlayerSessionStore] Seed: user=%lld inventory+=%d inventory-=%d device-rows-cleared=%d\n",
			user_id, inserted, deletedInvRows, deletedDevRows);
		return;
	}
	sqlite3_bind_int64(charsStmt, 1, user_id);

	int equippedCharacters = 0;
	while (sqlite3_step(charsStmt) == SQLITE_ROW) {
		const int64_t  char_id    = sqlite3_column_int64(charsStmt, 0);
		const uint32_t profile_id = (uint32_t)sqlite3_column_int(charsStmt, 1);
		const auto& loadout = Loadouts::GetLoadout(profile_id);
		if (loadout.empty()) continue;

		// Map (device_id, mods_csv, quality, oc) → inventory_id for this
		// profile, so we can pick the inventory rows the seed just inserted.
		sqlite3_stmt* invStmt = nullptr;
		if (sqlite3_prepare_v2(db,
		    "SELECT id, device_id, mod_effect_group_ids, quality, oc "
		    "FROM ga_players_inventory "
		    "WHERE user_id = ? AND profile_id = ?",
		    -1, &invStmt, nullptr) != SQLITE_OK) continue;
		sqlite3_bind_int64(invStmt, 1, user_id);
		sqlite3_bind_int(invStmt,   2, (int)profile_id);

		std::map<std::tuple<int, std::string, int, int>, int> invIndex;
		while (sqlite3_step(invStmt) == SQLITE_ROW) {
			int id        = sqlite3_column_int(invStmt, 0);
			int dev       = sqlite3_column_int(invStmt, 1);
			const char* m = (const char*)sqlite3_column_text(invStmt, 2);
			int q         = sqlite3_column_int(invStmt, 3);
			int oc        = sqlite3_column_int(invStmt, 4);
			invIndex[{ dev, std::string(m ? m : ""), q, oc }] = id;
		}
		sqlite3_finalize(invStmt);

		sqlite3_stmt* insDev = nullptr;
		if (sqlite3_prepare_v2(db,
		    "INSERT OR IGNORE INTO ga_character_devices "
		    "(character_id, inventory_id, equipped_slot) VALUES (?, ?, ?)",
		    -1, &insDev, nullptr) != SQLITE_OK) continue;

		for (const Loadouts::GearSlot& g : loadout) {
			const std::string modsCsv = ModsToCsv(ModResolver::Resolve(g.device_id, g.quality, g.mods));
			auto it = invIndex.find({ g.device_id, modsCsv, g.quality, g.mods.oc ? 1 : 0 });
			if (it == invIndex.end()) continue;  // shouldn't happen — we just inserted these
			sqlite3_reset(insDev);
			sqlite3_clear_bindings(insDev);
			sqlite3_bind_int64(insDev, 1, char_id);
			sqlite3_bind_int(insDev,   2, it->second);
			sqlite3_bind_int(insDev,   3, g.equip_slot);
			sqlite3_step(insDev);
		}
		sqlite3_finalize(insDev);
		++equippedCharacters;
	}
	sqlite3_finalize(charsStmt);

	// (3) Pin slot 14 (CLASS_DEVICE) for every character that has equipped
	//     gear but no slot-14 row yet. This is the migration safety net for
	//     existing characters that ran with the old synthetic-invId path —
	//     their ga_character_devices has 5-8 rows but nothing in slot 14.
	//     New characters got slot 14 from the opening-loadout pass above; the
	//     equip-screen save also pins slot 14 via SaveEquippedDevices. This
	//     pass covers the "previously equipped, never re-saved since the
	//     synthetic-invId fix" gap.
	//
	//     If a character's profile has no class device candidate in
	//     ga_players_inventory the subquery yields NULL → INSERT fails the
	//     inventory_id NOT NULL constraint → OR IGNORE swallows it silently.
	int pinnedSlot14 = 0;
	{
		sqlite3_stmt* pinStmt = nullptr;
		const char* pinSql =
			"INSERT OR IGNORE INTO ga_character_devices (character_id, inventory_id, equipped_slot) "
			"SELECT c.id, "
			"       (SELECT i.id FROM ga_players_inventory i "
			"        WHERE i.user_id = c.user_id "
			"          AND (i.profile_id = 0 OR i.profile_id = c.profile_id) "
			"          AND i.allowed_slots = '502' "
			"        ORDER BY i.profile_id DESC LIMIT 1), "
			"       14 "
			"FROM ga_characters c "
			"WHERE c.user_id = ? "
			"  AND NOT EXISTS ("
			"    SELECT 1 FROM ga_character_devices d "
			"    WHERE d.character_id = c.id AND d.equipped_slot = 14"
			"  )";
		if (sqlite3_prepare_v2(db, pinSql, -1, &pinStmt, nullptr) == SQLITE_OK) {
			sqlite3_bind_int64(pinStmt, 1, user_id);
			sqlite3_step(pinStmt);
			pinnedSlot14 = sqlite3_changes(db);
			sqlite3_finalize(pinStmt);
		} else {
			Logger::Log("db",
				"[PlayerSessionStore] Seed slot-14 pin prepare failed: %s\n",
				sqlite3_errmsg(db));
		}
	}

	Logger::Log("db",
		"[PlayerSessionStore] Seed: user=%lld inventory+=%d inventory-=%d device-rows-cleared=%d characters opened with loadout=%d slot14-pinned=%d\n",
		user_id, inserted, deletedInvRows, deletedDevRows, equippedCharacters, pinnedSlot14);

	// v76: fill empty cosmetic slots on every character (idempotent per
	// the INSERT OR IGNORE in SeedCharacterCosmeticDefaults). Unlike the
	// device seed above, this runs for every character regardless of
	// whether the character has device equips — cosmetics have their own
	// equip slots (6/12/16-21) that exist independently of device slots.
	{
		sqlite3_stmt* allCharsStmt = nullptr;
		if (sqlite3_prepare_v2(db,
		    "SELECT id, profile_id FROM ga_characters WHERE user_id = ?",
		    -1, &allCharsStmt, nullptr) == SQLITE_OK) {
			sqlite3_bind_int64(allCharsStmt, 1, user_id);
			std::vector<std::pair<int64_t, uint32_t>> chars;
			while (sqlite3_step(allCharsStmt) == SQLITE_ROW) {
				chars.emplace_back(sqlite3_column_int64(allCharsStmt, 0),
				                   (uint32_t)sqlite3_column_int(allCharsStmt, 1));
			}
			sqlite3_finalize(allCharsStmt);
			for (const auto& [cid, pid] : chars) {
				SeedCharacterCosmeticDefaults(cid, pid);
				SeedCharacterArmorDefaults(cid);
			}
		}
	}
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
	const char* kSeedHelmsSuitsTrails =
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
		"WHERE i.item_type_value_id = 1612 "
		"   OR (i.item_type_value_id = 950 AND i.item_subtype_value_id IN (1006, 1007, 1008))";
	{
		sqlite3_stmt* stmt = nullptr;
		if (sqlite3_prepare_v2(db, kSeedHelmsSuitsTrails, -1, &stmt, nullptr) == SQLITE_OK) {
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
	const char* kSeedDyes =
		"INSERT OR IGNORE INTO ga_players_inventory"
		"  (user_id, profile_id, device_id, quality, mod_effect_group_ids, oc, allowed_slots, item_id, stock_n) "
		"SELECT ?, 0, 0, 0, '', 0, '996,997,998,999,1000', i.item_id, n.n "
		"FROM asm_data_set_items i "
		"CROSS JOIN (SELECT 0 AS n UNION SELECT 1 UNION SELECT 2 UNION SELECT 3 UNION SELECT 4) n "
		"WHERE i.item_type_value_id = 1020";
	{
		sqlite3_stmt* stmt = nullptr;
		if (sqlite3_prepare_v2(db, kSeedDyes, -1, &stmt, nullptr) == SQLITE_OK) {
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
	const char* kInsertEquip =
		"INSERT OR IGNORE INTO ga_character_devices (character_id, inventory_id, equipped_slot) "
		"VALUES (?, ?, ?)";

	for (const auto& sl : slots) {
		if (sl.item_id == 0) continue;

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
	//   quality        — 1162 (Q_EPIC; tier color in the client UI)
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
		for (int vi = 0; vi < ArmorLoadouts::kVariantCount; ++vi) {
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

	Logger::Log("armor-seed",
		"SeedArmor: user=%lld inserted=%d (of %d possible — rest already present)\n",
		(long long)user_id, inserted, ArmorLoadouts::kSlotCount * ArmorLoadouts::kVariantCount);
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
	const char* kInsertEquip =
		"INSERT OR IGNORE INTO ga_character_devices (character_id, inventory_id, equipped_slot) "
		"VALUES (?, ?, ?)";

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

std::vector<DeviceRow> PlayerSessionStore::GetDevicesForCharacter(int64_t character_id) {
	std::lock_guard<std::mutex> lock(mutex_);
	std::vector<DeviceRow> result;
	sqlite3* db = Database::GetConnection();
	if (!db) return result;

	// JOIN: device descriptor comes from the inventory pool; the per-character
	// row contributes just the equipped slot. SVID is derived from the slot
	// rather than stored (it's a function of equip_point, not of the item).
	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"SELECT d.equipped_slot, i.id, i.device_id, i.quality, "
		"       i.mod_effect_group_ids, i.oc, i.item_id "
		"FROM ga_character_devices d "
		"JOIN ga_players_inventory i ON i.id = d.inventory_id "
		"WHERE d.character_id = ? "
		"ORDER BY d.equipped_slot",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[PlayerSessionStore] GetDevicesForCharacter prepare failed: %s\n", sqlite3_errmsg(db));
		return result;
	}
	sqlite3_bind_int64(stmt, 1, character_id);

	while (sqlite3_step(stmt) == SQLITE_ROW) {
		DeviceRow row;
		row.equip_slot      = sqlite3_column_int(stmt, 0);
		row.inventory_id    = sqlite3_column_int(stmt, 1);
		row.device_id       = sqlite3_column_int(stmt, 2);
		row.quality         = sqlite3_column_int(stmt, 3);
		row.mods            = CsvToMods((const char*)sqlite3_column_text(stmt, 4));
		row.oc              = sqlite3_column_int(stmt, 5) != 0;
		row.item_id         = sqlite3_column_int(stmt, 6);
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
                                              const std::map<int, int>& slot_to_inventory,
                                              const std::map<int, int>& misc_items) {
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

	// Validate every (equip_point, inventory_id) BEFORE we mutate. The whole
	// save either lands or doesn't — partial writes would desync the client
	// from server in confusing ways.
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

	for (const auto& kv : save_map) {
		const int equip_point = kv.first;
		const int inventory_id = kv.second;
		sqlite3_reset(vstmt);
		sqlite3_clear_bindings(vstmt);
		sqlite3_bind_int(vstmt, 1, inventory_id);
		if (sqlite3_step(vstmt) != SQLITE_ROW) {
			Logger::Log("armor",
				"[Save] REJECT char=%lld: inventory_id=%d not found\n",
				character_id, inventory_id);
			sqlite3_finalize(vstmt);
			return false;
		}
		const int64_t inv_user    = sqlite3_column_int64(vstmt, 0);
		const int     inv_profile = sqlite3_column_int(vstmt, 1);
		const std::vector<int> allowed = CsvToInts((const char*)sqlite3_column_text(vstmt, 2));
		const int     inv_device  = sqlite3_column_int(vstmt, 3);
		const int     inv_itemId  = sqlite3_column_int(vstmt, 4);
		const int     inv_subt    = sqlite3_column_int(vstmt, 5);
		if (inv_user != user_id) {
			Logger::Log("armor",
				"[Save] REJECT char=%lld: inventory_id=%d owned by user %lld, not %lld\n",
				character_id, inventory_id, inv_user, user_id);
			sqlite3_finalize(vstmt);
			return false;
		}
		if (inv_profile != 0 && inv_profile != (int)profile_id) {
			Logger::Log("armor",
				"[Save] REJECT char=%lld: inventory_id=%d profile=%d, char profile=%u\n",
				character_id, inventory_id, inv_profile, profile_id);
			sqlite3_finalize(vstmt);
			return false;
		}
		const int target_svid = EquipPointToSvid(equip_point);
		bool slotAllowed = false;
		for (int s : allowed) { if (s == target_svid) { slotAllowed = true; break; } }
		if (!slotAllowed) {
			Logger::Log("armor",
				"[Save] REJECT char=%lld: inventory_id=%d svid=%d not in allowed_slots\n",
				character_id, inventory_id, target_svid);
			sqlite3_finalize(vstmt);
			return false;
		}
		// Stash subtype for cosmetic rows (deviceId=0, itemId>0). Non-cosmetic
		// rows get subtype 0 so DbSlotFor passes through.
		inv_subtype[inventory_id] = (inv_device == 0 && inv_itemId > 0) ? inv_subt : 0;
	}
	sqlite3_finalize(vstmt);

	// All validation passed. Replace ga_character_devices for this character
	// in a single transaction.
	//
	// IMPORTANT: the DELETE is full-character. Any cosmetic rows whose engine
	// slot wasn't in this IPC payload would normally vanish here — but the
	// equip-screen client appears to ship the full current loadout (both gear
	// and appearance) on every Apply, so the rebuild covers them. The remap
	// in the INSERT pass ensures cosmetic suit/helmet land at DB slot 22/23,
	// leaving DB slot 6/12 free for the gameplay suit/helmet rows the client
	// also shipped.
	char* err = nullptr;
	sqlite3_exec(db, "BEGIN", nullptr, nullptr, &err);

	// Scope the DELETE to slots the client's SlotIndices[] actually controls
	// (engine equip-points 1–24 + cosmetic DB-slots 22/23). Armor slots
	// (group-129 SVIDs 1130/1132/1133/1136/1139/1142/1143) move through the
	// equip screen's separate Armor tab via FTGEQUIP_SLOTS_STRUCT.MiscItems[]
	// and are handled by the misc_items pass below. Without this gate, the
	// SlotIndices INSERT pass would clobber armor on every weapon/cosmetic
	// Apply because the client never includes armor in SlotIndices.
	sqlite3_stmt* del = nullptr;
	if (sqlite3_prepare_v2(db,
	    "DELETE FROM ga_character_devices "
	    "WHERE character_id = ? "
	    "  AND equipped_slot NOT IN (1130, 1132, 1133, 1136, 1139, 1142, 1143)",
	    -1, &del, nullptr) != SQLITE_OK) {
		sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, &err);
		return false;
	}
	sqlite3_bind_int64(del, 1, character_id);
	sqlite3_step(del);
	sqlite3_finalize(del);

	sqlite3_stmt* ins = nullptr;
	if (sqlite3_prepare_v2(db,
	    "INSERT INTO ga_character_devices (character_id, inventory_id, equipped_slot) "
	    "VALUES (?, ?, ?)",
	    -1, &ins, nullptr) != SQLITE_OK) {
		sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, &err);
		return false;
	}
	for (const auto& kv : save_map) {
		const int engine_slot = kv.first;
		const int inv_id      = kv.second;
		const int subtype     = inv_subtype[inv_id];
		const int db_slot     = CosmeticSlots::DbSlotFor(engine_slot, subtype);
		sqlite3_reset(ins);
		sqlite3_clear_bindings(ins);
		sqlite3_bind_int64(ins, 1, character_id);
		sqlite3_bind_int(ins,   2, inv_id);
		sqlite3_bind_int(ins,   3, db_slot);
		sqlite3_step(ins);
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
		if (sqlite3_prepare_v2(db,
		    "DELETE FROM ga_character_devices "
		    "WHERE character_id = ? "
		    "  AND equipped_slot IN (1130, 1132, 1133, 1136, 1139, 1142, 1143)",
		    -1, &delArmor, nullptr) == SQLITE_OK) {
			sqlite3_bind_int64(delArmor, 1, character_id);
			sqlite3_step(delArmor);
			sqlite3_finalize(delArmor);
		}

		sqlite3_stmt* insArmor = nullptr;
		if (sqlite3_prepare_v2(db,
		    "INSERT INTO ga_character_devices (character_id, inventory_id, equipped_slot) "
		    "VALUES (?, ?, ?)",
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
				sqlite3_reset(insArmor);
				sqlite3_clear_bindings(insArmor);
				sqlite3_bind_int64(insArmor, 1, character_id);
				sqlite3_bind_int  (insArmor, 2, inv_id);
				sqlite3_bind_int  (insArmor, 3, armor_svid);
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

	sqlite3_exec(db, "COMMIT", nullptr, nullptr, &err);
	Logger::Log("armor",
		"[Save] OK char=%lld user=%lld profile=%u entries=%zu armor=%d\n",
		character_id, user_id, profile_id, save_map.size(), armorEquipped);
	return true;
}

std::vector<SkillRow> PlayerSessionStore::GetSkillsForCharacter(int64_t character_id) {
	std::lock_guard<std::mutex> lock(mutex_);
	sqlite3* db = Database::GetConnection();
	std::vector<SkillRow> result;

	sqlite3_stmt* stmt = nullptr;
	int rc = sqlite3_prepare_v2(db,
		"SELECT skill_group_id, skill_id, points "
		"FROM ga_character_skills WHERE character_id = ? AND points > 0 "
		"ORDER BY skill_group_id, skill_id",
		-1, &stmt, nullptr);
	if (rc != SQLITE_OK || !stmt) {
		Logger::Log("db", "[PlayerSessionStore] GetSkillsForCharacter prepare failed: %s\n", sqlite3_errmsg(db));
		return result;
	}
	sqlite3_bind_int64(stmt, 1, character_id);

	while (sqlite3_step(stmt) == SQLITE_ROW) {
		SkillRow row;
		row.skill_group_id = sqlite3_column_int(stmt, 0);
		row.skill_id       = sqlite3_column_int(stmt, 1);
		row.points         = sqlite3_column_int(stmt, 2);
		result.push_back(row);
	}
	sqlite3_finalize(stmt);
	return result;
}

void PlayerSessionStore::SetSkillsForCharacter(int64_t character_id, const std::vector<SkillRow>& skills) {
	std::lock_guard<std::mutex> lock(mutex_);
	sqlite3* db = Database::GetConnection();

	char* err = nullptr;
	if (sqlite3_exec(db, "BEGIN", nullptr, nullptr, &err) != SQLITE_OK) {
		Logger::Log("db", "[PlayerSessionStore] SetSkillsForCharacter begin failed: %s\n", err);
		sqlite3_free(err);
		return;
	}

	sqlite3_stmt* del = nullptr;
	sqlite3_prepare_v2(db, "DELETE FROM ga_character_skills WHERE character_id = ?", -1, &del, nullptr);
	if (del) {
		sqlite3_bind_int64(del, 1, character_id);
		sqlite3_step(del);
		sqlite3_finalize(del);
	}

	sqlite3_stmt* ins = nullptr;
	sqlite3_prepare_v2(db,
		"INSERT INTO ga_character_skills (character_id, skill_group_id, skill_id, points) "
		"VALUES (?, ?, ?, ?)",
		-1, &ins, nullptr);
	if (ins) {
		for (const auto& s : skills) {
			if (s.points <= 0) continue;  // skip zero rows — not worth persisting
			sqlite3_bind_int64(ins, 1, character_id);
			sqlite3_bind_int(ins, 2, s.skill_group_id);
			sqlite3_bind_int(ins, 3, s.skill_id);
			sqlite3_bind_int(ins, 4, s.points);
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

	Logger::Log("db", "[PlayerSessionStore] Saved %d skill rows for character %lld\n",
		(int)skills.size(), (long long)character_id);
}

void PlayerSessionStore::ClearSkillsForCharacter(int64_t character_id) {
	SetSkillsForCharacter(character_id, {});
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
