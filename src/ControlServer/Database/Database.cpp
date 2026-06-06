#include "src/ControlServer/Database/Database.hpp"
#include "src/ControlServer/Logger.hpp"
#include "sqlite3.h"
#include <string>
#include <vector>
#include <map>

sqlite3* Database::connection = nullptr;
std::string Database::db_path_ = "server.db";

void Database::SetDbPath(const std::string& path) { db_path_ = path; }

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
		Logger::Log("db", "[Database::Init] Failed to open database\n");
		return;
	}

	char* err = nullptr;
	int result = 0;

	result = sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS version_info (version INTEGER DEFAULT 0)", nullptr, nullptr, &err);
	if (result != SQLITE_OK) {
		Logger::Log("db", "Failed to create version_info table: %s\n", err);
		return;
	}

	int version = 0;

	std::vector<std::map<std::string, std::string>> data;
	result = sqlite3_exec(db, "SELECT version FROM version_info LIMIT 1", Callback, &data, &err);
	if (result != SQLITE_OK) {
		Logger::Log("db", "Failed to check version_info table: %s\n", err);
		return;
	}

	if (data.size() == 0) {
		version = 0;

		result = sqlite3_exec(db, "INSERT INTO version_info (version) VALUES (0)", nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to insert version_info: %s\n", err);
			return;
		}
	} else {
		version = std::stoi(data[0]["version"]);
	}

	if (version < 1) {
		result = sqlite3_exec(db,
			"CREATE TABLE IF NOT EXISTS asm_data_set_bots ( \
				id INTEGER PRIMARY KEY AUTOINCREMENT, \
				bot_id INTEGER, \
				reference_name TEXT, \
				name_msg_id INTEGER, \
				desc_msg_id INTEGER, \
				level INTEGER, \
				pawn_class_res_id INTEGER, \
				controller_class_res_id INTEGER, \
				behavior_id INTEGER, \
				head_asm_id INTEGER, \
				body_asm_id INTEGER, \
				movement_asm_id INTEGER, \
				hit_points INTEGER, \
				bot_type_value_id INTEGER, \
				physical_type_value_id INTEGER, \
				default_slot_value_id INTEGER \
			);\
		", nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to create asm_data_set_bots table: %s\n", err);
			return;
		}

		result = sqlite3_exec(db,
			"CREATE TABLE IF NOT EXISTS asm_data_set_bot_spawn_tables ( \
				id INTEGER PRIMARY KEY AUTOINCREMENT, \
				bot_spawn_table_id INTEGER, \
				difficulty_value_id INTEGER, \
				player_profile_id INTEGER, \
				spawn_group INTEGER, \
				enemy_bot_id INTEGER, \
				bot_count INTEGER, \
				spawn_chance FLOAT, \
				team_size INTEGER, \
				multiple_class_flag INTEGER, \
				bot_balance_multiplier FLOAT, \
				spawn_group_min INTEGER, \
				spawn_group_max INTEGER, \
				spawn_group_respawn_sec INTEGER \
			); \
		", nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to create asm_data_set_bot_spawn_tables table: %s\n", err);
			return;
		}
	}

	if (version < 2) {
		result = sqlite3_exec(db,
			"CREATE TABLE IF NOT EXISTS obj_bot_factories ( \
				id INTEGER PRIMARY KEY AUTOINCREMENT, \
				map_object_id INTEGER, \
				bot_spawn_table_id INTEGER, \
				task_force_number INTEGER, \
				mutator_number INTEGER \
			); \
		", nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to create obj_bot_factories table: %s\n", err);
			return;
		}
	}

	if (version < 3) {
		result = sqlite3_exec(db,
			"insert into obj_bot_factories (map_object_id, bot_spawn_table_id, task_force_number, mutator_number) values \
				(13639, 147, 1, 0), \
				(13632, 86 , 1, 0), \
				(13629, 99 , 1, 0), \
				(13638, 148, 1, 0), \
				(13647, 87 , 1, 0), \
				(13630, 148, 1, 0), \
				(13634, 148, 1, 0), \
				(13633, 99 , 1, 0), \
				(13637, 148, 1, 0), \
				(13635, 102, 1, 0), \
				(13636, 99 , 1, 0), \
				(13640, 148, 1, 0), \
				(13642, 86 , 1, 0), \
				(13641, 99 , 1, 0), \
				(13810, 102, 1, 0), \
				(13645, 99 , 1, 0), \
				(13644, 86 , 1, 0), \
				(13646, 99 , 1, 0), \
				(13664, 99 , 1, 0), \
				(13648, 99 , 1, 0), \
				(13650, 148, 1, 0), \
				(13704, 86 , 1, 0), \
				(13652, 148, 1, 0), \
				(13651, 99 , 1, 0), \
				(13694, 102, 1, 0), \
				(13659, 102, 1, 0), \
				(13661, 86 , 1, 0), \
				(13655, 99 , 1, 0), \
				(13656, 99 , 1, 0), \
				(13657, 87 , 1, 0), \
				(13660, 99 , 1, 0), \
				(13654, 99 , 1, 0), \
				(13692, 147, 1, 0), \
				(13708, 86 , 1, 0), \
				(13643, 148, 1, 0), \
				(13709, 102, 1, 0), \
				(13673, 149, 1, 0), \
				(13805, 102, 1, 0), \
				(13691, 87 , 1, 0), \
				(13658, 87 , 1, 0), \
				(13703, 149, 1, 0), \
				(13662, 149, 1, 0), \
				(13649, 99 , 1, 0), \
				(13665, 102, 1, 0), \
				(13802, 102, 1, 0), \
				(13804, 102, 1, 0), \
				(13803, 102, 1, 0), \
				(13700, 102, 1, 0), \
				(13806, 102, 1, 0), \
				(13653, 149, 1, 0), \
				(13849, 166, 2, 0), \
				(13846, 166, 2, 0), \
				(13847, 166, 2, 0), \
				(13848, 166, 2, 0),  \
				\
				(12712, 29, 2, 0), \
				(12708, 29, 2, 0), \
				(12710, 58, 2, 0), \
				(12711, 29, 2, 0), \
				(12714, 34, 2, 0), \
				(12713, 34, 2, 0), \
				(12709, 28, 2, 0), \
				(12698, 41, 2, 0); \
			", nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to insert obj_bot_factories: %s\n", err);
			return;
		}
	}

	if (version < 4) {
		result = sqlite3_exec(db,
			"CREATE TABLE IF NOT EXISTS asm_data_set_bots_data_set_bot_devices ( \
				id INTEGER PRIMARY KEY AUTOINCREMENT, \
				bot_id INTEGER, \
				device_id INTEGER, \
				slot_used_value_id INTEGER \
			);", nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to create asm_data_set_bots_data_set_bot_devices table: %s\n", err);
			return;
		}
	}

	if (version < 5) {
		result = sqlite3_exec(db,
			"CREATE TABLE IF NOT EXISTS asm_data_set_devices ( \
				id INTEGER PRIMARY KEY AUTOINCREMENT, \
				device_id INTEGER, \
				form_class_res_id INTEGER, \
				mount_socket_res_id INTEGER, \
				time_to_equip_secs FLOAT, \
				container_skill_group_id INTEGER, \
				right_click_behavior_type_value_id INTEGER, \
				slot_used_value_id INTEGER \
			);", nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to create asm_data_set_devices table: %s\n", err);
			return;
		}

		result = sqlite3_exec(db,
			"CREATE TABLE IF NOT EXISTS asm_data_set_items ( \
				id INTEGER PRIMARY KEY AUTOINCREMENT, \
				name_msg_id INTEGER, \
				name_msg_translated TEXT, \
				desc_msg_id INTEGER, \
				desc_msg_translated TEXT, \
				class_res_id INTEGER, \
				item_id INTEGER, \
				item_type_value_id INTEGER, \
				item_subtype_value_id INTEGER, \
				skill_id INTEGER, \
				sub_skill_id INTEGER, \
				skill_level_min INTEGER, \
				quantity INTEGER, \
				icon_id INTEGER, \
				weight FLOAT, \
				time_to_live_secs FLOAT, \
				quality_value_id INTEGER, \
				required_achievement_id INTEGER, \
				required_achievement_points INTEGER, \
				ref_bot_id INTEGER, \
				ref_deployable_id INTEGER, \
				ref_device_id INTEGER, \
				item_bind_type_value_id INTEGER, \
				production_cost INTEGER, \
				required_level INTEGER, \
				purchased_value INTEGER, \
				bundle_loot_table_id INTEGER \
			); \
		", nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to create asm_data_set_items table: %s\n", err);
			return;
		}
	}

	if (version < 6) {
		result = sqlite3_exec(db,
			"CREATE TABLE IF NOT EXISTS asm_data_set_devices_data_set_device_modes ( \
				id INTEGER PRIMARY KEY AUTOINCREMENT, \
				device_id INTEGER, \
				device_mode_id INTEGER, \
				name_msg_id INTEGER, \
				name_msg_translated TEXT \
			);", nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to create asm_data_set_devices_data_set_device_modes table: %s\n", err);
			return;
		}
	}

	if (version < 7) {
		result = sqlite3_exec(db,
			"CREATE TABLE IF NOT EXISTS obj_mission_objective_bots ( \
				id INTEGER PRIMARY KEY AUTOINCREMENT, \
				map_object_id INTEGER, \
				bot_factory_id INTEGER \
			);", nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to create obj_mission_objective_bots table: %s\n", err);
			return;
		}

		result = sqlite3_exec(db,
			"INSERT INTO obj_mission_objective_bots (map_object_id, bot_factory_id) VALUES \
				(11324, 12698); \
			", nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to insert obj_mission_objective_bots: %s\n", err);
			return;
		}
	}

	if (version < 8) {
		result = sqlite3_exec(db,
			"CREATE TABLE IF NOT EXISTS asm_data_set_effect_groups ( \
				id INTEGER PRIMARY KEY AUTOINCREMENT, \
				effect_group_id INTEGER, \
				lifetime_sec REAL, \
				apply_interval_sec REAL, \
				target_fx_id INTEGER, \
				fx_display_group_res_id INTEGER, \
				icon_id INTEGER, \
				application_value_id INTEGER, \
				category_value_id INTEGER, \
				application_value REAL, \
				application_chance REAL, \
				situational_type_value_id INTEGER, \
				required_category_value_id INTEGER, \
				required_skill_id INTEGER, \
				effect_group_type_value_id INTEGER, \
				health INTEGER, \
				situational_value REAL, \
				stack_count_max INTEGER, \
				buff_value INTEGER, \
				contagion_flag INTEGER, \
				device_specific_flag INTEGER, \
				posture_type_value_id INTEGER \
			);", nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to create asm_data_set_effect_groups table: %s\n", err);
			return;
		}

		result = sqlite3_exec(db,
			"CREATE TABLE IF NOT EXISTS asm_data_set_effects ( \
				id INTEGER PRIMARY KEY AUTOINCREMENT, \
				effect_group_id INTEGER, \
				effect_id INTEGER, \
				class_res_id INTEGER, \
				base_value REAL, \
				min_value REAL, \
				max_value REAL, \
				calc_method_value_id INTEGER, \
				prop_id INTEGER, \
				property_value_id INTEGER, \
				apply_on_interval_flag INTEGER \
			);", nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to create asm_data_set_effects table: %s\n", err);
			return;
		}
	}

	if (version < 9) {
		result = sqlite3_exec(db,
			"ALTER TABLE asm_data_set_bots ADD COLUMN default_sensor_range REAL; \
			ALTER TABLE asm_data_set_bots ADD COLUMN default_aggro_range INTEGER; \
			ALTER TABLE asm_data_set_bots ADD COLUMN default_help_range INTEGER; \
			ALTER TABLE asm_data_set_bots ADD COLUMN hearing_range REAL; \
			ALTER TABLE asm_data_set_bots ADD COLUMN default_speed REAL; \
			ALTER TABLE asm_data_set_bots ADD COLUMN walk_speed_pct REAL; \
			ALTER TABLE asm_data_set_bots ADD COLUMN crouch_speed_pct REAL; \
			ALTER TABLE asm_data_set_bots ADD COLUMN chase_range INTEGER; \
			ALTER TABLE asm_data_set_bots ADD COLUMN chase_time_sec REAL; \
			ALTER TABLE asm_data_set_bots ADD COLUMN stealth_sensor_range INTEGER; \
			ALTER TABLE asm_data_set_bots ADD COLUMN stealth_aggro_range INTEGER; \
			ALTER TABLE asm_data_set_bots ADD COLUMN hibernate_on_idle_sec INTEGER; \
			ALTER TABLE asm_data_set_bots ADD COLUMN hibernate_delay_rate REAL; \
			ALTER TABLE asm_data_set_bots ADD COLUMN icon_id INTEGER; \
			ALTER TABLE asm_data_set_bots ADD COLUMN bot_rank_value_id INTEGER; \
			ALTER TABLE asm_data_set_bots ADD COLUMN target_only_physical_type_value_id INTEGER; \
			ALTER TABLE asm_data_set_bots ADD COLUMN skill_group_id INTEGER; \
			ALTER TABLE asm_data_set_bots ADD COLUMN skill_group_set_id INTEGER; \
			ALTER TABLE asm_data_set_bots ADD COLUMN fixed_fov_degrees INTEGER; \
			ALTER TABLE asm_data_set_bots ADD COLUMN loot_table_id INTEGER; \
			ALTER TABLE asm_data_set_bots ADD COLUMN default_power_pool INTEGER; \
			ALTER TABLE asm_data_set_bots ADD COLUMN rotation_rate INTEGER; \
			ALTER TABLE asm_data_set_bots ADD COLUMN class_type_value_id INTEGER; \
			ALTER TABLE asm_data_set_bots ADD COLUMN device_slot_unlock_group_id INTEGER; \
			ALTER TABLE asm_data_set_bots ADD COLUMN pickup_device_id INTEGER; \
			ALTER TABLE asm_data_set_bots ADD COLUMN xp_value INTEGER; \
			ALTER TABLE asm_data_set_bots ADD COLUMN currency_value INTEGER; \
			ALTER TABLE asm_data_set_bots ADD COLUMN squad_role_value_id INTEGER; \
			ALTER TABLE asm_data_set_bots ADD COLUMN default_posture_value_id INTEGER; \
			ALTER TABLE asm_data_set_bots ADD COLUMN acceleration_rate REAL; \
			ALTER TABLE asm_data_set_bots ADD COLUMN accuracy_override REAL; \
			ALTER TABLE asm_data_set_bots ADD COLUMN bot_balance_multiplier REAL; \
			ALTER TABLE asm_data_set_bots ADD COLUMN power_pool_regen_per_sec REAL; \
			ALTER TABLE asm_data_set_bots ADD COLUMN hibernate_invulnerability_flag INTEGER; \
			ALTER TABLE asm_data_set_bots ADD COLUMN can_jump_flag INTEGER; \
			ALTER TABLE asm_data_set_bots ADD COLUMN can_climb_ladders_flag INTEGER; \
			ALTER TABLE asm_data_set_bots ADD COLUMN path_only_flag INTEGER; \
			ALTER TABLE asm_data_set_bots ADD COLUMN always_load_on_server_flag INTEGER; \
			ALTER TABLE asm_data_set_bots ADD COLUMN destroy_on_owner_death_flag INTEGER; \
		", nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to migrate asm_data_set_bots to v9: %s\n", err);
			return;
		}
	}

	if (version < 10) {
		result = sqlite3_exec(db,
			"CREATE TABLE IF NOT EXISTS ga_players ( \
				session_guid TEXT PRIMARY KEY, \
				player_name TEXT NOT NULL, \
				ip_address TEXT NOT NULL, \
				created_at INTEGER NOT NULL, \
				last_seen_at INTEGER NOT NULL \
			);",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to create ga_players table: %s\n", err);
			return;
		}
	}

	if (version < 11) {
		result = sqlite3_exec(db,
			"CREATE TABLE IF NOT EXISTS ga_users ( \
				id INTEGER PRIMARY KEY AUTOINCREMENT, \
				username TEXT UNIQUE NOT NULL \
			);",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to create ga_users table: %s\n", err);
			return;
		}

		result = sqlite3_exec(db,
			"CREATE TABLE IF NOT EXISTS ga_characters ( \
				id INTEGER PRIMARY KEY AUTOINCREMENT, \
				user_id INTEGER NOT NULL REFERENCES ga_users(id), \
				profile_id INTEGER NOT NULL, \
				head_asm_id INTEGER NOT NULL, \
				gender_type_value_id INTEGER NOT NULL, \
				morph_data BLOB \
			);",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to create ga_characters table: %s\n", err);
			return;
		}
	}

	if (version < 12) {
		result = sqlite3_exec(db,
			"ALTER TABLE ga_players ADD COLUMN user_id INTEGER;",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to add user_id to ga_players: %s\n", err);
			return;
		}
	}

	if (version < 13) {
		result = sqlite3_exec(db,
			"CREATE TABLE IF NOT EXISTS ga_character_quests ( \
				id INTEGER PRIMARY KEY AUTOINCREMENT, \
				character_id INTEGER NOT NULL REFERENCES ga_characters(id), \
				quest_id INTEGER NOT NULL, \
				status TEXT NOT NULL DEFAULT 'active', \
				accepted_at INTEGER NOT NULL DEFAULT (strftime('%s','now')), \
				completed_at INTEGER, \
				UNIQUE(character_id, quest_id) \
			);",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to create ga_character_quests table: %s\n", err);
			return;
		}
	}

	if (version < 14) { // tutorial inception bots
		result = sqlite3_exec(db,
			"insert into obj_bot_factories (map_object_id, bot_spawn_table_id, task_force_number, mutator_number) values \
				(9832, 53, 2, 0), \
				(9833, 53, 2, 0), \
				(11812, 53, 2, 0), \
				(12627, 53, 2, 0), \
				(12628, 53, 2, 0); \
			", nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to insert obj_bot_factories: %s\n", err);
			return;
		}
	}

	if (version < 15) {
		result = sqlite3_exec(db,
			"CREATE TABLE IF NOT EXISTS ga_instances ( \
				id           INTEGER PRIMARY KEY AUTOINCREMENT, \
				map_name     TEXT    NOT NULL, \
				state        TEXT    NOT NULL DEFAULT 'STARTING', \
				pid          INTEGER NOT NULL DEFAULT 0, \
				udp_port     INTEGER NOT NULL, \
				ip_address   TEXT    NOT NULL DEFAULT '127.0.0.1', \
				player_count INTEGER NOT NULL DEFAULT 0, \
				started_at   INTEGER NOT NULL DEFAULT (strftime('%s','now')), \
				sealed_at    INTEGER \
			);",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to create ga_instances table: %s\n", err);
			return;
		}
		result = sqlite3_exec(db,
			"CREATE INDEX IF NOT EXISTS idx_ga_instances_state ON ga_instances(state);",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to create ga_instances index: %s\n", err);
			return;
		}
	}

	if (version < 16) {
		result = sqlite3_exec(db,
			"ALTER TABLE ga_instances ADD COLUMN instance_id INTEGER NOT NULL DEFAULT 0;",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to add instance_id column: %s\n", err);
			sqlite3_free(err);
			// Column may already exist -- continue
		}
		result = sqlite3_exec(db,
			"ALTER TABLE ga_instances ADD COLUMN is_home_map INTEGER NOT NULL DEFAULT 0;",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to add is_home_map column: %s\n", err);
			sqlite3_free(err);
		}
		result = sqlite3_exec(db,
			"ALTER TABLE ga_instances ADD COLUMN max_players INTEGER NOT NULL DEFAULT 0;",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to add max_players column: %s\n", err);
			sqlite3_free(err);
		}
		result = sqlite3_exec(db,
			"ALTER TABLE ga_instances ADD COLUMN game_mode TEXT NOT NULL DEFAULT '';",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to add game_mode column: %s\n", err);
			sqlite3_free(err);
		}
		result = sqlite3_exec(db,
			"CREATE UNIQUE INDEX IF NOT EXISTS idx_ga_instances_instance_id "
			"ON ga_instances(instance_id) WHERE instance_id != 0;",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to create instance_id index: %s\n", err);
			sqlite3_free(err);
		}
	}

	if (version < 17) {
		result = sqlite3_exec(db,
			"CREATE TABLE IF NOT EXISTS ga_character_devices ("
			"  id            INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  character_id  INTEGER NOT NULL REFERENCES ga_characters(id),"
			"  device_id     INTEGER NOT NULL,"
			"  equip_slot    INTEGER NOT NULL,"
			"  slot_value_id INTEGER NOT NULL,"
			"  quality       INTEGER NOT NULL DEFAULT 0,"
			"  inventory_id  INTEGER NOT NULL,"
			"  effect_group_id INTEGER NOT NULL DEFAULT 0"
			");",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to create ga_character_devices table: %s\n", err);
			sqlite3_free(err);
			return;
		}
	}

	if (version < 18) {
		result = sqlite3_exec(db,
			"CREATE TABLE IF NOT EXISTS ga_instance_players ("
			"  id                INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  instance_id       INTEGER NOT NULL,"
			"  session_guid      TEXT NOT NULL,"
			"  character_id      INTEGER,"
			"  task_force_number INTEGER NOT NULL DEFAULT 1,"
			"  joined_at         INTEGER NOT NULL DEFAULT (strftime('%s','now')),"
			"  left_at           INTEGER DEFAULT NULL,"
			"  UNIQUE(instance_id, session_guid)"
			");",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to create ga_instance_players table: %s\n", err);
			sqlite3_free(err);
			return;
		}
		result = sqlite3_exec(db,
			"CREATE INDEX IF NOT EXISTS idx_ga_instance_players_instance "
			"ON ga_instance_players(instance_id) WHERE left_at IS NULL;",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to create ga_instance_players index: %s\n", err);
			sqlite3_free(err);
		}
		result = sqlite3_exec(db,
			"ALTER TABLE ga_instances ADD COLUMN last_empty_at INTEGER DEFAULT NULL;",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to add last_empty_at column: %s\n", err);
			sqlite3_free(err);
		}
	}

	// NOTE: ga_character_skills migration runs UNCONDITIONALLY, not gated by
	// `version < N`. Reason: the game-server DLL's Database::Init
	// (src/Database/Database.cpp) runs independent migrations against the same
	// shared server.db and bumps `version_info.version` to its own target
	// (currently 24). Both servers share the same version counter; control-
	// server migrations gated by a lower version number get silently skipped
	// on subsequent boots when the game DLL has already bumped past them. All
	// three statements below are idempotent (CREATE IF NOT EXISTS / ALTER with
	// error-swallow) so running every boot is cheap and safe.
	{
		result = sqlite3_exec(db,
			"CREATE TABLE IF NOT EXISTS ga_character_skills ("
			"  id             INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  character_id   INTEGER NOT NULL REFERENCES ga_characters(id),"
			"  skill_group_id INTEGER NOT NULL,"
			"  skill_id       INTEGER NOT NULL,"
			"  points         INTEGER NOT NULL DEFAULT 0,"
			"  UNIQUE(character_id, skill_group_id, skill_id)"
			");",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to create ga_character_skills table: %s\n", err);
			sqlite3_free(err);
			return;
		}
		result = sqlite3_exec(db,
			"CREATE INDEX IF NOT EXISTS idx_ga_character_skills_char "
			"ON ga_character_skills(character_id);",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to create ga_character_skills index: %s\n", err);
			sqlite3_free(err);
		}
		// ALTER TABLE ADD COLUMN fails once the column exists — swallow that
		// error so we stay idempotent across boots.
		result = sqlite3_exec(db,
			"ALTER TABLE ga_characters ADD COLUMN last_respec_at INTEGER NOT NULL DEFAULT 0;",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			// Expected on second+ boot — log at debug level rather than error.
			sqlite3_free(err);
		}

		// hair_asm_id / skin_mat_param_id / eye_mat_param_id — mirror of v81
		// in the game-DLL migration ladder. Added here so the control server
		// can SELECT these columns at first-boot before any game-DLL
		// instance has had a chance to run its own migrations. Both sides
		// run the same idempotent ALTER; whoever wins the race adds the
		// column, the other no-ops. DEFAULT 0 = bald (engine treats hair
		// asm 0 as "no hair", the only crash-safe fallback for legacy chars).
		const char* kAppearanceAlters[] = {
			"ALTER TABLE ga_characters ADD COLUMN hair_asm_id        INTEGER NOT NULL DEFAULT 0;",
			"ALTER TABLE ga_characters ADD COLUMN skin_mat_param_id  INTEGER NOT NULL DEFAULT 0;",
			"ALTER TABLE ga_characters ADD COLUMN eye_mat_param_id   INTEGER NOT NULL DEFAULT 0;",
		};
		for (const char* sql : kAppearanceAlters) {
			if (sqlite3_exec(db, sql, nullptr, nullptr, &err) != SQLITE_OK) {
				sqlite3_free(err); err = nullptr;
			}
		}

		// Backfill any hair_asm_id IN (0, 403) rows to 1974 ("NewHair15").
		//
		// Both 0 (the original v81 default) and 403 (the v82 attempt at
		// "Bald") were wrong: 0 is a non-existent asm, and 403 is in the
		// character-builder mesh category (type 596), not the in-game
		// pawn hair category (type 850) the engine looks up at spawn.
		// Asm 1974 is what SpawnBotPawn uses on every bot and is
		// confirmed to load cleanly at gameplay time. Idempotent.
		if (sqlite3_exec(db,
		        "UPDATE ga_characters SET hair_asm_id = 1974 WHERE hair_asm_id IN (0, 403);",
		        nullptr, nullptr, &err) != SQLITE_OK) {
			if (err) { sqlite3_free(err); err = nullptr; }
		}

		// Continuous-queue support: instances optionally carry the queue_id
		// they were spawned for, can point at a predecessor (DRAFTING state =
		// READY-but-waiting for predecessor's BeginEndMission), and stamp the
		// time the parent's BeginEndMission native fired so GSC_CHANGE_INSTANCE
		// can decide between routing home vs continuing to the successor.
		// All three ALTERs are idempotent — second-boot "duplicate column"
		// errors are swallowed.
		result = sqlite3_exec(db,
			"ALTER TABLE ga_instances ADD COLUMN queue_id INTEGER DEFAULT NULL;",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) sqlite3_free(err);
		result = sqlite3_exec(db,
			"ALTER TABLE ga_instances ADD COLUMN predecessor_instance_id INTEGER DEFAULT NULL;",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) sqlite3_free(err);
		result = sqlite3_exec(db,
			"ALTER TABLE ga_instances ADD COLUMN end_mission_at INTEGER DEFAULT NULL;",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) sqlite3_free(err);
		// Lookup successor-by-parent for MarkMissionEnded promotion.
		result = sqlite3_exec(db,
			"CREATE INDEX IF NOT EXISTS idx_ga_instances_predecessor "
			"ON ga_instances(predecessor_instance_id) "
			"WHERE predecessor_instance_id IS NOT NULL;",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) sqlite3_free(err);

		// ga_instance_players.profile_id — populated by InsertInstancePlayer
		// from TcpSession::selected_profile_id_. Drives DATA_SET_PROFILE_COUNTS
		// in GET_TICKET_INFO so the queue cards show class breakdowns of
		// players currently in-mission for that queue.
		result = sqlite3_exec(db,
			"ALTER TABLE ga_instance_players ADD COLUMN profile_id INTEGER DEFAULT NULL;",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) sqlite3_free(err);

		// Queue-pop controls (min_players_to_pop / max_players_per_instance /
		// pop_delay_seconds) and per-map sizing (min_players / max_players).
		// All idempotent — second-boot duplicate-column errors are swallowed.
		const char* kQueuePopAlters[] = {
			"ALTER TABLE ga_queues ADD COLUMN min_players_to_pop       INTEGER NOT NULL DEFAULT 1;",
			"ALTER TABLE ga_queues ADD COLUMN max_players_per_instance INTEGER NOT NULL DEFAULT 0;",
			"ALTER TABLE ga_queues ADD COLUMN pop_delay_seconds        REAL    NOT NULL DEFAULT 0.0;",
			"ALTER TABLE ga_map_pool_entries ADD COLUMN min_players INTEGER DEFAULT NULL;",
			"ALTER TABLE ga_map_pool_entries ADD COLUMN max_players INTEGER DEFAULT NULL;",
		};
		for (const char* sql : kQueuePopAlters) {
			if (sqlite3_exec(db, sql, nullptr, nullptr, &err) != SQLITE_OK) {
				sqlite3_free(err); err = nullptr;
			}
		}

		// Pop-delay policy (halve_on_join / fixed / reset_on_join) and
		// cap-instant-pop short-circuit. Idempotent — same duplicate-column
		// swallow pattern. instant_pop_when_full defaults to 1, which is a
		// minor behavioural diff for existing queues: any queue that hits
		// its cap during a running delay now pops immediately instead of
		// waiting out the remaining timer.
		const char* kDelayPolicyAlters[] = {
			"ALTER TABLE ga_queues ADD COLUMN pop_delay_policy      TEXT    NOT NULL DEFAULT 'halve_on_join';",
			"ALTER TABLE ga_queues ADD COLUMN instant_pop_when_full INTEGER NOT NULL DEFAULT 1;",
		};
		for (const char* sql : kDelayPolicyAlters) {
			if (sqlite3_exec(db, sql, nullptr, nullptr, &err) != SQLITE_OK) {
				sqlite3_free(err); err = nullptr;
			}
		}

		// Wire-only difficulty override. Decouples the value advertised on
		// the GET_TICKET_INFO queue card (used by the client to GROUP cards
		// by tier) from the actual difficulty_value_id used by spawn /
		// matchmaking. NULL => fall back to difficulty_value_id (existing
		// behaviour). Used so Double Agent queues can share difficulty
		// progression with SpecOps but appear under a distinct UI group.
		const char* kMarshalDifficultyAlter =
			"ALTER TABLE ga_queues ADD COLUMN marshal_difficulty_value_id INTEGER DEFAULT NULL;";
		if (sqlite3_exec(db, kMarshalDifficultyAlter, nullptr, nullptr, &err) != SQLITE_OK) {
			sqlite3_free(err); err = nullptr;
		}

		// ga_queues — data-driven queue definitions. Each row maps to a
		// MATCH_QUEUE_ID; the schema carries both matchmaking behaviour
		// (rule_class + taskforce_policy + continue_in_queue) and the
		// GET_TICKET_INFO wire fields the client uses to render the card.
		// rule_class NULL = DataDrivenMatchRule (the generic one parameterised
		// by taskforce_policy). enabled=0 hides the row from the client.
		result = sqlite3_exec(db,
			"CREATE TABLE IF NOT EXISTS ga_queues ("
			"  queue_id                INTEGER PRIMARY KEY,"
			"  name                    TEXT    NOT NULL,"
			"  rule_class              TEXT             DEFAULT NULL,"
			"  taskforce_policy        TEXT    NOT NULL DEFAULT 'pinned_1',"
			"  continue_in_queue       INTEGER NOT NULL DEFAULT 0,"
			"  enabled                 INTEGER NOT NULL DEFAULT 1,"
			"  queue_type_value_id     INTEGER NOT NULL DEFAULT 0,"
			"  status_msg_id           INTEGER NOT NULL DEFAULT 0,"
			"  name_msg_id             INTEGER NOT NULL DEFAULT 0,"
			"  desc_msg_id             INTEGER NOT NULL DEFAULT 0,"
			"  icon_id                 INTEGER NOT NULL DEFAULT 0,"
			"  max_players_per_side    INTEGER NOT NULL DEFAULT 1,"
			"  min_players_per_team    INTEGER NOT NULL DEFAULT 1,"
			"  max_players_per_team    INTEGER NOT NULL DEFAULT 1,"
			"  level_min               INTEGER NOT NULL DEFAULT 1,"
			"  level_max               INTEGER NOT NULL DEFAULT 200,"
			"  tab                     INTEGER NOT NULL DEFAULT 0,"
			"  map_x                   REAL    NOT NULL DEFAULT 0.0,"
			"  map_y                   REAL    NOT NULL DEFAULT 0.0,"
			"  map_active_flag         INTEGER NOT NULL DEFAULT 1,"
			"  map_icon_texture_res_id INTEGER NOT NULL DEFAULT 0,"
			"  video_res_id            INTEGER NOT NULL DEFAULT 0,"
			"  location_value_id       INTEGER NOT NULL DEFAULT 0,"
			"  double_agent_flag       INTEGER NOT NULL DEFAULT 0,"
			"  sys_site_id             INTEGER NOT NULL DEFAULT 0,"
			"  sort_order              INTEGER NOT NULL DEFAULT 0,"
			"  bonus_queue_flag        INTEGER NOT NULL DEFAULT 0,"
			"  difficulty_value_id     INTEGER NOT NULL DEFAULT 0,"
			"  access_flags            INTEGER NOT NULL DEFAULT 0,"
			"  active_flag             INTEGER NOT NULL DEFAULT 1,"
			"  locked_flag             INTEGER NOT NULL DEFAULT 0,"
			"  remaining_seconds       INTEGER          DEFAULT NULL,"
			"  min_players_to_pop      INTEGER NOT NULL DEFAULT 1,"
			"  max_players_per_instance INTEGER NOT NULL DEFAULT 0,"
			"  pop_delay_seconds       REAL    NOT NULL DEFAULT 0.0,"
			"  pop_delay_policy        TEXT    NOT NULL DEFAULT 'halve_on_join',"
			"  instant_pop_when_full   INTEGER NOT NULL DEFAULT 1,"
			"  marshal_difficulty_value_id INTEGER          DEFAULT NULL"
			");",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to create ga_queues table: %s\n", err);
			sqlite3_free(err);
		}

		// ga_map_pools — pool of (map, game_mode) entries shared by N queues.
		// Each ga_queues row carries a nullable map_pool_id pointing here.
		// Same shape as the old ga_queue_map_pool but keyed by pool, so
		// difficulty variants of the same content can reuse one curated list.
		result = sqlite3_exec(db,
			"CREATE TABLE IF NOT EXISTS ga_map_pools ("
			"  map_pool_id INTEGER PRIMARY KEY,"
			"  name        TEXT NOT NULL UNIQUE"
			");",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to create ga_map_pools table: %s\n", err);
			sqlite3_free(err);
		}

		// Seed pool ids 1/2/3 — anchors for the backfill UPDATE below.
		// Pool 3 ('ddr') was historically seeded only inside the gated v63
		// block in src/Database/Database.cpp, so existing DBs already past
		// v63 ended up with ga_map_pool_entries rows for map_pool_id=3 but
		// no matching ga_map_pools row. INSERT OR IGNORE adopts the orphan
		// idempotently. Map pool names stay 'specops' / 'pvp' / 'ddr' even
		// though their queues display as 'umax' / 'merc' / 'ddr' — pool
		// names are operator-facing only.
		result = sqlite3_exec(db,
			"INSERT OR IGNORE INTO ga_map_pools (map_pool_id, name) VALUES"
			" (1, 'specops'),"
			" (2, 'pvp'),"
			" (3, 'ddr');",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to seed ga_map_pools: %s\n", err);
			sqlite3_free(err);
		}

		// Add ga_queues.map_pool_id (nullable). ALTER ADD COLUMN fails once
		// the column exists — swallow that error so the migration is
		// idempotent.
		result = sqlite3_exec(db,
			"ALTER TABLE ga_queues ADD COLUMN map_pool_id INTEGER DEFAULT NULL;",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) sqlite3_free(err);

		// Adopt old auto-id DDR seed rows into canonical queue_id=3, then
		// remove only exact legacy-seed duplicates. Deliberate extra DDR
		// variants with different queue config must survive.
		const char* kDdrQueueRepair[] = {
			"UPDATE ga_queues SET queue_id = 3 "
			"WHERE queue_id = (SELECT MIN(queue_id) FROM ga_queues WHERE name = 'ddr' "
			"AND COALESCE(rule_class, '') = '' AND taskforce_policy = 'pinned_2' "
			"AND continue_in_queue = 0 AND enabled = 1 AND queue_type_value_id = 1454 "
			"AND status_msg_id = 0 AND name_msg_id = 62550 AND desc_msg_id = 64938 "
			"AND icon_id = 1714 AND max_players_per_side = 10 AND min_players_per_team = 1 "
			"AND max_players_per_team = 10 AND level_min = 5 AND level_max = 50 "
			"AND tab = 231 AND map_x = 0.0 AND map_y = 0.0 AND map_active_flag = 1 "
			"AND map_icon_texture_res_id = 5126 AND video_res_id = 0 "
			"AND location_value_id = 0 AND double_agent_flag = 1 AND sys_site_id = 0 "
			"AND sort_order = 0 AND bonus_queue_flag = 1 AND difficulty_value_id = 1471 "
			"AND access_flags = 0 AND active_flag = 1 AND locked_flag = 0 "
			"AND COALESCE(map_pool_id, 0) = 3 AND min_players_to_pop = 1 "
			"AND max_players_per_instance = 0 AND pop_delay_seconds = 0.0) "
			"AND NOT EXISTS (SELECT 1 FROM ga_queues WHERE queue_id = 3);",
			"DELETE FROM ga_queues WHERE name = 'ddr' "
			"AND COALESCE(rule_class, '') = '' AND taskforce_policy = 'pinned_2' "
			"AND continue_in_queue = 0 AND enabled = 1 AND queue_type_value_id = 1454 "
			"AND status_msg_id = 0 AND name_msg_id = 62550 AND desc_msg_id = 64938 "
			"AND icon_id = 1714 AND max_players_per_side = 10 AND min_players_per_team = 1 "
			"AND max_players_per_team = 10 AND level_min = 5 AND level_max = 50 "
			"AND tab = 231 AND map_x = 0.0 AND map_y = 0.0 AND map_active_flag = 1 "
			"AND map_icon_texture_res_id = 5126 AND video_res_id = 0 "
			"AND location_value_id = 0 AND double_agent_flag = 1 AND sys_site_id = 0 "
			"AND sort_order = 0 AND bonus_queue_flag = 1 AND difficulty_value_id = 1471 "
			"AND access_flags = 0 AND active_flag = 1 AND locked_flag = 0 "
			"AND COALESCE(map_pool_id, 0) = 3 AND min_players_to_pop = 1 "
			"AND max_players_per_instance = 0 AND pop_delay_seconds = 0.0 "
			"AND queue_id <> COALESCE((SELECT queue_id FROM ga_queues WHERE queue_id = 3 "
			"AND name = 'ddr' AND COALESCE(rule_class, '') = '' AND taskforce_policy = 'pinned_2' "
			"AND continue_in_queue = 0 AND enabled = 1 AND queue_type_value_id = 1454 "
			"AND status_msg_id = 0 AND name_msg_id = 62550 AND desc_msg_id = 64938 "
			"AND icon_id = 1714 AND max_players_per_side = 10 AND min_players_per_team = 1 "
			"AND max_players_per_team = 10 AND level_min = 5 AND level_max = 50 "
			"AND tab = 231 AND map_x = 0.0 AND map_y = 0.0 AND map_active_flag = 1 "
			"AND map_icon_texture_res_id = 5126 AND video_res_id = 0 "
			"AND location_value_id = 0 AND double_agent_flag = 1 AND sys_site_id = 0 "
			"AND sort_order = 0 AND bonus_queue_flag = 1 AND difficulty_value_id = 1471 "
			"AND access_flags = 0 AND active_flag = 1 AND locked_flag = 0 "
			"AND COALESCE(map_pool_id, 0) = 3 AND min_players_to_pop = 1 "
			"AND max_players_per_instance = 0 AND pop_delay_seconds = 0.0), "
			"(SELECT MIN(queue_id) FROM ga_queues WHERE name = 'ddr' "
			"AND COALESCE(rule_class, '') = '' AND taskforce_policy = 'pinned_2' "
			"AND continue_in_queue = 0 AND enabled = 1 AND queue_type_value_id = 1454 "
			"AND status_msg_id = 0 AND name_msg_id = 62550 AND desc_msg_id = 64938 "
			"AND icon_id = 1714 AND max_players_per_side = 10 AND min_players_per_team = 1 "
			"AND max_players_per_team = 10 AND level_min = 5 AND level_max = 50 "
			"AND tab = 231 AND map_x = 0.0 AND map_y = 0.0 AND map_active_flag = 1 "
			"AND map_icon_texture_res_id = 5126 AND video_res_id = 0 "
			"AND location_value_id = 0 AND double_agent_flag = 1 AND sys_site_id = 0 "
			"AND sort_order = 0 AND bonus_queue_flag = 1 AND difficulty_value_id = 1471 "
			"AND access_flags = 0 AND active_flag = 1 AND locked_flag = 0 "
			"AND COALESCE(map_pool_id, 0) = 3 AND min_players_to_pop = 1 "
			"AND max_players_per_instance = 0 AND pop_delay_seconds = 0.0));",
			"INSERT OR IGNORE INTO ga_queues "
			"(queue_id, name, taskforce_policy, continue_in_queue, enabled, "
			" queue_type_value_id, status_msg_id, name_msg_id, desc_msg_id, icon_id, "
			" max_players_per_side, min_players_per_team, max_players_per_team, "
			" level_min, level_max, tab, map_x, map_y, map_active_flag, "
			" map_icon_texture_res_id, video_res_id, location_value_id, "
			" double_agent_flag, sys_site_id, sort_order, bonus_queue_flag, "
			" difficulty_value_id, access_flags, active_flag, locked_flag, "
			" map_pool_id, min_players_to_pop, max_players_per_instance, pop_delay_seconds) VALUES"
			" (3, 'ddr', 'pinned_2', 0, 1,"
			"  1454, 0, 62550, 64938, 1714,"
			"  10, 1, 10, 5, 50, 231, 0.0, 0.0, 1,"
			"  5126, 0, 0, 1, 0, 0, 1,"
			"  1471, 0, 1, 0, 3, 1, 0, 0.0);",
		};
		for (const char* sql : kDdrQueueRepair) {
			result = sqlite3_exec(db, sql, nullptr, nullptr, &err);
			if (result != SQLITE_OK) {
				Logger::Log("db", "Failed DDR queue repair: %s\n", err);
				sqlite3_free(err); err = nullptr;
			}
		}

		// Remove exact duplicate DDR map-object overrides created by the same
		// retry loop. Keep one copy per value so existing data still applies.
		{
			bool map_object_config_exists = false;
			sqlite3_stmt* probe = nullptr;
			if (sqlite3_prepare_v2(db,
					"SELECT 1 FROM sqlite_master WHERE type='table' AND name='map_object_config'",
					-1, &probe, nullptr) == SQLITE_OK && probe) {
				map_object_config_exists = sqlite3_step(probe) == SQLITE_ROW;
				sqlite3_finalize(probe);
			}
			if (map_object_config_exists) {
				result = sqlite3_exec(db,
					"DELETE FROM map_object_config "
					"WHERE map_name = 'Raid_DomeCityDefense_P' "
					"AND id NOT IN ("
					"  SELECT MIN(id) FROM map_object_config "
					"  WHERE map_name = 'Raid_DomeCityDefense_P' "
					"  GROUP BY map_name, map_object_id, column_name, value, "
					"           COALESCE(variant_group, ''), COALESCE(variant_id, ''), weight"
					");",
					nullptr, nullptr, &err);
				if (result != SQLITE_OK) {
					Logger::Log("db", "Failed DDR map_object_config dedupe: %s\n", err);
					sqlite3_free(err);
				}
			}
		}

		// Seed the two known queues mirroring the pre-DB hardcoded config in
		// main.cpp. INSERT OR IGNORE preserves any operator edits across boots.
		result = sqlite3_exec(db,
			"INSERT OR IGNORE INTO ga_queues "
			"(queue_id, name, taskforce_policy, continue_in_queue, "
			" queue_type_value_id, name_msg_id, desc_msg_id, icon_id, "
			" max_players_per_side, min_players_per_team, max_players_per_team, "
			" level_min, level_max, tab, map_x, map_y, map_active_flag, "
			" map_icon_texture_res_id, location_value_id, double_agent_flag, "
			" bonus_queue_flag, difficulty_value_id, active_flag, locked_flag) VALUES"
			" (1, 'umax',     'pinned_1', 0,"
			"  0x3fd, 0xd8a9, 0xd8a8, 0x219,"
			"  10, 1, 10, 5, 200, 0x1bb, 6.0, 0.0, 1, 0x1406, 0x5c5, 1, 0, 0x5bf, 1, 0),"
			" (2, 'merc',     'balanced', 0,"
			"  0x3fe, 0xa200, 0xa1ff, 0x214,"
			"  10, 1, 3,  5, 200, 0x1,   1.0, 0.0, 1, 0x1406, 0,     1, 0, 0,     1, 0);",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to seed ga_queues: %s\n", err);
			sqlite3_free(err);
		}

		// Backfill ga_queues.map_pool_id from queue_id. The seeded queues
		// (1/2) match the seeded pools above. No-op after first boot.
		result = sqlite3_exec(db,
			"UPDATE ga_queues SET map_pool_id = queue_id WHERE map_pool_id IS NULL;",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) sqlite3_free(err);

		// Rename the seeded queues to their canonical names. Map pool names
		// stay 'specops' / 'pvp' on purpose — these are queue-display names,
		// not pool names. Gated on the old name so operator renames stick.
		result = sqlite3_exec(db,
			"UPDATE ga_queues SET name = 'umax' WHERE queue_id = 1 AND name = 'specops';",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) sqlite3_free(err);
		result = sqlite3_exec(db,
			"UPDATE ga_queues SET name = 'merc' WHERE queue_id = 2 AND name = 'pvp';",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) sqlite3_free(err);

		// Seed the medium / high / max difficulty queues. Identical to umax
		// (queue 1) except name, name_msg_id, desc_msg_id, difficulty_value_id,
		// sort_order. All share map_pool_id=1 (the specops pool) so they
		// pull from the same map list. sort_order: medium=1, high=2, max=3,
		// umax=4 (UPDATE below). queue_ids 4/5/6 — 3 is already taken by ddr.
		result = sqlite3_exec(db,
			"INSERT OR IGNORE INTO ga_queues "
			"(queue_id, name, taskforce_policy, continue_in_queue, "
			" queue_type_value_id, name_msg_id, desc_msg_id, icon_id, "
			" max_players_per_side, min_players_per_team, max_players_per_team, "
			" level_min, level_max, tab, map_x, map_y, map_active_flag, "
			" map_icon_texture_res_id, location_value_id, double_agent_flag, "
			" bonus_queue_flag, difficulty_value_id, active_flag, locked_flag, "
			" sort_order, map_pool_id) VALUES"
			" (4, 'medium', 'pinned_1', 0,"
			"  0x3fd, 27673, 41459, 0x219,"
			"  10, 1, 10, 5, 200, 0x1bb, 6.0, 0.0, 1, 0x1406, 0x5c5, 1, 0, 1029, 1, 0,"
			"  1, 1),"
			" (5, 'high',   'pinned_1', 0,"
			"  0x3fd, 27674, 55458, 0x219,"
			"  10, 1, 10, 5, 200, 0x1bb, 6.0, 0.0, 1, 0x1406, 0x5c5, 1, 0, 1030, 1, 0,"
			"  2, 1),"
			" (6, 'max',    'pinned_1', 0,"
			"  0x3fd, 34212, 55460, 0x219,"
			"  10, 1, 10, 5, 200, 0x1bb, 6.0, 0.0, 1, 0x1406, 0x5c5, 1, 0, 1259, 1, 0,"
			"  3, 1);",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to seed medium/high/max difficulty queues: %s\n", err);
			sqlite3_free(err);
		}

		// Bump umax to 4th place behind the new difficulty queues. Gated on
		// sort_order=0 so operator edits stick across boots.
		result = sqlite3_exec(db,
			"UPDATE ga_queues SET sort_order = 4 WHERE queue_id = 1 AND sort_order = 0;",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) sqlite3_free(err);

		// Seed the three Double Agent queues. Asymmetric coop with random
		// side assignment per the rule's hardcoded shape table (1v1 .. 6v4).
		// rule_class='DoubleAgent' routes through RuleFactory; the
		// taskforce_policy column is ignored by that rule. Three difficulty
		// tiers (high / max / umax) sharing all wire fields except
		// difficulty_value_id, location_value_id, name. queue_ids 8/9/10,
		// sort_orders 5/6/7 placing them after umax (queue 1, sort 4) in
		// the queue_type_value_id=1021 (SpecOps progression) group.
		// map_pool_id=1 — shares the specops pool with the
		// medium/high/max/umax difficulty queues (operator can curate a
		// DA-only pool later by editing each row's map_pool_id).
		// Location values: 1483=Sonoran Desert, 1478=Mining Province,
		// 1477=Commonwealth Prime (sourced from TcpSession's legacy
		// hardcoded ticket-info hexes at 0x5cb/0x5c6/0x5c5).
		result = sqlite3_exec(db,
			"INSERT OR IGNORE INTO ga_queues "
			"(queue_id, name, rule_class, taskforce_policy, continue_in_queue, enabled, "
			" queue_type_value_id, status_msg_id, name_msg_id, desc_msg_id, icon_id, "
			" max_players_per_side, min_players_per_team, max_players_per_team, "
			" level_min, level_max, tab, map_x, map_y, map_active_flag, "
			" map_icon_texture_res_id, video_res_id, location_value_id, double_agent_flag, "
			" sys_site_id, sort_order, bonus_queue_flag, difficulty_value_id, access_flags, "
			" active_flag, locked_flag, map_pool_id, "
			" min_players_to_pop, max_players_per_instance, pop_delay_seconds, "
			" pop_delay_policy, instant_pop_when_full, "
			" marshal_difficulty_value_id) VALUES "
			"(8, 'double_agent_high', 'DoubleAgent', 'pinned_1', 0, 1, "
			" 1021, 0, 34213, 27674, 537, "
			" 6, 1, 6, "
			" 5, 50, 443, 6.0, 0.0, 1, "
			" 5126, 0, 1483, 1, "
			" 0, 5, 0, 1030, 0, "
			" 1, 0, 1, "
			" 2, 10, 15.0, "
			" 'reset_on_join', 1, 1260), "
			"(9, 'double_agent_max', 'DoubleAgent', 'pinned_1', 0, 1, "
			" 1021, 0, 34213, 34212, 537, "
			" 6, 1, 6, "
			" 5, 50, 443, 6.0, 0.0, 1, "
			" 5126, 0, 1478, 1, "
			" 0, 6, 0, 1259, 0, "
			" 1, 0, 1, "
			" 2, 10, 15.0, "
			" 'reset_on_join', 1, 1260), "
			"(10, 'double_agent_umax', 'DoubleAgent', 'pinned_1', 0, 1, "
			" 1021, 0, 34213, 55465, 537, "
			" 6, 1, 6, "
			" 5, 50, 443, 6.0, 0.0, 1, "
			" 5126, 0, 1477, 1, "
			" 0, 7, 0, 1471, 0, "
			" 1, 0, 1, "
			" 2, 10, 15.0, "
			" 'reset_on_join', 1, 1260);",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to seed double_agent queues: %s\n", err);
			sqlite3_free(err);
		}

		// PvP queue 2 needs video_res_id=0x171a (the Mercenary preview video).
		// Defaulted to 0 above; explicit update keeps the seed line readable.
		result = sqlite3_exec(db,
			"UPDATE ga_queues SET video_res_id = 0x171a "
			"WHERE queue_id = 2 AND video_res_id = 0;",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) sqlite3_free(err);

		// ga_map_pool_entries — replaces ga_queue_map_pool. Same columns
		// minus queue_id, plus the map_pool_id key. PRIMARY KEY now
		// (map_pool_id, map_name, game_mode) so the same map can appear in
		// many pools.
		result = sqlite3_exec(db,
			"CREATE TABLE IF NOT EXISTS ga_map_pool_entries ("
			"  map_pool_id INTEGER NOT NULL REFERENCES ga_map_pools(map_pool_id) ON DELETE CASCADE,"
			"  map_name    TEXT    NOT NULL,"
			"  game_mode   TEXT    NOT NULL,"
			"  weight      INTEGER NOT NULL DEFAULT 1,"
			"  enabled     INTEGER NOT NULL DEFAULT 1,"
			"  min_players INTEGER          DEFAULT NULL,"
			"  max_players INTEGER          DEFAULT NULL,"
			"  PRIMARY KEY (map_pool_id, map_name, game_mode)"
			");",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to create ga_map_pool_entries table: %s\n", err);
			sqlite3_free(err);
		}

		// One-shot migration from the old ga_queue_map_pool. Probe
		// sqlite_master; if the legacy table still exists, copy its rows
		// over with queue_id ↦ map_pool_id (matches the backfill above
		// where queue_id == map_pool_id for the seeded queues), then drop
		// it. On subsequent boots this block is a no-op because the legacy
		// table is gone.
		{
			bool legacy_exists = false;
			sqlite3_stmt* probe = nullptr;
			if (sqlite3_prepare_v2(db,
			        "SELECT 1 FROM sqlite_master WHERE type='table' AND name='ga_queue_map_pool'",
			        -1, &probe, nullptr) == SQLITE_OK && probe) {
				if (sqlite3_step(probe) == SQLITE_ROW) legacy_exists = true;
				sqlite3_finalize(probe);
			}
			if (legacy_exists) {
				result = sqlite3_exec(db,
					"INSERT OR IGNORE INTO ga_map_pool_entries "
					"  (map_pool_id, map_name, game_mode, weight, enabled) "
					"SELECT queue_id, map_name, game_mode, weight, enabled "
					"FROM ga_queue_map_pool;",
					nullptr, nullptr, &err);
				if (result != SQLITE_OK) {
					Logger::Log("db", "Failed to copy ga_queue_map_pool -> ga_map_pool_entries: %s\n", err);
					sqlite3_free(err);
				}
				result = sqlite3_exec(db,
					"DROP TABLE ga_queue_map_pool;",
					nullptr, nullptr, &err);
				if (result != SQLITE_OK) {
					Logger::Log("db", "Failed to drop legacy ga_queue_map_pool: %s\n", err);
					sqlite3_free(err);
				} else {
					Logger::Log("db", "Migrated ga_queue_map_pool -> ga_map_pool_entries and dropped legacy table\n");
				}
			}
		}

		// Seed map pool entries — same lists as the old hardcoded blocks,
		// now keyed by map_pool_id (1=specops, 2=pvp matching the seeded
		// queues). INSERT OR IGNORE keeps operator edits intact across
		// boots and is harmless after the one-shot copy populated these
		// same rows.
		result = sqlite3_exec(db,
			"INSERT OR IGNORE INTO ga_map_pool_entries (map_pool_id, map_name, game_mode) VALUES"
			" (1, '1P_CPLab05_P', 'TgGame.TgGame_Mission'),"
			" (1, '1P_CPLab03', 'TgGame.TgGame_Mission');",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to seed ga_map_pool_entries (specops): %s\n", err);
			sqlite3_free(err);
		}
		result = sqlite3_exec(db,
			"INSERT OR IGNORE INTO ga_map_pool_entries (map_pool_id, map_name, game_mode) VALUES"
			" (2, 'Rot_Redistribution05',     'TgGame.TgGame_PointRotation'),"
			" (2, 'Rot_Redistribution04',     'TgGame.TgGame_PointRotation'),"
			" (2, 'Rot_Redistribution03',     'TgGame.TgGame_PointRotation'),"
			" (2, 'Rot_Trafalgar_P',          'TgGame.TgGame_PointRotation'),"
			" (2, 'Rot_BlackwaterLoch_P',     'TgGame.TgGame_PointRotation'),"
			" (2, 'Ticket_Silo_4v4_P',        'TgGame.TgGame_PointRotation'),"
			" (2, 'Ticket_Osprey_4v4_P',      'TgGame.TgGame_PointRotation'),"
			" (2, 'Ticket_HimLab_4v4',        'TgGame.TgGame_PointRotation'),"
			" (2, 'Push_Toxicity',            'TgGame.TgGame_Escort'),"
			" (2, 'push_Ravine_P',            'TgGame.TgGame_Escort'),"
			" (2, 'Push_Dust_P',              'TgGame.TgGame_Escort'),"
			" (2, 'Push_IceFloe3_P',          'TgGame.TgGame_Escort'),"
			" (2, 'Push_IceFloe_P',           'TgGame.TgGame_Escort'),"
			" (2, 'HEX_AVA_Push_Lab1_P',      'TgGame.TgGame_Escort'),"
			" (2, 'HEX_AVA_Push_Factory1_P',  'TgGame.TgGame_Escort'),"
			" (2, 'HEX_AVA_2pt_Theft_Lab1',   'TgGame.TgGame_Escort'),"
			" (2, 'HEX_AVA_2pt_Theft_Factory1_P', 'TgGame.TgGame_Escort'),"
			" (2, '3P_Him_Arena_P',           'TgGame.TgGame_Mission'),"
			" (2, 'Climate_Control_P',        'TgGame.TgGame_Mission'),"
			" (2, '3P_Climate_Control3_P',    'TgGame.TgGame_Mission'),"
			" (2, '3P_VolcanoAssault_P',      'TgGame.TgGame_Mission'),"
			" (2, 'Ice_GorgeA01_v2',          'TgGame.TgGame_Mission'),"
			" (2, '3P_Beachhead3_P',          'TgGame.TgGame_Mission'),"
			" (2, 'MissileComplex_4v4_P',     'TgGame.TgGame_Mission'),"
			" (2, 'Ticket_Datafarm_P',        'TgGame.TgGame_Ticket'),"
			" (2, 'Ticket_Datafarm2',         'TgGame.TgGame_Ticket'),"
			" (2, 'Ticket_Datafarm3',         'TgGame.TgGame_Ticket'),"
			" (2, 'SeaSide_Ticket_P',         'TgGame.TgGame_Ticket'),"
			" (2, 'SeaSide_Ticket2_P',        'TgGame.TgGame_Ticket'),"
			" (2, 'SeaSide_Ticket3',          'TgGame.TgGame_Ticket'),"
			" (2, 'Ticket_Volcano_P',         'TgGame.TgGame_Ticket');",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to seed ga_map_pool_entries (pvp): %s\n", err);
			sqlite3_free(err);
		}
		result = sqlite3_exec(db,
			"INSERT OR IGNORE INTO ga_map_pool_entries (map_pool_id, map_name, game_mode, weight, enabled) VALUES"
			" (3, 'Raid_DomeCityDefense_P', 'TgGame.TgGame_Defense', 1, 1);",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to seed ga_map_pool_entries (ddr): %s\n", err);
			sqlite3_free(err);
		}

		// Drop the CHECK constraint on ga_queues.taskforce_policy. The C++
		// ParseTaskforcePolicy already validates with a graceful pinned_1
		// fallback on unknown, so the DB constraint just forces a schema
		// migration for every new policy value (e.g. 'balanced_pvp' below
		// would otherwise be rejected by the original constraint that only
		// allowed pinned_1/pinned_2/balanced). Idempotent: substring-match
		// on sqlite_master.sql guards the rebuild.
		{
			bool needs_rebuild = false;
			sqlite3_stmt* st = nullptr;
			if (sqlite3_prepare_v2(db,
					"SELECT 1 FROM sqlite_master WHERE type='table' AND name='ga_queues' "
					"AND sql LIKE '%CHECK (taskforce_policy%'",
					-1, &st, nullptr) == SQLITE_OK) {
				if (sqlite3_step(st) == SQLITE_ROW) needs_rebuild = true;
				sqlite3_finalize(st);
			}
			if (needs_rebuild) {
				const char* sqls[] = {
					"BEGIN TRANSACTION",
					"CREATE TABLE ga_queues_v2 ("
					"  queue_id                INTEGER PRIMARY KEY,"
					"  name                    TEXT    NOT NULL,"
					"  rule_class              TEXT             DEFAULT NULL,"
					"  taskforce_policy        TEXT    NOT NULL DEFAULT 'pinned_1',"
					"  continue_in_queue       INTEGER NOT NULL DEFAULT 0,"
					"  enabled                 INTEGER NOT NULL DEFAULT 1,"
					"  queue_type_value_id     INTEGER NOT NULL DEFAULT 0,"
					"  status_msg_id           INTEGER NOT NULL DEFAULT 0,"
					"  name_msg_id             INTEGER NOT NULL DEFAULT 0,"
					"  desc_msg_id             INTEGER NOT NULL DEFAULT 0,"
					"  icon_id                 INTEGER NOT NULL DEFAULT 0,"
					"  max_players_per_side    INTEGER NOT NULL DEFAULT 1,"
					"  min_players_per_team    INTEGER NOT NULL DEFAULT 1,"
					"  max_players_per_team    INTEGER NOT NULL DEFAULT 1,"
					"  level_min               INTEGER NOT NULL DEFAULT 1,"
					"  level_max               INTEGER NOT NULL DEFAULT 200,"
					"  tab                     INTEGER NOT NULL DEFAULT 0,"
					"  map_x                   REAL    NOT NULL DEFAULT 0.0,"
					"  map_y                   REAL    NOT NULL DEFAULT 0.0,"
					"  map_active_flag         INTEGER NOT NULL DEFAULT 1,"
					"  map_icon_texture_res_id INTEGER NOT NULL DEFAULT 0,"
					"  video_res_id            INTEGER NOT NULL DEFAULT 0,"
					"  location_value_id       INTEGER NOT NULL DEFAULT 0,"
					"  double_agent_flag       INTEGER NOT NULL DEFAULT 0,"
					"  sys_site_id             INTEGER NOT NULL DEFAULT 0,"
					"  sort_order              INTEGER NOT NULL DEFAULT 0,"
					"  bonus_queue_flag        INTEGER NOT NULL DEFAULT 0,"
					"  difficulty_value_id     INTEGER NOT NULL DEFAULT 0,"
					"  access_flags            INTEGER NOT NULL DEFAULT 0,"
					"  active_flag             INTEGER NOT NULL DEFAULT 1,"
					"  locked_flag             INTEGER NOT NULL DEFAULT 0,"
					"  remaining_seconds       INTEGER          DEFAULT NULL,"
					"  map_pool_id             INTEGER          DEFAULT NULL,"
					"  min_players_to_pop      INTEGER NOT NULL DEFAULT 1,"
					"  max_players_per_instance INTEGER NOT NULL DEFAULT 0,"
					"  pop_delay_seconds       REAL    NOT NULL DEFAULT 0.0,"
					"  pop_delay_policy        TEXT    NOT NULL DEFAULT 'halve_on_join',"
					"  instant_pop_when_full   INTEGER NOT NULL DEFAULT 1,"
					"  marshal_difficulty_value_id INTEGER          DEFAULT NULL"
					")",
					"INSERT INTO ga_queues_v2 SELECT * FROM ga_queues",
					"DROP TABLE ga_queues",
					"ALTER TABLE ga_queues_v2 RENAME TO ga_queues",
					"COMMIT",
				};
				bool ok = true;
				for (const char* sql : sqls) {
					if (sqlite3_exec(db, sql, nullptr, nullptr, &err) != SQLITE_OK) {
						Logger::Log("db", "ga_queues CHECK-drop step failed: %s -- SQL: %s\n",
							err ? err : "?", sql);
						if (err) { sqlite3_free(err); err = nullptr; }
						sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
						ok = false;
						break;
					}
				}
				if (ok) {
					Logger::Log("db", "[Migration] ga_queues CHECK constraint on taskforce_policy dropped\n");
				}
			}
		}

		// Switch the 'merc' queue to the class-aware balanced_pvp policy.
		// Idempotent — the WHERE filter prevents a second-boot UPDATE from
		// touching the row again. Runs after the CHECK-drop above so the
		// new value is accepted.
		result = sqlite3_exec(db,
			"UPDATE ga_queues SET taskforce_policy='balanced_pvp' "
			"WHERE name='merc' AND taskforce_policy != 'balanced_pvp'",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to set merc queue to balanced_pvp: %s\n", err ? err : "?");
			if (err) sqlite3_free(err);
		} else if (sqlite3_changes(db) > 0) {
			Logger::Log("db", "[Migration] merc queue taskforce_policy -> balanced_pvp\n");
		}
	}

	{
		const char* kControlGameplaySchema =
			"CREATE TABLE IF NOT EXISTS asm_data_set_properties ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  prop_id INTEGER,"
			"  name TEXT,"
			"  prop_type_value_id INTEGER,"
			"  prop_uom_value_id INTEGER,"
			"  ui_name_msg_id INTEGER,"
			"  ui_code TEXT"
			");"
			"CREATE TABLE IF NOT EXISTS asm_data_set_blueprint_mod_effect_groups ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  blueprint_mod_id INTEGER,"
			"  effect_group_id INTEGER,"
			"  make_chance REAL"
			");"
			"CREATE TABLE IF NOT EXISTS asm_data_set_blueprint_item_mods ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  blueprint_id INTEGER,"
			"  blueprint_mod_id INTEGER,"
			"  name_msg_id INTEGER,"
			"  quality_value_id INTEGER,"
			"  icon_id INTEGER"
			");"
			"CREATE TABLE IF NOT EXISTS asm_data_set_blueprints ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  blueprint_id INTEGER,"
			"  created_item_id INTEGER,"
			"  generation_value_id INTEGER,"
			"  durability INTEGER,"
			"  quantity INTEGER,"
			"  loot_table_group_id INTEGER,"
			"  override_name_msg_id INTEGER,"
			"  destroy_on_use_flag INTEGER"
			");"
			"CREATE TABLE IF NOT EXISTS ga_players_inventory ("
			"  id                   INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  user_id              INTEGER NOT NULL REFERENCES ga_users(id),"
			"  profile_id           INTEGER NOT NULL DEFAULT 0,"
			"  device_id            INTEGER NOT NULL DEFAULT 0,"
			"  quality              INTEGER NOT NULL DEFAULT 0,"
			"  mod_effect_group_ids TEXT    NOT NULL DEFAULT '',"
			"  oc                   INTEGER NOT NULL DEFAULT 0,"
			"  allowed_slots        TEXT    NOT NULL DEFAULT '',"
			"  created_at           INTEGER NOT NULL DEFAULT (strftime('%s','now')),"
			"  item_id              INTEGER NOT NULL DEFAULT 0,"
			"  stock_n              INTEGER NOT NULL DEFAULT 0"
			");"
			"CREATE TABLE IF NOT EXISTS ga_character_devices ("
			"  id              INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  character_id    INTEGER NOT NULL REFERENCES ga_characters(id),"
			"  item_profile_id INTEGER NOT NULL,"
			"  inventory_id    INTEGER NOT NULL REFERENCES ga_players_inventory(id),"
			"  equipped_slot   INTEGER NOT NULL,"
			"  UNIQUE(character_id, item_profile_id, equipped_slot)"
			");"
			"CREATE INDEX IF NOT EXISTS idx_ga_players_inventory_user "
			"  ON ga_players_inventory(user_id, profile_id);"
			"CREATE UNIQUE INDEX IF NOT EXISTS idx_ga_players_inventory_cosmetic_uniq "
			"  ON ga_players_inventory(user_id, item_id, stock_n) WHERE item_id > 0;"
			"CREATE INDEX IF NOT EXISTS idx_ga_players_inventory_item "
			"  ON ga_players_inventory(user_id, item_id);"
			"CREATE INDEX IF NOT EXISTS idx_ga_character_devices_char "
			"  ON ga_character_devices(character_id);";
		result = sqlite3_exec(db, kControlGameplaySchema, nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to ensure control gameplay schema: %s\n", err);
			sqlite3_free(err);
			err = nullptr;
			return;
		}

		const char* kInventoryColumnAlters[] = {
			"ALTER TABLE ga_players_inventory ADD COLUMN item_id INTEGER NOT NULL DEFAULT 0;",
			"ALTER TABLE ga_players_inventory ADD COLUMN stock_n INTEGER NOT NULL DEFAULT 0;",
		};
		for (const char* sql : kInventoryColumnAlters) {
			if (sqlite3_exec(db, sql, nullptr, nullptr, &err) != SQLITE_OK) {
				sqlite3_free(err);
				err = nullptr;
			}
		}
	}

	// Floor-only version write. The game-server DLL bumps `version_info.version`
	// past 19 (currently to 24) — an unconditional UPDATE here would silently
	// downgrade the counter, causing the DLL's `version < 21` block to re-fire
	// on its next boot, which DROPs seven asm_* tables for recapture. Without
	// `AsmDataCapture::bPopulateDatabase = true` on that next launch the drops
	// run but the recapture doesn't, leaving those tables empty across every
	// subsequent boot. The WHERE clause keeps this write a one-way ratchet so
	// the DLL's higher version sticks.
	result = sqlite3_exec(db,
		"UPDATE version_info SET version = 19 WHERE version < 19",
		nullptr, nullptr, &err);
	if (result != SQLITE_OK) {
		Logger::Log("db", "Failed to update version_info: %s\n", err);
		return;
	}

	sqlite3_free(err);
	err = nullptr;
	result = sqlite3_exec(db,
		"UPDATE map_game_info "
		"SET entry_background_image_res_id = 4985, is_pvp = 1 "
		"WHERE map_game_id = 100005 "
		"   OR map_name IN ('Dome3_VR_Arena_P', 'Dome3_VR_Arena_P_reserved')",
		nullptr, nullptr, &err);
	if (result != SQLITE_OK) {
		sqlite3_free(err);
		err = nullptr;
	}

	// User-moderation tables (sessions, account bans, IP bans). UNCONDITIONAL +
	// idempotent because the version counter is shared with the DLL and has
	// already been bumped past any reasonable migration number — same
	// rationale as the ga_character_skills block above.
	{
		result = sqlite3_exec(db,
			"CREATE TABLE IF NOT EXISTS ga_user_sessions ("
			"  id          INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  user_id     INTEGER,"
			"  username    TEXT NOT NULL,"
			"  ip          TEXT NOT NULL,"
			"  login_at    INTEGER NOT NULL,"
			"  logout_at   INTEGER,"
			"  outcome     TEXT NOT NULL DEFAULT 'ok'"
			");",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to create ga_user_sessions table: %s\n", err);
			sqlite3_free(err);
			err = nullptr;
		}

		result = sqlite3_exec(db,
			"CREATE INDEX IF NOT EXISTS idx_ga_user_sessions_user "
			"ON ga_user_sessions(user_id, login_at DESC);",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to create idx_ga_user_sessions_user: %s\n", err);
			sqlite3_free(err);
			err = nullptr;
		}

		result = sqlite3_exec(db,
			"CREATE INDEX IF NOT EXISTS idx_ga_user_sessions_ip "
			"ON ga_user_sessions(ip, login_at DESC);",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to create idx_ga_user_sessions_ip: %s\n", err);
			sqlite3_free(err);
			err = nullptr;
		}

		result = sqlite3_exec(db,
			"CREATE TABLE IF NOT EXISTS ga_user_bans ("
			"  id          INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  user_id     INTEGER NOT NULL UNIQUE,"
			"  reason      TEXT NOT NULL,"
			"  banned_at   INTEGER NOT NULL,"
			"  lifted_at   INTEGER"
			");",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to create ga_user_bans table: %s\n", err);
			sqlite3_free(err);
			err = nullptr;
		}

		result = sqlite3_exec(db,
			"CREATE TABLE IF NOT EXISTS ga_ip_bans ("
			"  id          INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  ip          TEXT NOT NULL UNIQUE,"
			"  reason      TEXT NOT NULL,"
			"  banned_at   INTEGER NOT NULL,"
			"  lifted_at   INTEGER"
			");",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to create ga_ip_bans table: %s\n", err);
			sqlite3_free(err);
			err = nullptr;
		}
	}

	// NOTE: PlayerSessionStore::Init() is called separately from main.cpp -- not here.
	Logger::Log("db", "[Database::Init] Schema at version >= 19, WAL mode enabled\n");
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
