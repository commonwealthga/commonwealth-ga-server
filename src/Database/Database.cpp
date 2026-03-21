#include "src/Database/Database.hpp"
#include "src/GameServer/Storage/PlayerRegistry/PlayerRegistry.hpp"
#include "src/Utils/Logger/Logger.hpp"

sqlite3* Database::connection = nullptr;

sqlite3* Database::GetConnection() {
	if (Database::connection == nullptr) {
		int result = sqlite3_open("server.db", &connection);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to open database: %s", sqlite3_errmsg(connection));
			return nullptr;
		}
	}
	
	return Database::connection;
}

void Database::CloseConnection() {
	sqlite3_close(Database::connection);
	Database::connection = nullptr;
}

int Database::Callback(void* data, int argc, char** argv, char** azColName) {

	std::vector<std::map<std::string, std::string>>* dataMap = static_cast<std::vector<std::map<std::string, std::string>>*>(data);

	std::map<std::string, std::string> row;

	for (int i = 0; i < argc; i++) {
		row[azColName[i]] = argv[i];
	}

	dataMap->push_back(row);

	return 0;
}

void Database::Init() {
	sqlite3* db = GetConnection();

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
				(12709, 28, 2, 0); \
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


	result = sqlite3_exec(db, "UPDATE version_info SET version = 14", nullptr, nullptr, &err);
	if (result != SQLITE_OK) {
		Logger::Log("db", "Failed to update version_info: %s\n", err);
		return;
	}

	PlayerRegistry::Init();
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

