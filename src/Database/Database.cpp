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

	if (version < 16) {
		// Mega-migration: every asm_* table captured by AsmDataCapture that did not
		// previously exist. Column types match the CMarshal getter used in the
		// game's per-row loader (byte/int32/flag -> INTEGER, float -> REAL,
		// wchar_t name -> TEXT). Unnamed fields are stored as field_0xXXX — rename
		// once the symbolic constant is identified.
		const char* kNewTables =
			// helper-dispatched flat/simple
			"CREATE TABLE IF NOT EXISTS asm_data_set_projectiles ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  device_projectile_id INTEGER,"
			"  class_res_id INTEGER,"
			"  asm_id INTEGER,"
			"  explosion_fx_id INTEGER,"
			"  impact_fx_group_id INTEGER,"
			"  toss_z REAL,"
			"  acceleration_rate REAL,"
			"  draw_scale REAL,"
			"  max_nbr_of_bounces INTEGER,"
			"  spawn_item_id INTEGER,"
			"  spawn_bot_id INTEGER,"
			"  spawn_deployable_id INTEGER,"
			"  delay_track_secs REAL,"
			"  rotation_follows_velocity_flag INTEGER,"
			"  stick_to_wall_flag INTEGER,"
			"  track_target_flag INTEGER,"
			"  track_to_world_pos_flag INTEGER,"
			"  aim_from_socket_flag INTEGER"
			");"

			"CREATE TABLE IF NOT EXISTS asm_data_set_deployables ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  deployable_id INTEGER,"
			"  class_res_id INTEGER,"
			"  name_msg_id INTEGER, name_msg_translated TEXT,"
			"  desc_msg_id INTEGER, desc_msg_translated TEXT,"
			"  deployable_type_value_id INTEGER,"
			"  asm_id INTEGER,"
			"  health INTEGER,"
			"  device_id INTEGER,"
			"  death_fx_id INTEGER,"
			"  pickup_device_id INTEGER,"
			"  use_device_chance REAL,"
			"  vulnerable_to_attack_flags INTEGER,"
			"  loot_table_id INTEGER,"
			"  physical_type_value_id INTEGER,"
			"  artillery_target_type_value_id INTEGER,"
			"  destroy_on_owner_death_flag INTEGER,"
			"  delay_deploy_flag INTEGER,"
			"  require_ammo_to_live_flag INTEGER,"
			"  show_countdown_timer_flag INTEGER"
			");"

			"CREATE TABLE IF NOT EXISTS asm_data_set_deployables_visibility_configs ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  deployable_id INTEGER,"
			"  visibility_config_id INTEGER,"
			"  child_deployable_id INTEGER,"
			"  proximity_distance REAL"
			");"

			"CREATE TABLE IF NOT EXISTS asm_data_set_destructibles ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  destructible_object_id INTEGER,"
			"  name_msg_id INTEGER, name_msg_translated TEXT,"
			"  desc_msg_id INTEGER, desc_msg_translated TEXT,"
			"  asm_id INTEGER,"
			"  health INTEGER"
			");"

			"CREATE TABLE IF NOT EXISTS asm_data_set_assembly_meshes ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  asm_id INTEGER,"
			"  name TEXT,"
			"  mesh_res_id INTEGER,"
			"  anim_tree_res_id INTEGER,"
			"  physics_asset_res_id INTEGER,"
			"  socket_res_id INTEGER,"
			"  aim_offset_profile_res_id INTEGER,"
			"  socket_offset_info_res_id INTEGER,"
			"  asm_mesh_type_value_id INTEGER,"
			"  scale REAL, cull_distance REAL,"
			"  scale_3d_x REAL, scale_3d_y REAL, scale_3d_z REAL,"
			"  translation_x REAL, translation_y REAL, translation_z REAL,"
			"  rotator_pitch INTEGER, rotator_yaw INTEGER, rotator_roll INTEGER,"
			"  collision_height REAL, collision_radius REAL, collision_depth REAL,"
			"  crouch_height REAL,"
			"  hit_collision_height REAL, hit_collision_radius REAL,"
			"  physics_weight REAL, life_after_death_secs REAL,"
			"  destroyed_asm_id INTEGER,"
			"  material_res_group_id INTEGER,"
			"  race_material_parameter_id INTEGER"
			");"

			"CREATE TABLE IF NOT EXISTS asm_data_set_fx ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  fx_id INTEGER,"
			"  name TEXT,"
			"  priority_value_id INTEGER,"
			"  mic_res_id INTEGER,"
			"  transition_sec REAL"
			");"

			"CREATE TABLE IF NOT EXISTS asm_data_set_material_res_groups ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  material_res_group_id INTEGER,"
			"  name TEXT"
			");"

			"CREATE TABLE IF NOT EXISTS asm_data_set_material_parameters ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  material_parameter_id INTEGER,"
			"  param_type_value_id INTEGER,"
			"  linear_color_r REAL,"
			"  linear_color_g REAL,"
			"  linear_color_b REAL,"
			"  linear_color_a REAL"
			");"

			"CREATE TABLE IF NOT EXISTS asm_data_set_visibility_configs ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  visibility_config_id INTEGER,"
			"  display_flags INTEGER,"
			"  see_flags INTEGER"
			");"

			"CREATE TABLE IF NOT EXISTS asm_data_set_icons ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  icon_id INTEGER,"
			"  texture_res_id INTEGER,"
			"  icon_index INTEGER"
			");"

			"CREATE TABLE IF NOT EXISTS asm_data_set_properties ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  prop_id INTEGER,"
			"  name TEXT,"
			"  prop_type_value_id INTEGER,"
			"  prop_uom_value_id INTEGER,"
			"  ui_name_msg_id INTEGER,"
			"  ui_code TEXT"
			");"

			"CREATE TABLE IF NOT EXISTS asm_data_set_hex_buildings ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  hex_building_id INTEGER,"
			"  name_msg_id INTEGER,"
			"  desc_msg_id INTEGER,"
			"  building_type_value_id INTEGER,"
			"  building_subtype_value_id INTEGER,"
			"  map_game_id INTEGER,"
			"  mesh_res_id INTEGER,"
			"  bonus_value_id INTEGER,"
			"  price INTEGER,"
			"  output_value INTEGER,"
			"  defensive_cargo_space INTEGER,"
			"  defensive_player_capacity INTEGER,"
			"  icon_id INTEGER,"
			"  tech_level_value_id INTEGER"
			");"

			"CREATE TABLE IF NOT EXISTS asm_data_set_objectives ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  objective_id INTEGER,"
			"  class_res_id INTEGER,"
			"  name_msg_id INTEGER, name_msg_translated TEXT,"
			"  asm_id INTEGER,"
			"  proximity_distance REAL,"
			"  proximity_height REAL,"
			"  text_msg_id INTEGER,"
			"  bot_id INTEGER,"
			"  objective_type_value_id INTEGER,"
			"  icon_id INTEGER,"
			"  desc_msg_id INTEGER"
			");"

			"CREATE TABLE IF NOT EXISTS asm_data_set_objectives_props ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  objective_id INTEGER,"
			"  prop_id INTEGER,"
			"  value REAL"
			");"

			"CREATE TABLE IF NOT EXISTS asm_data_set_xp_levels ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  xp_level_id INTEGER,"
			"  level INTEGER,"
			"  xp_value INTEGER,"
			"  skill_points INTEGER,"
			"  device_points INTEGER,"
			"  protection_points INTEGER"
			");"

			"CREATE TABLE IF NOT EXISTS asm_data_set_xp_caps ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  xp_level_id INTEGER,"
			"  raw_data BLOB"
			");"

			"CREATE TABLE IF NOT EXISTS asm_data_set_device_slot_unlock_groups ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  device_slot_unlock_group_id INTEGER,"
			"  name TEXT"
			");"

			"CREATE TABLE IF NOT EXISTS asm_data_set_device_slot_unlock_list ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  device_slot_unlock_group_id INTEGER,"
			"  slot_value_id INTEGER,"
			"  required_xp_level_id INTEGER,"
			"  unlock_type_value_id INTEGER"
			");"

			"CREATE TABLE IF NOT EXISTS asm_data_set_ui_volumes ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  ui_volume_id INTEGER,"
			"  volume_type_value_id INTEGER,"
			"  name_msg_id INTEGER, name_msg_translated TEXT,"
			"  summary_msg_id INTEGER, summary_msg_translated TEXT,"
			"  desc_msg_id INTEGER, desc_msg_translated TEXT,"
			"  parent_help_ui_volume_id INTEGER,"
			"  use_msg_id INTEGER,"
			"  ui_scene_res_id INTEGER,"
			"  map_game_id INTEGER,"
			"  loot_table_id INTEGER,"
			"  queue_selection_list_id INTEGER,"
			"  quest_group_id INTEGER"
			");"

			"CREATE TABLE IF NOT EXISTS asm_data_set_quests ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  quest_id INTEGER,"
			"  quest_type_value_id INTEGER,"
			"  level_min INTEGER,"
			"  repeatable_flag INTEGER,"
			"  repeatable_cooldown_type_value_id INTEGER,"
			"  reward_is_pooled_flag INTEGER,"
			"  name_msg_id INTEGER, name_msg_translated TEXT,"
			"  desc_msg_id INTEGER, desc_msg_translated TEXT,"
			"  short_desc_msg_id INTEGER,"
			"  turnin_ui_volume_id INTEGER,"
			"  turnin_msg_id INTEGER,"
			"  completed_msg_id INTEGER,"
			"  xp_reward_value INTEGER,"
			"  currency_reward_value INTEGER,"
			"  loot_table_id INTEGER,"
			"  field_0x613 INTEGER"
			");"

			"CREATE TABLE IF NOT EXISTS asm_data_set_quest_groups ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  quest_group_id INTEGER,"
			"  name_msg_id INTEGER"
			");"

			"CREATE TABLE IF NOT EXISTS asm_data_set_quest_group_lists ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  quest_group_id INTEGER,"
			"  quest_id INTEGER,"
			"  disregard_quest_flag INTEGER,"
			"  quest_status_type_value_id INTEGER"
			");"

			"CREATE TABLE IF NOT EXISTS asm_data_set_achievements ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  achievement_id INTEGER,"
			"  achievement_group_id INTEGER,"
			"  name_msg_id INTEGER, name_msg_translated TEXT,"
			"  desc_msg_id INTEGER, desc_msg_translated TEXT,"
			"  icon_id INTEGER,"
			"  loot_table_id INTEGER,"
			"  title_msg_id INTEGER,"
			"  points_earned INTEGER"
			");"

			"CREATE TABLE IF NOT EXISTS asm_data_set_achievement_groups ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  achievement_group_id INTEGER,"
			"  group_type_value_id INTEGER,"
			"  name_msg_id INTEGER, name_msg_translated TEXT,"
			"  parent_achievement_group_id INTEGER"
			");"

			"CREATE TABLE IF NOT EXISTS asm_data_set_event_rewards ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  event_reward_id INTEGER,"
			"  reward_event_value_id INTEGER,"
			"  xp_reward_value REAL,"
			"  token_reward_value REAL,"
			"  currency_reward_value REAL,"
			"  difficulty_value_id INTEGER,"
			"  process_order INTEGER"
			");"

			"CREATE TABLE IF NOT EXISTS asm_data_set_impact_fx ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  impact_fx_group_id INTEGER,"
			"  material_type_res_id INTEGER,"
			"  decal_material_res_id INTEGER,"
			"  decal_height REAL,"
			"  decal_width REAL,"
			"  sound_res_id INTEGER,"
			"  particle_res_id INTEGER,"
			"  fx_id INTEGER,"
			"  decal_random_rotation_flag INTEGER"
			");"

			"CREATE TABLE IF NOT EXISTS asm_data_set_sound_cues ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  audio_group_id INTEGER,"
			"  sound_cue_id INTEGER,"
			"  sound_cue_res_id INTEGER,"
			"  sound_cue_name TEXT"
			");"

			"CREATE TABLE IF NOT EXISTS asm_data_set_tips ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  game_tip_id INTEGER,"
			"  tip_msg_id INTEGER, tip_msg_translated TEXT"
			");"

			"CREATE TABLE IF NOT EXISTS asm_data_set_resources ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  res_id INTEGER,"
			"  name TEXT,"
			"  res_type_value_id INTEGER"
			");"

			"CREATE TABLE IF NOT EXISTS asm_data_set_valid_values ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  valid_value_group_id INTEGER,"
			"  value_id INTEGER,"
			"  text_msg_id INTEGER, text_msg_translated TEXT,"
			"  sort_order INTEGER"
			");"

			"CREATE TABLE IF NOT EXISTS asm_data_set_production_devices ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  device_id INTEGER"
			");"

			"CREATE TABLE IF NOT EXISTS asm_data_set_production_devices_used_resources ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  device_id INTEGER,"
			"  res_id INTEGER,"
			"  always_load_on_server_flag INTEGER"
			");"

			"CREATE TABLE IF NOT EXISTS asm_data_set_production_flairs ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  item_id INTEGER"
			");"

			"CREATE TABLE IF NOT EXISTS asm_data_set_production_flairs_used_resources ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  item_id INTEGER,"
			"  res_id INTEGER,"
			"  always_load_on_server_flag INTEGER"
			");"

			"CREATE TABLE IF NOT EXISTS asm_data_set_pet_used_resources ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  item_id INTEGER,"
			"  res_id INTEGER"
			");"

			"CREATE TABLE IF NOT EXISTS asm_data_set_skill_group_ranks ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  skill_group_id INTEGER,"
			"  skill_id INTEGER"
			");"

			"CREATE TABLE IF NOT EXISTS asm_data_set_player_used_resources ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  res_id INTEGER,"
			"  always_load_on_server_flag INTEGER"
			");"

			"CREATE TABLE IF NOT EXISTS asm_data_set_production_map_game_list ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  map_game_id INTEGER"
			");"

			"CREATE TABLE IF NOT EXISTS asm_data_set_game_used_resources ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  map_game_id INTEGER,"
			"  res_id INTEGER,"
			"  always_load_on_server_flag INTEGER"
			");"

			"CREATE TABLE IF NOT EXISTS asm_data_set_device_used_animsets ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  res_id INTEGER,"
			"  always_load_on_server_flag INTEGER"
			");"

			"CREATE TABLE IF NOT EXISTS asm_data_set_hardcoded_resources ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  res_id INTEGER"
			");"

			"CREATE TABLE IF NOT EXISTS asm_data_set_world_objects ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  asm_id INTEGER"
			");"

			"CREATE TABLE IF NOT EXISTS asm_data_set_world_object_resources ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  asm_id INTEGER,"
			"  res_id INTEGER"
			");"

			"CREATE TABLE IF NOT EXISTS asm_data_set_objective_bots ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  objective_id INTEGER"
			");"

			"CREATE TABLE IF NOT EXISTS asm_data_set_objective_bot_resources ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  objective_id INTEGER,"
			"  res_id INTEGER"
			");"

			"CREATE TABLE IF NOT EXISTS asm_data_set_sub_skills ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  sub_skill_id INTEGER,"
			"  skill_id INTEGER,"
			"  name_msg_translated TEXT"
			");"

			"CREATE TABLE IF NOT EXISTS asm_data_set_skill_group_skills ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  skill_group_id INTEGER,"
			"  skill_id INTEGER,"
			"  name_msg_translated TEXT,"
			"  desc_msg_translated TEXT,"
			"  prereq_group_points INTEGER,"
			"  prereq_skill_id INTEGER,"
			"  prereq_skill_points INTEGER,"
			"  sort_order INTEGER,"
			"  type_value_id INTEGER,"
			"  group_loc_x INTEGER,"
			"  group_loc_y INTEGER,"
			"  icon_id INTEGER"
			");"

			"CREATE TABLE IF NOT EXISTS asm_data_set_blueprint_items ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  blueprint_id INTEGER,"
			"  item_id INTEGER,"
			"  quantity INTEGER"
			");"

			"CREATE TABLE IF NOT EXISTS asm_data_set_blueprint_item_mods ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  blueprint_id INTEGER,"
			"  blueprint_mod_id INTEGER,"
			"  name_msg_id INTEGER,"
			"  quality_value_id INTEGER,"
			"  icon_id INTEGER"
			");"

			"CREATE TABLE IF NOT EXISTS asm_data_set_loot_table_items ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  loot_table_id INTEGER,"
			"  loot_table_item_id INTEGER,"
			"  item_id INTEGER,"
			"  sub_loot_table_id INTEGER,"
			"  quantity INTEGER,"
			"  drop_chance REAL,"
			"  sort_order INTEGER,"
			"  quality_value_id INTEGER,"
			"  quality_ranking INTEGER,"
			"  max_quality_value_id INTEGER,"
			"  auto_create_flag INTEGER,"
			"  use_player_components_flag INTEGER"
			");"

			"CREATE TABLE IF NOT EXISTS asm_data_set_loot_table_item_prices ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  loot_table_item_id INTEGER,"
			"  price INTEGER,"
			"  currency INTEGER"
			");"

			"CREATE TABLE IF NOT EXISTS asm_data_set_loot_table_group_items ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  loot_table_group_id INTEGER,"
			"  sort_order INTEGER,"
			"  loot_table_id INTEGER"
			");"

			"CREATE TABLE IF NOT EXISTS asm_data_set_blueprint_mod_effect_groups ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  blueprint_mod_id INTEGER,"
			"  effect_group_id INTEGER,"
			"  make_chance REAL"
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

			"CREATE TABLE IF NOT EXISTS asm_data_set_loot_tables ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  loot_table_id INTEGER,"
			"  name_msg_translated TEXT,"
			"  desc_msg_translated TEXT,"
			"  loot_table_type_value_id INTEGER,"
			"  system_craft_skill_level INTEGER"
			");"

			"CREATE TABLE IF NOT EXISTS asm_data_set_loot_table_groups ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  loot_table_group_id INTEGER,"
			"  name_msg_translated TEXT,"
			"  desc_msg_translated TEXT"
			");"

			"CREATE TABLE IF NOT EXISTS asm_data_set_skill_group_sets ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  skill_group_set_id INTEGER,"
			"  name TEXT,"
			"  total_points_available INTEGER,"
			"  bot_id INTEGER"
			");"

			"CREATE TABLE IF NOT EXISTS asm_data_set_skill_group_set_groups ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  skill_group_set_id INTEGER,"
			"  skill_group_id INTEGER,"
			"  primary_flag INTEGER"
			");"

			"CREATE TABLE IF NOT EXISTS asm_data_set_skill_groups ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  skill_group_id INTEGER,"
			"  name TEXT"
			");"
			;

		result = sqlite3_exec(db, kNewTables, nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed v16 mega-migration: %s\n", err);
			return;
		}
	}

	if (version < 17) {
		// v17: expand the device-mode capture and add every nested data set that the
		// game's asm.dat parser walks but we never bound to a table. Existing asm_*
		// tables that were populated under v16 are left alone — AsmDataCapture's
		// per-table guards skip re-insertion on any table that already has rows.
		//
		// Exception: asm_data_set_devices_data_set_device_modes is dropped and
		// recreated with the full ~30-column schema; the prior captures stored only
		// (device_id, device_mode_id, name_msg_id, name_msg_translated) which is a
		// strict subset of the new schema. Next game run repopulates it in full.
		const char* kV17 =
			"DROP TABLE IF EXISTS asm_data_set_devices_data_set_device_modes;"

			"CREATE TABLE asm_data_set_devices_data_set_device_modes ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  device_id INTEGER,"
			"  device_mode_id INTEGER,"
			"  name_msg_id INTEGER,"
			"  name_msg_translated TEXT,"
			"  class_res_id INTEGER,"
			"  damage_class_res_id INTEGER,"
			"  device_projectile_id INTEGER,"
			"  deployable_id INTEGER,"
			"  bot_id INTEGER,"
			"  impact_fx_group_id INTEGER,"
			"  damage_type_value_id INTEGER,"
			"  offhand_anim_res_id INTEGER,"
			"  target_type_value_id INTEGER,"
			"  target_type_affect_value_id INTEGER,"
			"  attack_type_value_id INTEGER,"
			"  remote_type_value_id INTEGER,"
			"  camera_type_value_id INTEGER,"
			"  reticule_res_id INTEGER,"
			"  scope_material_res_id INTEGER,"
			"  scale_fire_anim_flag INTEGER,"
			"  interrupt_fire_anim_on_refire_flag INTEGER,"
			"  require_los_flag INTEGER,"
			"  continuous_fire_flag INTEGER,"
			"  restrict_in_combat_flag INTEGER,"
			"  require_aimmode_flag INTEGER,"
			"  do_not_pause_ai_flag INTEGER,"
			"  restrict_firing_flags INTEGER,"
			"  restrict_physics_firing_flags INTEGER,"
			"  return_to_idle_anim_secs REAL,"
			"  icon_id INTEGER,"
			"  attack_rating INTEGER,"
			"  fire_mode_sequence INTEGER"
			");"

			// device → effect group (equip effect, first row wins in binary logic)
			"CREATE TABLE IF NOT EXISTS asm_data_set_device_effect_groups ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  device_id INTEGER,"
			"  effect_group_id INTEGER"
			");"

			// per-mode effect groups (what TgDeviceFire::GetEffectGroup walks at runtime)
			"CREATE TABLE IF NOT EXISTS asm_data_set_device_mode_effect_groups ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  device_id INTEGER,"
			"  device_mode_id INTEGER,"
			"  effect_group_id INTEGER,"
			"  effect_group_type_value_id INTEGER"
			");"

			// per-mode property overrides
			"CREATE TABLE IF NOT EXISTS asm_data_set_device_mode_properties ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  device_id INTEGER,"
			"  device_mode_id INTEGER,"
			"  prop_id INTEGER,"
			"  value REAL"
			");"

			// device animation set groups (gender × dest_type → anim_set_res_id)
			"CREATE TABLE IF NOT EXISTS asm_data_set_device_anim_set_groups ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  device_id INTEGER,"
			"  gender_type_value_id INTEGER,"
			"  anim_set_dest_type_value_id INTEGER,"
			"  anim_set_res_id INTEGER"
			");"

			// item → passive effect groups
			"CREATE TABLE IF NOT EXISTS asm_data_set_item_effect_groups ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  item_id INTEGER,"
			"  effect_group_id INTEGER,"
			"  effect_group_type_value_id INTEGER"
			");"

			// item → property overrides
			"CREATE TABLE IF NOT EXISTS asm_data_set_item_props ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  item_id INTEGER,"
			"  prop_id INTEGER,"
			"  value REAL"
			");"

			// item → mesh assembly groups (per gender/quality/mesh type)
			"CREATE TABLE IF NOT EXISTS asm_data_set_item_mesh_asm_groups ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  item_id INTEGER,"
			"  gender_type_value_id INTEGER,"
			"  item_mesh_asm_type_value_id INTEGER,"
			"  quality_value_id INTEGER,"
			"  asm_id INTEGER"
			");"

			// store prices (per-item price rows keyed by currency)
			"CREATE TABLE IF NOT EXISTS asm_data_set_prices ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  item_id INTEGER,"
			"  price INTEGER,"
			"  currency_type_value_id INTEGER"
			");"

			// skill → effect groups (skill-tree nodes that grant passive effects)
			"CREATE TABLE IF NOT EXISTS asm_data_set_skill_effect_groups ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  skill_group_id INTEGER,"
			"  skill_id INTEGER,"
			"  sub_skill_id INTEGER,"
			"  effect_group_id INTEGER,"
			"  effect_group_type_value_id INTEGER"
			");"
			;
		result = sqlite3_exec(db, kV17, nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed v17 migration: %s\n", err);
			return;
		}
	}

	if (version < 18) {
		// v18: the complete asm.dat audit (.planning/asm-dat-audit/) found 17
		// datasets reachable from LoadAssemblyDatFile that were never captured.
		// All are nested under parents whose scalar data we already store.
		// Schemas below match field sets observed in the per-row loader functions
		// (FUN_* addresses cited in .planning/asm-dat-audit/LOADER_FINDINGS.md).
		//
		// Also rebuilds asm_data_set_xp_caps — the v16 placeholder had
		// {id, xp_level_id, raw_data BLOB} and no walker; the real schema is
		// below. Table was empty, so drop+recreate is safe.
		const char* kV18 =
			"DROP TABLE IF EXISTS asm_data_set_xp_caps;"

			// ---------- Assembly-mesh sub-tables ----------
			"CREATE TABLE asm_data_set_asm_mesh_anim_sets ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  asm_id INTEGER,"
			"  anim_set_res_id INTEGER,"
			"  sort_order INTEGER"
			");"

			"CREATE TABLE asm_data_set_asm_mesh_audio_groups ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  asm_id INTEGER,"
			"  audio_group_id INTEGER"
			");"

			"CREATE TABLE asm_data_set_asm_mesh_fxs ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  asm_id INTEGER,"
			"  socket_res_id INTEGER,"
			"  display_group_res_id INTEGER,"
			"  display_mode INTEGER,"
			"  fx_id INTEGER,"
			"  display_order INTEGER,"
			"  slot_value_id INTEGER"
			");"

			"CREATE TABLE asm_data_set_asm_mesh_impact_grps ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  asm_id INTEGER,"
			"  impact_fx_group_id INTEGER,"
			"  impact_fx_group_type_value_id INTEGER"
			");"

			"CREATE TABLE asm_data_set_asm_mesh_morph_sets ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  asm_id INTEGER,"
			"  morph_set_res_id INTEGER,"
			"  sort_order INTEGER"
			");"

			// ---------- Special-FX sub-tables ----------
			"CREATE TABLE asm_data_set_special_fx_materials ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  fx_id INTEGER,"
			"  fx_material_id INTEGER,"
			"  fx_material_type_value_id INTEGER,"
			"  material_res_id INTEGER,"
			"  same_team_flag INTEGER"
			");"

			"CREATE TABLE asm_data_set_special_fx_psc ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  fx_id INTEGER,"
			"  fx_psc_id INTEGER,"
			"  psc_template_res_id INTEGER,"
			"  psc_loops_flag INTEGER,"
			"  scale REAL,"
			"  scaling_type_value_id INTEGER,"
			"  fx_display_group_res_id INTEGER"
			");"

			"CREATE TABLE asm_data_set_special_fx_sounds ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  fx_id INTEGER,"
			"  fx_sound_id INTEGER,"
			"  sound_cue_res_id INTEGER,"
			"  allow_sound_to_finish_flag INTEGER"
			");"

			// ---------- Material resources hierarchy ----------
			"CREATE TABLE asm_data_set_material_resources ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  material_res_group_id INTEGER,"
			"  material_res_id INTEGER,"
			"  material_res_sub_type_value_id INTEGER,"
			"  material_res_type_value_id INTEGER,"
			"  res_id INTEGER,"
			"  sort_order INTEGER"
			");"

			"CREATE TABLE asm_data_set_mat_res_params ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  material_res_id INTEGER,"
			"  mat_res_param_id INTEGER,"
			"  material_parameter_id INTEGER,"
			"  parameter_res_id INTEGER"
			");"

			// ---------- XP caps (rebuild of v16 placeholder) ----------
			"CREATE TABLE asm_data_set_xp_caps ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  xp_level_id INTEGER,"
			"  xp_mission_cap_id INTEGER,"
			"  event_reward_id INTEGER,"
			"  xp_value INTEGER,"
			"  reward_event_value_id INTEGER"
			");"

			// ---------- Quest requirement chain ----------
			"CREATE TABLE asm_data_set_quest_requirements ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  quest_id INTEGER,"
			"  quest_requirement_id INTEGER,"
			"  count INTEGER,"
			"  map_game_id INTEGER,"
			"  requirement_type_value_id INTEGER,"
			"  gameplay_type_value_id INTEGER,"
			"  difficulty_value_id INTEGER,"
			"  name_msg_id INTEGER,"
			"  target_bot_id INTEGER,"
			"  target_item_id INTEGER,"
			"  target_ui_volume_id INTEGER"
			");"

			"CREATE TABLE asm_data_set_quest_prerequisites ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  quest_id INTEGER,"
			"  prereq_quest_id INTEGER,"
			"  quest_status_type_value_id INTEGER"
			");"

			"CREATE TABLE asm_data_set_quest_req_objectives ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  quest_requirement_id INTEGER,"
			"  objective_id INTEGER"
			");"

			// ---------- Bot AI hierarchy (behaviors → actions → tests) ----------
			"CREATE TABLE asm_data_set_bot_behaviors ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  behavior_id INTEGER,"
			"  name TEXT"
			");"

			"CREATE TABLE asm_data_set_bot_actions ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  behavior_id INTEGER,"
			"  action_id INTEGER,"
			"  action_type_value_id INTEGER,"
			"  description TEXT,"
			"  movement_type_value_id INTEGER,"
			"  destination_type_value_id INTEGER,"
			"  pace_type_value_id INTEGER,"
			"  target_type_value_id INTEGER,"
			"  look_at_type_value_id INTEGER,"
			"  sound_cue_id INTEGER,"
			"  audio_chance REAL,"
			"  animation_res_id INTEGER,"
			"  animation_seconds REAL,"
			"  alarm_bot_spawn_table_id INTEGER,"
			"  generic_event_number INTEGER,"
			"  sub_behavior_id INTEGER,"
			"  custom_param_string TEXT,"
			"  emote_interruptible_flag INTEGER,"
			"  reevaluate_tests_flag INTEGER,"
			"  fire_mode_sequence INTEGER,"
			"  slot_used_value_id INTEGER,"
			"  secondary_fire_mode_sequence INTEGER,"
			"  secondary_slot_used_value_id INTEGER,"
			"  posture_type_value_id INTEGER"
			");"

			"CREATE TABLE asm_data_set_bot_tests ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  action_id INTEGER,"
			"  test_type_value_id INTEGER,"
			"  comparison_type_value_id INTEGER,"
			"  comparison_value REAL"
			");"
			;
		result = sqlite3_exec(db, kV18, nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed v18 migration: %s\n", err);
			return;
		}
	}

	result = sqlite3_exec(db, "UPDATE version_info SET version = 18", nullptr, nullptr, &err);
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

