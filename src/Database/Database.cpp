#include "src/Database/Database.hpp"
#include "src/Config/Config.hpp"
#include "src/GameServer/Storage/PlayerRegistry/PlayerRegistry.hpp"
#include "src/Utils/Logger/Logger.hpp"

sqlite3* Database::connection = nullptr;

sqlite3* Database::GetConnection() {
	if (Database::connection == nullptr) {
		const std::string db_path = Config::GetDbPath();
		int result = sqlite3_open(db_path.c_str(), &connection);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to open database '%s': %s", db_path.c_str(), sqlite3_errmsg(connection));
			return nullptr;
		}
		Logger::Log("db", "Opened database '%s'\n", db_path.c_str());
		// journal_mode is persistent in the DB header (set by the control-server
		// open) — re-asserting it is cheap and makes the intent explicit. The
		// other three pragmas are per-connection: every game-DLL connection has
		// to set them itself. journal_size_limit is the one that actually shrinks
		// the WAL after each autocheckpoint — without it the file grows to its
		// historical high-water mark and never comes back down.
		sqlite3_exec(connection, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
		sqlite3_exec(connection, "PRAGMA busy_timeout=5000;", nullptr, nullptr, nullptr);
		sqlite3_exec(connection, "PRAGMA synchronous=NORMAL;", nullptr, nullptr, nullptr);
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
		row[azColName[i]] = argv[i];
	}

	dataMap->push_back(row);

	return 0;
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

void Database::Init() {
	sqlite3* db = GetConnection();
	if (!db) {
		Logger::Log("db", "Database::Init skipped: no DB connection\n");
		PlayerRegistry::Init();
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

	auto exec_ignore_error = [&](const char* sql) {
		char* local_err = nullptr;
		if (sqlite3_exec(db, sql, nullptr, nullptr, &local_err) != SQLITE_OK) {
			sqlite3_free(local_err);
		}
	};

	auto exec_required = [&](const char* label, const char* sql) -> bool {
		char* local_err = nullptr;
		if (sqlite3_exec(db, sql, nullptr, nullptr, &local_err) != SQLITE_OK) {
			Logger::Log("db", "Failed schema floor (%s): %s\n", label, local_err ? local_err : "");
			sqlite3_free(local_err);
			return false;
		}
		return true;
	};

	const char* kSharedGameplaySchemaFloor =
		"CREATE TABLE IF NOT EXISTS asm_data_set_properties ("
		"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
		"  prop_id INTEGER,"
		"  name TEXT,"
		"  prop_type_value_id INTEGER,"
		"  prop_uom_value_id INTEGER,"
		"  ui_name_msg_id INTEGER,"
		"  ui_code TEXT"
		");"
		"CREATE TABLE IF NOT EXISTS asm_data_set_device_effect_groups ("
		"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
		"  device_id INTEGER,"
		"  effect_group_id INTEGER"
		");"
		"CREATE TABLE IF NOT EXISTS asm_data_set_device_mode_effect_groups ("
		"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
		"  device_id INTEGER,"
		"  device_mode_id INTEGER,"
		"  effect_group_id INTEGER,"
		"  effect_group_type_value_id INTEGER"
		");"
		"CREATE TABLE IF NOT EXISTS asm_data_set_device_mode_properties ("
		"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
		"  device_id INTEGER,"
		"  device_mode_id INTEGER,"
		"  prop_id INTEGER,"
		"  base_value REAL,"
		"  min_value REAL,"
		"  max_value REAL"
		");"
		"CREATE TABLE IF NOT EXISTS asm_data_set_device_anim_set_groups ("
		"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
		"  device_id INTEGER,"
		"  gender_type_value_id INTEGER,"
		"  anim_set_dest_type_value_id INTEGER,"
		"  anim_set_res_id INTEGER"
		");"
		"CREATE TABLE IF NOT EXISTS asm_data_set_item_effect_groups ("
		"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
		"  item_id INTEGER,"
		"  effect_group_id INTEGER,"
		"  effect_group_type_value_id INTEGER"
		");"
		"CREATE TABLE IF NOT EXISTS asm_data_set_item_props ("
		"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
		"  item_id INTEGER,"
		"  prop_id INTEGER,"
		"  value REAL"
		");"
		"CREATE TABLE IF NOT EXISTS asm_data_set_item_mesh_asm_groups ("
		"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
		"  item_id INTEGER,"
		"  gender_type_value_id INTEGER,"
		"  item_mesh_asm_type_value_id INTEGER,"
		"  quality_value_id INTEGER,"
		"  asm_id INTEGER"
		");"
		"CREATE TABLE IF NOT EXISTS asm_data_set_prices ("
		"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
		"  item_id INTEGER,"
		"  price INTEGER,"
		"  currency_type_value_id INTEGER"
		");"
		"CREATE TABLE IF NOT EXISTS asm_data_set_devices_data_set_device_modes ("
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
		"  name_msg_id INTEGER,"
		"  name_msg_translated TEXT,"
		"  desc_msg_id INTEGER,"
		"  desc_msg_translated TEXT,"
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
		"  scale REAL,"
		"  cull_distance REAL,"
		"  scale_3d_x REAL,"
		"  scale_3d_y REAL,"
		"  scale_3d_z REAL,"
		"  translation_x REAL,"
		"  translation_y REAL,"
		"  translation_z REAL,"
		"  rotation_pitch REAL,"
		"  rotation_yaw REAL,"
		"  rotation_roll REAL,"
		"  collision_radius REAL,"
		"  collision_height REAL,"
		"  overlay_material_res_id INTEGER"
		");";
	if (!exec_required("shared gameplay tables", kSharedGameplaySchemaFloor)) return;

	const char* kDeviceModeColumnFloors[] = {
		"ALTER TABLE asm_data_set_devices_data_set_device_modes ADD COLUMN class_res_id INTEGER;",
		"ALTER TABLE asm_data_set_devices_data_set_device_modes ADD COLUMN damage_class_res_id INTEGER;",
		"ALTER TABLE asm_data_set_devices_data_set_device_modes ADD COLUMN device_projectile_id INTEGER;",
		"ALTER TABLE asm_data_set_devices_data_set_device_modes ADD COLUMN deployable_id INTEGER;",
		"ALTER TABLE asm_data_set_devices_data_set_device_modes ADD COLUMN bot_id INTEGER;",
		"ALTER TABLE asm_data_set_devices_data_set_device_modes ADD COLUMN impact_fx_group_id INTEGER;",
		"ALTER TABLE asm_data_set_devices_data_set_device_modes ADD COLUMN damage_type_value_id INTEGER;",
		"ALTER TABLE asm_data_set_devices_data_set_device_modes ADD COLUMN offhand_anim_res_id INTEGER;",
		"ALTER TABLE asm_data_set_devices_data_set_device_modes ADD COLUMN target_type_value_id INTEGER;",
		"ALTER TABLE asm_data_set_devices_data_set_device_modes ADD COLUMN target_type_affect_value_id INTEGER;",
		"ALTER TABLE asm_data_set_devices_data_set_device_modes ADD COLUMN attack_type_value_id INTEGER;",
		"ALTER TABLE asm_data_set_devices_data_set_device_modes ADD COLUMN remote_type_value_id INTEGER;",
		"ALTER TABLE asm_data_set_devices_data_set_device_modes ADD COLUMN camera_type_value_id INTEGER;",
		"ALTER TABLE asm_data_set_devices_data_set_device_modes ADD COLUMN reticule_res_id INTEGER;",
		"ALTER TABLE asm_data_set_devices_data_set_device_modes ADD COLUMN scope_material_res_id INTEGER;",
		"ALTER TABLE asm_data_set_devices_data_set_device_modes ADD COLUMN scale_fire_anim_flag INTEGER;",
		"ALTER TABLE asm_data_set_devices_data_set_device_modes ADD COLUMN interrupt_fire_anim_on_refire_flag INTEGER;",
		"ALTER TABLE asm_data_set_devices_data_set_device_modes ADD COLUMN require_los_flag INTEGER;",
		"ALTER TABLE asm_data_set_devices_data_set_device_modes ADD COLUMN continuous_fire_flag INTEGER;",
		"ALTER TABLE asm_data_set_devices_data_set_device_modes ADD COLUMN restrict_in_combat_flag INTEGER;",
		"ALTER TABLE asm_data_set_devices_data_set_device_modes ADD COLUMN require_aimmode_flag INTEGER;",
		"ALTER TABLE asm_data_set_devices_data_set_device_modes ADD COLUMN do_not_pause_ai_flag INTEGER;",
		"ALTER TABLE asm_data_set_devices_data_set_device_modes ADD COLUMN restrict_firing_flags INTEGER;",
		"ALTER TABLE asm_data_set_devices_data_set_device_modes ADD COLUMN restrict_physics_firing_flags INTEGER;",
		"ALTER TABLE asm_data_set_devices_data_set_device_modes ADD COLUMN return_to_idle_anim_secs REAL;",
		"ALTER TABLE asm_data_set_devices_data_set_device_modes ADD COLUMN icon_id INTEGER;",
		"ALTER TABLE asm_data_set_devices_data_set_device_modes ADD COLUMN attack_rating INTEGER;",
		"ALTER TABLE asm_data_set_devices_data_set_device_modes ADD COLUMN fire_mode_sequence INTEGER;",
	};
	for (const char* sql : kDeviceModeColumnFloors) {
		exec_ignore_error(sql);
	}
	exec_ignore_error("ALTER TABLE asm_data_set_impact_fx ADD COLUMN flag_0x1d9 INTEGER;");

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
			"  decal_random_rotation_flag INTEGER,"
			"  flag_0x1d9 INTEGER"
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

			// per-mode property overrides (base/min/max triple — see v20 notes)
			"CREATE TABLE IF NOT EXISTS asm_data_set_device_mode_properties ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  device_id INTEGER,"
			"  device_mode_id INTEGER,"
			"  prop_id INTEGER,"
			"  base_value REAL,"
			"  min_value REAL,"
			"  max_value REAL"
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

	if (version < 19) {
		// v19: re-seed ga_character_devices for characters affected by the
		// class-label rotation bug (fixed alongside this migration). Three of
		// four switch cases were misassigned, so every existing character with
		// profile_id 679 / 680 / 681 was equipped with another class's loadout.
		// Profile 567 (Medic) was unaffected.
		//
		// Historical: this migration mirrored a now-removed
		// PlayerSessionStore::SeedDefaultDevices. Live seeding now reads from
		// src/ControlServer/Loadouts/ClassLoadouts.cpp via
		// PlayerSessionStore::ResyncCharacterDevicesFromLoadout, which runs on
		// every character login — so on next login this migration's output is
		// overwritten with whatever the current ClassLoadouts.cpp says. Kept
		// here so freshly-restored old DBs still pass through the fix once.
		struct Seed { int deviceId; int slot; int slotValueId; int quality; };
		auto effectGroupId = [](int deviceId) -> int {
			switch (deviceId) {
				case 7031: return 26173;
				case 7032: return 26173;
				case 7033: return 26173;
				case 7034: return 26173;
				case 2991: return 16670;
				case 2531: return 16653;
				case 5800: return 22334;
				case 2906: return 9071;
				default:   return 0;
			}
		};
		auto seedFor = [](uint32_t profile_id) -> std::vector<Seed> {
			switch (profile_id) {
				case 679: // Robotics
					return {{5802,1,221,0},{6885,2,198,0},{2918,3,200,0},{7034,5,201,0},
					        {2300,7,203,0},{2066,8,204,0},{2886,10,386,0},{864,14,502,0}};
				case 680: // Assault
					return {{5801,1,221,0},{5788,2,198,0},{2914,3,200,0},{7031,5,201,0},
					        {3699,7,203,0},{2498,8,204,0},{5775,10,386,0},{864,14,502,0}};
				case 681: // Recon
					return {{5799,1,221,0},{2110,2,198,0},{3023,3,200,0},{7033,5,201,0},
					        {2219,7,203,0},{2129,8,204,0},{2113,10,386,0},{864,14,502,0}};
				default:
					return {};
			}
		};

		// Find every affected character.
		std::vector<std::pair<int64_t, uint32_t>> affected;
		sqlite3_stmt* qstmt = nullptr;
		if (sqlite3_prepare_v2(db,
		    "SELECT id, profile_id FROM ga_characters WHERE profile_id IN (679, 680, 681)",
		    -1, &qstmt, nullptr) == SQLITE_OK) {
			while (sqlite3_step(qstmt) == SQLITE_ROW) {
				affected.emplace_back(sqlite3_column_int64(qstmt, 0),
				                      static_cast<uint32_t>(sqlite3_column_int(qstmt, 1)));
			}
			sqlite3_finalize(qstmt);
		} else {
			Logger::Log("db", "v19: select affected characters failed: %s\n", sqlite3_errmsg(db));
		}

		sqlite3_stmt* delstmt = nullptr;
		sqlite3_stmt* insstmt = nullptr;
		if (sqlite3_prepare_v2(db,
		    "DELETE FROM ga_character_devices WHERE character_id = ?",
		    -1, &delstmt, nullptr) != SQLITE_OK) {
			Logger::Log("db", "v19: delete prepare failed: %s\n", sqlite3_errmsg(db));
		} else if (sqlite3_prepare_v2(db,
		    "INSERT INTO ga_character_devices "
		    "(character_id, device_id, equip_slot, slot_value_id, quality, inventory_id, effect_group_id) "
		    "VALUES (?, ?, ?, ?, ?, ?, ?)",
		    -1, &insstmt, nullptr) != SQLITE_OK) {
			Logger::Log("db", "v19: insert prepare failed: %s\n", sqlite3_errmsg(db));
			sqlite3_finalize(delstmt);
			delstmt = nullptr;
		} else {
			sqlite3_exec(db, "BEGIN TRANSACTION", nullptr, nullptr, nullptr);
			int fixed = 0;
			for (const auto& [character_id, profile_id] : affected) {
				const auto seeds = seedFor(profile_id);
				if (seeds.empty()) continue;

				sqlite3_reset(delstmt);
				sqlite3_bind_int64(delstmt, 1, character_id);
				sqlite3_step(delstmt);

				int invId = 10000;
				for (const auto& s : seeds) {
					sqlite3_reset(insstmt);
					sqlite3_bind_int64(insstmt, 1, character_id);
					sqlite3_bind_int(insstmt,   2, s.deviceId);
					sqlite3_bind_int(insstmt,   3, s.slot);
					sqlite3_bind_int(insstmt,   4, s.slotValueId);
					sqlite3_bind_int(insstmt,   5, s.quality);
					sqlite3_bind_int(insstmt,   6, invId++);
					sqlite3_bind_int(insstmt,   7, effectGroupId(s.deviceId));
					sqlite3_step(insstmt);
				}
				++fixed;
			}
			sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr);
			sqlite3_finalize(delstmt);
			sqlite3_finalize(insstmt);
			Logger::Log("db", "v19: re-seeded devices for %d characters with rotated loadouts\n", fixed);
		}
	}

	if (version < 20) {
		// v20: fix asm_data_set_device_mode_properties schema.
		//
		// The original walker (AsmDataCapture::WalkDeviceModeProperties) was
		// reading a non-existent `VALUE` float field and the wrong reader for
		// PROP_ID (byte instead of uint32).  All 9373 rows were captured with
		// prop_id truncated to 8 bits and value=0.0.  Ground truth from the
		// client's native loader (FUN_109a7d20):
		//
		//   PROP_ID    (0x03E7, TCP_UINT32)  →  TgProperty descriptor + 0x3C
		//   BASE_VALUE (0x0067, TCP_FLOAT)   →  TgProperty descriptor + 0x40
		//   MIN_VALUE  (0x035E, TCP_FLOAT)   →  TgProperty descriptor + 0x48
		//   MAX_VALUE  (0x034B, TCP_FLOAT)   →  TgProperty descriptor + 0x4C
		//
		// The in-memory descriptor also copies base_value → raw_value at +0x44,
		// but raw_value is not a marshal field — it's a runtime-only mirror.
		// Not stored here; callers that need it can treat base_value as raw.
		//
		// Only drop+recreate when the OLD schema is still present (legacy `value`
		// column).  Fresh installs hit the updated v17 CREATE which already uses
		// the new columns, so the drop would needlessly wipe freshly-captured
		// data — and if AsmDataCapture::bPopulateDatabase is off at that moment,
		// nothing refills the table.  Detect via pragma_table_info.
		bool needsRewrite = false;
		sqlite3_stmt* checkStmt = nullptr;
		if (sqlite3_prepare_v2(db,
			"SELECT 1 FROM pragma_table_info('asm_data_set_device_mode_properties') WHERE name='value';",
			-1, &checkStmt, nullptr) == SQLITE_OK) {
			if (sqlite3_step(checkStmt) == SQLITE_ROW) needsRewrite = true;
			sqlite3_finalize(checkStmt);
		}
		if (needsRewrite) {
			const char* kV20 =
				"DROP TABLE IF EXISTS asm_data_set_device_mode_properties;"
				"CREATE TABLE asm_data_set_device_mode_properties ("
				"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
				"  device_id INTEGER,"
				"  device_mode_id INTEGER,"
				"  prop_id INTEGER,"
				"  base_value REAL,"
				"  min_value REAL,"
				"  max_value REAL"
				");";
			result = sqlite3_exec(db, kV20, nullptr, nullptr, &err);
			if (result != SQLITE_OK) {
				Logger::Log("db", "Failed v20 migration: %s\n", err);
				return;
			}
			Logger::Log("db", "v20: old schema detected — dropped+recreated asm_data_set_device_mode_properties for recapture\n");
		} else {
			Logger::Log("db", "v20: table already on new schema — no data touched\n");
		}
	}

	if (version < 21) {
		// v21: re-verification of asm.dat capture against
		// CAssemblyManager::LoadAssemblyDatFile @ 0x10951030 found:
		//
		// (a) wrong field id in two material_* walkers:
		//     `material_res_id` was being read as GA_T::MATERIAL_RES_ID = 0x033D,
		//     but the loader (FUN_1094a650) reads MATERIAL_RESOURCE_ID = 0x033B
		//     into that slot. All `material_res_id` rows in
		//     asm_data_set_material_resources / mat_res_params are NULL/0/garbage.
		//
		// (b) missing columns (loader reads them, we never bound a column):
		//     - asm_data_set_items: wear_flair_start_date (W 0x551), wear_flair_duration (B 0x550)
		//     - asm_data_set_devices: in_hand_device_flag (Fg 0x2D4)
		//     - asm_data_set_bots: crew_control_radius (F 0xEC)
		//     - asm_data_set_assembly_meshes: 18 named flag bits packed into row+0x74
		//
		// (c) type mismatch on asm_mesh_audio_groups: loader (FUN_1094b470) reads
		//     AUDIO_GROUP_ID via Int32 (0x58, TYPE_TCP_UINT32); our walker used Byte.
		//     Same field is read as Byte in the sound_cues loader, so values likely
		//     fit in 8 bits in practice, but we should match the dataset-of-record.
		//
		// (d) missing dataset: DATA_SET_ACHIEVEMENT_REQS (0x10F) is nested under
		//     achievements (per FUN_1094d1b0 → FUN_1093f110 + FUN_1093dbf0). We
		//     never registered a walker.
		//
		// Strategy: for each affected existing table, DROP + CREATE with the new
		// schema so AsmDataCapture::ShouldWalk fires on the next launch and
		// re-populates with corrected reads. For the new ACHIEVEMENT_REQS table,
		// CREATE IF NOT EXISTS. Idempotent across fresh / v20 / v21 databases.
		//
		// All dropped tables are pure asm.dat captures (read-only mirrors of
		// game data) — no user data is at risk.
		const char* kV21 =
			// Items: add wear_flair_start_date / wear_flair_duration.
			"DROP TABLE IF EXISTS asm_data_set_items;"
			"CREATE TABLE asm_data_set_items ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  name_msg_id INTEGER,"
			"  name_msg_translated TEXT,"
			"  desc_msg_id INTEGER,"
			"  desc_msg_translated TEXT,"
			"  class_res_id INTEGER,"
			"  item_id INTEGER,"
			"  item_type_value_id INTEGER,"
			"  item_subtype_value_id INTEGER,"
			"  skill_id INTEGER,"
			"  sub_skill_id INTEGER,"
			"  skill_level_min INTEGER,"
			"  quantity INTEGER,"
			"  icon_id INTEGER,"
			"  weight FLOAT,"
			"  time_to_live_secs FLOAT,"
			"  quality_value_id INTEGER,"
			"  required_achievement_id INTEGER,"
			"  required_achievement_points INTEGER,"
			"  ref_bot_id INTEGER,"
			"  ref_deployable_id INTEGER,"
			"  ref_device_id INTEGER,"
			"  item_bind_type_value_id INTEGER,"
			"  production_cost INTEGER,"
			"  required_level INTEGER,"
			"  wear_flair_start_date TEXT,"
			"  wear_flair_duration INTEGER,"
			"  purchased_value INTEGER,"
			"  bundle_loot_table_id INTEGER"
			");"

			// Devices: add in_hand_device_flag.
			"DROP TABLE IF EXISTS asm_data_set_devices;"
			"CREATE TABLE asm_data_set_devices ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  device_id INTEGER,"
			"  form_class_res_id INTEGER,"
			"  mount_socket_res_id INTEGER,"
			"  time_to_equip_secs FLOAT,"
			"  container_skill_group_id INTEGER,"
			"  right_click_behavior_type_value_id INTEGER,"
			"  slot_used_value_id INTEGER,"
			"  in_hand_device_flag INTEGER"
			");"

			// Bots: add crew_control_radius.
			"DROP TABLE IF EXISTS asm_data_set_bots;"
			"CREATE TABLE asm_data_set_bots ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  bot_id INTEGER,"
			"  reference_name TEXT,"
			"  name_msg_id INTEGER,"
			"  desc_msg_id INTEGER,"
			"  level INTEGER,"
			"  pawn_class_res_id INTEGER,"
			"  controller_class_res_id INTEGER,"
			"  behavior_id INTEGER,"
			"  head_asm_id INTEGER,"
			"  body_asm_id INTEGER,"
			"  movement_asm_id INTEGER,"
			"  hit_points INTEGER,"
			"  bot_type_value_id INTEGER,"
			"  physical_type_value_id INTEGER,"
			"  default_slot_value_id INTEGER,"
			"  default_sensor_range REAL,"
			"  default_aggro_range INTEGER,"
			"  default_help_range INTEGER,"
			"  hearing_range REAL,"
			"  default_speed REAL,"
			"  walk_speed_pct REAL,"
			"  crouch_speed_pct REAL,"
			"  chase_range INTEGER,"
			"  chase_time_sec REAL,"
			"  stealth_sensor_range INTEGER,"
			"  stealth_aggro_range INTEGER,"
			"  hibernate_on_idle_sec INTEGER,"
			"  hibernate_delay_rate REAL,"
			"  icon_id INTEGER,"
			"  bot_rank_value_id INTEGER,"
			"  target_only_physical_type_value_id INTEGER,"
			"  skill_group_id INTEGER,"
			"  skill_group_set_id INTEGER,"
			"  fixed_fov_degrees INTEGER,"
			"  loot_table_id INTEGER,"
			"  default_power_pool INTEGER,"
			"  rotation_rate INTEGER,"
			"  class_type_value_id INTEGER,"
			"  device_slot_unlock_group_id INTEGER,"
			"  pickup_device_id INTEGER,"
			"  xp_value INTEGER,"
			"  currency_value INTEGER,"
			"  squad_role_value_id INTEGER,"
			"  default_posture_value_id INTEGER,"
			"  acceleration_rate REAL,"
			"  accuracy_override REAL,"
			"  bot_balance_multiplier REAL,"
			"  power_pool_regen_per_sec REAL,"
			"  crew_control_radius REAL,"
			"  hibernate_invulnerability_flag INTEGER,"
			"  can_jump_flag INTEGER,"
			"  can_climb_ladders_flag INTEGER,"
			"  path_only_flag INTEGER,"
			"  always_load_on_server_flag INTEGER,"
			"  destroy_on_owner_death_flag INTEGER"
			");"

			// Assembly meshes: add 18 named flag bit columns.
			"DROP TABLE IF EXISTS asm_data_set_assembly_meshes;"
			"CREATE TABLE asm_data_set_assembly_meshes ("
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
			"  scale REAL,"
			"  cull_distance REAL,"
			"  scale_3d_x REAL,"
			"  scale_3d_y REAL,"
			"  scale_3d_z REAL,"
			"  translation_x REAL,"
			"  translation_y REAL,"
			"  translation_z REAL,"
			"  rotator_pitch INTEGER,"
			"  rotator_yaw INTEGER,"
			"  rotator_roll INTEGER,"
			"  collision_height REAL,"
			"  collision_radius REAL,"
			"  collision_depth REAL,"
			"  crouch_height REAL,"
			"  hit_collision_height REAL,"
			"  hit_collision_radius REAL,"
			"  physics_weight REAL,"
			"  life_after_death_secs REAL,"
			"  destroyed_asm_id INTEGER,"
			"  material_res_group_id INTEGER,"
			"  race_material_parameter_id INTEGER,"
			"  accept_decals_flag INTEGER,"
			"  accept_decals_runtime_flag INTEGER,"
			"  accept_lights_flag INTEGER,"
			"  allow_approx_occlusion_flag INTEGER,"
			"  block_actors_flag INTEGER,"
			"  block_non_zero_extent_flag INTEGER,"
			"  block_rigid_body_flag INTEGER,"
			"  block_zero_extent_flag INTEGER,"
			"  cast_dynamic_shadow_flag INTEGER,"
			"  cast_shadow_flag INTEGER,"
			"  collide_actors_flag INTEGER,"
			"  force_dir_light_map_flag INTEGER,"
			"  has_physics_asset_inst_flag INTEGER,"
			"  is_female_flag INTEGER,"
			"  notify_rigid_body_collision_flag INTEGER,"
			"  only_owner_see_flag INTEGER,"
			"  owner_no_see_flag INTEGER,"
			"  update_joints_from_anim_flag INTEGER"
			");"

			// Material resources: parent FK column was reading the wrong field id
			// (GA_T::MATERIAL_RES_ID = 0x033D vs the loader's MATERIAL_RESOURCE_ID
			// = 0x033B). Drop+recreate so corrected walker re-populates.
			"DROP TABLE IF EXISTS asm_data_set_material_resources;"
			"CREATE TABLE asm_data_set_material_resources ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  material_res_group_id INTEGER,"
			"  material_res_id INTEGER,"
			"  material_res_sub_type_value_id INTEGER,"
			"  material_res_type_value_id INTEGER,"
			"  res_id INTEGER,"
			"  sort_order INTEGER"
			");"

			// Mat-res params: same fix — parent linkage was via wrong field id.
			"DROP TABLE IF EXISTS asm_data_set_mat_res_params;"
			"CREATE TABLE asm_data_set_mat_res_params ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  material_res_id INTEGER,"
			"  mat_res_param_id INTEGER,"
			"  material_parameter_id INTEGER,"
			"  parameter_res_id INTEGER"
			");"

			// Asm-mesh audio groups: type bug. Loader reads via Int32.
			// Schema column type unchanged (INTEGER); only the reader is fixed.
			// Drop+recapture so any rows that were silently truncated get
			// re-bound with the full uint32 value.
			"DROP TABLE IF EXISTS asm_data_set_asm_mesh_audio_groups;"
			"CREATE TABLE asm_data_set_asm_mesh_audio_groups ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  asm_id INTEGER,"
			"  audio_group_id INTEGER"
			");"

			// New: DATA_SET_ACHIEVEMENT_REQS (0x10F) — nested under achievements.
			// Per-row scalars come from FUN_1093f110 + FUN_1093dbf0.
			"CREATE TABLE IF NOT EXISTS asm_data_set_achievement_reqs ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  achievement_id INTEGER,"
			"  requirement_id INTEGER,"
			"  requirement_list_id INTEGER,"
			"  requirement_value REAL,"
			"  metric_value_id INTEGER,"
			"  map_game_id INTEGER,"
			"  gameplay_type_value_id INTEGER,"
			"  class_value_id INTEGER,"
			"  difficulty_value_id INTEGER,"
			"  team_size INTEGER"
			");"
			;
		result = sqlite3_exec(db, kV21, nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed v21 migration: %s\n", err);
			return;
		}
		Logger::Log("db", "v21: rebuilt items/devices/bots/assembly_meshes/material_resources/"
		            "mat_res_params/asm_mesh_audio_groups for recapture; created achievement_reqs\n");
	}

	if (version < 22) {
		// v22: deep-dive audit of vtable-dispatched per-row readers found:
		//
		// (a) `asm_data_set_skill_group_ranks` was capturing only
		//     {skill_group_id, skill_id} but the actual SKILL_GROUP_RANKS row
		//     reader (FUN_1094c250 = vtable[1] of CAmSkillRank @ 0x1168f16c)
		//     reads 8 more scalars: RANK_ID (B 0x417), name (Translate 0x371),
		//     desc (Translate 0x1F9), RANK (B 0x416), TRAINING_MAP_GAME_ID
		//     (B 0x515), DESC_TEXTURE_RES_ID (I 0x1FB), REQUIRED_XP_LEVEL_ID
		//     (B 0x434), AUTO_ALLOCATE_FLAG (Fg 0x5C). Plus nested
		//     DATA_SET_SKILL_EFFECT_GROUPS per rank row.
		//
		// (b) `asm_data_set_skill_effect_groups` lost the rank_id parent
		//     context when fired from a skill-group-rank row. Add `rank_id`.
		//
		// (c) `asm_data_set_fx.name` was a wchar read of field 0x370, but the
		//     loader (FUN_1094a800) reads 0x370 as int32-resource-id and
		//     resolves through DAT_119a1c80. Rename to `name_res_id` and
		//     read as Int32; consumers join with asm_data_set_resources for
		//     the human-readable name.
		//
		// All three tables are pure asm.dat captures — drop+recreate then let
		// AsmDataCapture::ShouldWalk fire on the next launch to repopulate.
		const char* kV22 =
			// Skill group ranks: full row schema.
			"DROP TABLE IF EXISTS asm_data_set_skill_group_ranks;"
			"CREATE TABLE asm_data_set_skill_group_ranks ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  skill_group_id INTEGER,"
			"  skill_id INTEGER,"
			"  rank_id INTEGER,"
			"  name_msg_translated TEXT,"
			"  desc_msg_translated TEXT,"
			"  rank INTEGER,"
			"  training_map_game_id INTEGER,"
			"  desc_texture_res_id INTEGER,"
			"  required_xp_level_id INTEGER,"
			"  auto_allocate_flag INTEGER"
			");"

			// Skill effect groups: add rank_id parent linkage.
			"DROP TABLE IF EXISTS asm_data_set_skill_effect_groups;"
			"CREATE TABLE asm_data_set_skill_effect_groups ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  skill_group_id INTEGER,"
			"  skill_id INTEGER,"
			"  sub_skill_id INTEGER,"
			"  rank_id INTEGER,"
			"  effect_group_id INTEGER,"
			"  effect_group_type_value_id INTEGER"
			");"

			// FX: rename `name` → `name_res_id`. Loader reads 0x370 via
			// get_int32_t (resource-id), not get_wchar_2.
			"DROP TABLE IF EXISTS asm_data_set_fx;"
			"CREATE TABLE asm_data_set_fx ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  fx_id INTEGER,"
			"  name_res_id INTEGER,"
			"  priority_value_id INTEGER,"
			"  mic_res_id INTEGER,"
			"  transition_sec REAL"
			");"
			;
		result = sqlite3_exec(db, kV22, nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed v22 migration: %s\n", err);
			return;
		}
		Logger::Log("db", "v22: rebuilt skill_group_ranks (full 10-col schema), "
		            "skill_effect_groups (added rank_id), and fx (name→name_res_id)\n");
	}

	if (version < 23) {
		// v23: capture message translations from lang_English.dat.
		//
		// FUN_1093ebb0 (locale-specific) and FUN_1093d1a0 (English fallback)
		// both call CMarshal__get_array(0x017A = DATA_SET_MSG_TRANSLATIONS) on
		// the language file's marshal context. The per-row schema (per
		// FUN_10939430) is:
		//   MSG_ID       (I 0x036E)  — translation key
		//   MESSAGE      (W 0x0355)  — translation text
		//   SOUND_RES_ID (I 0x0493)  — linked voice clip res id (often 0)
		//
		// On non-English locales, lang_<locale>.dat loads first (giving
		// localized text), then lang_English.dat loads as fallback (giving
		// English). Using `msg_id INTEGER PRIMARY KEY` with INSERT OR
		// REPLACE in the walker means the English pass overwrites — final
		// table holds English content regardless of locale.
		const char* kV23 =
			"CREATE TABLE IF NOT EXISTS asm_data_set_msg_translations ("
			"  msg_id INTEGER PRIMARY KEY,"
			"  message TEXT,"
			"  sound_res_id INTEGER"
			");";
		result = sqlite3_exec(db, kV23, nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed v23 migration: %s\n", err);
			return;
		}
		Logger::Log("db", "v23: created asm_data_set_msg_translations\n");
	}

	if (version < 24) {
		// v24: add mod_effect_group_ids to ga_character_devices.
		//
		// Stores per-equipped-item rolled-mod effect_group_ids as a CSV list
		// (e.g. "24208,24211,24212,24208,24211,24212" → six 'h' letters).
		// Source-of-truth is src/ControlServer/Loadouts/ClassLoadouts.cpp;
		// PlayerSessionStore::ResyncCharacterDevicesFromLoadout overwrites
		// this column on character SELECT, so editing the loadout file +
		// restart is enough to take effect.
		//
		// Idempotent: ControlServer/Database.cpp resets version_info=19 at
		// every boot, so this can re-run after the column already exists —
		// swallow the duplicate-column error and let the version bump below
		// flag the table as fully migrated.
		result = sqlite3_exec(db,
			"ALTER TABLE ga_character_devices ADD COLUMN mod_effect_group_ids TEXT NOT NULL DEFAULT '';",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			sqlite3_free(err);
			err = nullptr;
		} else {
			Logger::Log("db", "v24: added ga_character_devices.mod_effect_group_ids\n");
		}
	}

	if (version < 25) {
		// v25: add `oc` flag to ga_character_devices.
		//
		// 0/1 — when 1, send_inventory_response picks an Overclocked-named
		// blueprint_id (asm_data_set_blueprints.override_name_msg_id != 0,
		// e.g. 6424 → "Focused Repair Arm OC") so the client renders the
		// "OC" name suffix. Cosmetic-only; stats still come from the rolled
		// mods + device base data. Wiped & rewritten by
		// ResyncCharacterDevicesFromLoadout from g.mods.oc on every login.
		// Idempotent — swallow duplicate-column error.
		result = sqlite3_exec(db,
			"ALTER TABLE ga_character_devices ADD COLUMN oc INTEGER NOT NULL DEFAULT 0;",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			sqlite3_free(err);
			err = nullptr;
		} else {
			Logger::Log("db", "v25: added ga_character_devices.oc\n");
		}
	}

	if (version < 26) {
		// v26: obj_map_object — informative lookup table populated by
		// MapDataDumper when the server is launched with -dumpmapdata=1.
		// One row per placed actor that exposes m_nMapObjectId (player
		// starts, bot starts, factories, mission objectives, …).
		//
		// `group` is a SQL reserved word in some dialects, so the column
		// is double-quoted at definition and must be quoted at read time
		// too (`SELECT "group" FROM obj_map_object …`).
		//
		// UNIQUE is on (map_name, map_object_id), not map_object_id alone,
		// because the editor's id counter restarts per map — different
		// maps will share ids freely. Re-dumping the same map updates
		// rows in place via INSERT OR REPLACE in the dumper.
		const char* kV26 =
			"CREATE TABLE IF NOT EXISTS obj_map_object ("
			"  id            INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  map_name      TEXT NOT NULL,"
			"  class_name    TEXT NOT NULL,"
			"  map_object_id INTEGER NOT NULL,"
			"  \"group\"     TEXT,"
			"  tag           TEXT,"
			"  UNIQUE(map_name, map_object_id)"
			");";
		result = sqlite3_exec(db, kV26, nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed v26 migration: %s\n", err);
			return;
		}
		Logger::Log("db", "v26: created obj_map_object\n");
	}

	if (version < 27) {
		// v27: fix 1P_CPLab05_P boss room objective wiring + map the 16
		// previously-unmapped alarm factories on the same level.
		//
		// (a) obj_mission_objective_bots had the Room-Bossroom objective
		//     (map_object_id=11324) linked to factory 12741, whose spawn
		//     table 46 is the bossroom adds (drones, helot/techro, Support
		//     Destroyer as the final group). SpawnObjectiveBot drains the
		//     entire factory queue and stamps the LAST spawned bot as
		//     r_ObjectiveBot, so the Support Destroyer was claiming the boss
		//     slot and the actual boss factory (12698, spawn table 41 =
		//     Reaper/Vanguard/Viking) never fired. Re-point the objective
		//     at 12698 so the boss becomes the objective. The adds factory
		//     (12741) falls back to bAutoSpawn=1 and relies on the binary's
		//     PostBeginPlay autospawn driver to keep spawning, same as
		//     factories 12708..12714 on this map.
		//
		// (b) The 12 "BOT - Alarm Bots" factories use spawn table 33
		//     (Alarm Responders + Elite Assassins/Helots) and the 4
		//     "BOT - Alarm Boss,Room - Bossroom" factories use spawn table
		//     59 (mixed alarm responders + boss-room elites). Same
		//     task_force_number=2 / mutator_number=0 as the other 8 mapped
		//     factories on this level.
		result = sqlite3_exec(db,
			"UPDATE obj_mission_objective_bots "
			"SET bot_factory_id = 12698 "
			"WHERE map_object_id = 11324;",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed v27 migration (a): %s\n", err);
			return;
		}

		result = sqlite3_exec(db,
			"INSERT INTO obj_bot_factories (map_object_id, bot_spawn_table_id, task_force_number, mutator_number) VALUES "
			"(12724, 33, 2, 0), "
			"(12727, 33, 2, 0), "
			"(12728, 33, 2, 0), "
			"(12729, 33, 2, 0), "
			"(12730, 33, 2, 0), "
			"(12731, 33, 2, 0), "
			"(12732, 33, 2, 0), "
			"(12733, 33, 2, 0), "
			"(12734, 33, 2, 0), "
			"(12735, 33, 2, 0), "
			"(12736, 33, 2, 0), "
			"(12737, 33, 2, 0), "
			"(12738, 59, 2, 0), "
			"(12739, 59, 2, 0), "
			"(12740, 59, 2, 0), "
			"(12744, 59, 2, 0);",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed v27 migration (b): %s\n", err);
			return;
		}
		Logger::Log("db", "v27: repointed 1P_CPLab05_P bossroom objective 11324 → factory 12698, "
		                  "added 16 alarm-factory mappings (12× table 33, 4× table 59)\n");
	}

	if (version < 28) {
		// v28: add location_x / location_y / location_z to obj_map_object.
		//
		// The dumper now writes the AActor->Location of each placed actor so
		// we can render factory positions top-down per map (UE3 stores world
		// units in cm; X/Y are the horizontal plane, Z is altitude). The new
		// data is filled on the next -dumpmapdata=1 run; rows from prior runs
		// keep their default 0/0/0 until the map is redumped.
		//
		// Idempotent: ALTER TABLE ADD COLUMN fails after the column exists,
		// so swallow the error per v24/v25 pattern. Done as three separate
		// statements so each column's add stands or fails independently.
		const char* kV28[] = {
			"ALTER TABLE obj_map_object ADD COLUMN location_x REAL NOT NULL DEFAULT 0;",
			"ALTER TABLE obj_map_object ADD COLUMN location_y REAL NOT NULL DEFAULT 0;",
			"ALTER TABLE obj_map_object ADD COLUMN location_z REAL NOT NULL DEFAULT 0;",
		};
		int added = 0;
		for (const char* stmt : kV28) {
			result = sqlite3_exec(db, stmt, nullptr, nullptr, &err);
			if (result != SQLITE_OK) {
				sqlite3_free(err);
				err = nullptr;
			} else {
				added++;
			}
		}
		if (added > 0) {
			Logger::Log("db", "v28: added %d location columns to obj_map_object\n", added);
		}
	}

	if (version < 29) {
		// v29: obj_map_object_location_list — per-factory spawn-point cloud.
		//
		// ATgBotFactory.LocationList is a TArray<ANavigationPoint*> populated
		// by the level designer; SpawnNextBot picks one of these per spawn,
		// so the bot's actual world position at spawn time is one of the list
		// entries (NOT the factory's own Location, which is usually the
		// editor sprite icon). The dumper iterates the list per factory and
		// writes one row per (map_object_id, index_in_list).
		//
		// The (map_name, map_object_id, index_in_list) composite uniqueness
		// makes re-dumps idempotent for entries at the same index; the
		// dumper deletes all rows for the current map up front so that
		// removed list entries don't linger across re-dumps.
		const char* kV29 =
			"CREATE TABLE IF NOT EXISTS obj_map_object_location_list ("
			"  id              INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  map_name        TEXT NOT NULL,"
			"  map_object_id   INTEGER NOT NULL,"
			"  index_in_list   INTEGER NOT NULL,"
			"  location_x      REAL NOT NULL,"
			"  location_y      REAL NOT NULL,"
			"  location_z      REAL NOT NULL,"
			"  UNIQUE(map_name, map_object_id, index_in_list)"
			");";
		result = sqlite3_exec(db, kV29, nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed v29 migration: %s\n", err);
			return;
		}
		Logger::Log("db", "v29: created obj_map_object_location_list\n");
	}

	if (version < 30) {
		// v30: 1P_CPLab03 factory mappings + group annotations + new
		// fallback columns on obj_mission_objective_bots.
		//
		// 1P_CPLab03's bossroom doesn't use a dedicated boss factory like
		// CPLab05 does — Kismet spawns the boss bot directly. To express
		// that case we widen obj_mission_objective_bots with two
		// alternative resolution columns: bot_id (spawn this exact bot)
		// and spawn_table_id (pick one bot from this table). The DLL-side
		// fallback chain in TgMissionObjective_Bot__SpawnObjectiveBot is:
		//   1. bot_factory_id (legacy)
		//   2. bot_id (this column)
		//   3. spawn_table_id (this column, weighted pick from group 1).
		// ALTERs are idempotent — error-swallow per v24/v25 pattern.
		{
			const char* kV30_alter[] = {
				"ALTER TABLE obj_mission_objective_bots ADD COLUMN bot_id         INTEGER;",
				"ALTER TABLE obj_mission_objective_bots ADD COLUMN spawn_table_id INTEGER;",
			};
			for (const char* stmt : kV30_alter) {
				result = sqlite3_exec(db, stmt, nullptr, nullptr, &err);
				if (result != SQLITE_OK) { sqlite3_free(err); err = nullptr; }
			}
		}

		// The 1P_CPLab03 boss uses the spawn_table_id fallback — table 36 is
		// Boss Think-Tank (0.8) / Boss Vanguard (0.2). No factory exists.
		result = sqlite3_exec(db,
			"INSERT INTO obj_mission_objective_bots (map_object_id, spawn_table_id) "
			"VALUES (12485, 36);",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed v30 migration (objective insert): %s\n", err);
			return;
		}

		// Output of tools/map_planner.py after manually identifying each
		// factory on this map (none had editor-set groups, so they were
		// labelled by hand using top-down screenshots + the spawn-point
		// clouds dumped by v29).
		const char* kV30_groups =
			"UPDATE obj_map_object SET \"group\" = 'ballistas and sentinels - first hallway' WHERE map_name = '1P_CPLab03' AND map_object_id = 12488;"
			"UPDATE obj_map_object SET \"group\" = 'generic group of enemies'                WHERE map_name = '1P_CPLab03' AND map_object_id = 12489;"
			"UPDATE obj_map_object SET \"group\" = 'generic group of enemies'                WHERE map_name = '1P_CPLab03' AND map_object_id = 12490;"
			"UPDATE obj_map_object SET \"group\" = 'generic group of enemies'                WHERE map_name = '1P_CPLab03' AND map_object_id = 12491;"
			"UPDATE obj_map_object SET \"group\" = 'generic group of enemies'                WHERE map_name = '1P_CPLab03' AND map_object_id = 12492;"
			"UPDATE obj_map_object SET \"group\" = 'Boss room - initial adds'                WHERE map_name = '1P_CPLab03' AND map_object_id = 12493;"
			"UPDATE obj_map_object SET \"group\" = 'Boss room - alarm adds'                  WHERE map_name = '1P_CPLab03' AND map_object_id = 12494;"
			"UPDATE obj_map_object SET \"group\" = 'support??'                               WHERE map_name = '1P_CPLab03' AND map_object_id = 12496;"
			"UPDATE obj_map_object SET \"group\" = 'alarm responders'                        WHERE map_name = '1P_CPLab03' AND map_object_id = 12497;"
			"UPDATE obj_map_object SET \"group\" = 'alarm responders'                        WHERE map_name = '1P_CPLab03' AND map_object_id = 12498;"
			"UPDATE obj_map_object SET \"group\" = 'alarm responders'                        WHERE map_name = '1P_CPLab03' AND map_object_id = 12499;"
			"UPDATE obj_map_object SET \"group\" = 'alarm responders'                        WHERE map_name = '1P_CPLab03' AND map_object_id = 12500;"
			"UPDATE obj_map_object SET \"group\" = 'alarm responders'                        WHERE map_name = '1P_CPLab03' AND map_object_id = 12501;"
			"UPDATE obj_map_object SET \"group\" = 'alarm responders'                        WHERE map_name = '1P_CPLab03' AND map_object_id = 12502;"
			"UPDATE obj_map_object SET \"group\" = 'alarm responders'                        WHERE map_name = '1P_CPLab03' AND map_object_id = 12503;"
			"UPDATE obj_map_object SET \"group\" = 'alarm responders'                        WHERE map_name = '1P_CPLab03' AND map_object_id = 12504;"
			"UPDATE obj_map_object SET \"group\" = 'alarm responders'                        WHERE map_name = '1P_CPLab03' AND map_object_id = 12505;";
		result = sqlite3_exec(db, kV30_groups, nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed v30 migration (group updates): %s\n", err);
			return;
		}

		const char* kV30_factories =
			"INSERT INTO obj_bot_factories (map_object_id, bot_spawn_table_id, task_force_number, mutator_number) VALUES "
			"(12488, 29, 2, 0),"
			"(12489, 28, 2, 0),"
			"(12490, 28, 2, 0),"
			"(12491, 28, 2, 0),"
			"(12492, 28, 2, 0),"
			"(12493, 46, 2, 0),"
			"(12494, 64, 2, 0),"
			"(12495, 34, 2, 0),"
			"(12496, 34, 2, 0),"
			"(12497, 33, 2, 0),"
			"(12498, 33, 2, 0),"
			"(12499, 33, 2, 0),"
			"(12500, 33, 2, 0),"
			"(12501, 33, 2, 0),"
			"(12502, 33, 2, 0),"
			"(12503, 33, 2, 0),"
			"(12504, 33, 2, 0),"
			"(12505, 33, 2, 0);";
		result = sqlite3_exec(db, kV30_factories, nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed v30 migration (factory inserts): %s\n", err);
			return;
		}
		Logger::Log("db", "v30: 1P_CPLab03 — 17 group annotations + 18 factory mappings\n");
	}

	if (version < 31) {
		// v31: obj_mission_objective_bots.task_force_number.
		//
		// The factory-driven path (tier 1 in SpawnObjectiveBot) already
		// picks up task force from the linked TgBotFactory's s_nTaskForce.
		// The bot_id / spawn_table_id fallbacks (tiers 2 and 3, added in
		// v30) had no equivalent — bots ended up on whatever team
		// SpawnBotById defaulted them to. Add the column and seed it for
		// the CPLab03 boss row so the boss reliably joins the defender
		// task force (PvE convention).
		result = sqlite3_exec(db,
			"ALTER TABLE obj_mission_objective_bots ADD COLUMN task_force_number INTEGER;",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			// Already exists on a re-run — fine, keep going.
			sqlite3_free(err);
			err = nullptr;
		}

		result = sqlite3_exec(db,
			"UPDATE obj_mission_objective_bots SET task_force_number = 2 "
			"WHERE map_object_id = 12485;",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed v31 migration (CPLab03 boss tf update): %s\n", err);
			return;
		}
		Logger::Log("db", "v31: added obj_mission_objective_bots.task_force_number, "
		                  "set CPLab03 boss objective 12485 → tf 2\n");
	}

	if (version < 32) {
		// v32: 1P_SDColony03_P factory mappings + group annotations + boss
		// objective. Map planner output. 18 factories cover the level
		// (mostly Colony Drones/Soldiers from table 167; one Wasp spawner,
		// one Tick spawner, one Control Panel, one Juggernaut wave); the
		// bossroom objective 13854 falls back to spawn_table 174 (Boss
		// Vulcan), same defender task force as the rest.
		const char* kV32_groups =
			"UPDATE obj_map_object SET \"group\" = 'guardian'                WHERE map_name = '1P_SDColony03_P' AND map_object_id = 13807;"
			"UPDATE obj_map_object SET \"group\" = 'generic spawn'           WHERE map_name = '1P_SDColony03_P' AND map_object_id = 13842;"
			"UPDATE obj_map_object SET \"group\" = 'generic spawn'           WHERE map_name = '1P_SDColony03_P' AND map_object_id = 13850;"
			"UPDATE obj_map_object SET \"group\" = 'generic spawn'           WHERE map_name = '1P_SDColony03_P' AND map_object_id = 13851;"
			"UPDATE obj_map_object SET \"group\" = 'maybe wasps or I dunno'  WHERE map_name = '1P_SDColony03_P' AND map_object_id = 13852;"
			"UPDATE obj_map_object SET \"group\" = 'control panel?'          WHERE map_name = '1P_SDColony03_P' AND map_object_id = 13853;"
			"UPDATE obj_map_object SET \"group\" = 'generic spawn'           WHERE map_name = '1P_SDColony03_P' AND map_object_id = 13966;"
			"UPDATE obj_map_object SET \"group\" = 'generic spawn'           WHERE map_name = '1P_SDColony03_P' AND map_object_id = 13967;"
			"UPDATE obj_map_object SET \"group\" = 'generic spawn'           WHERE map_name = '1P_SDColony03_P' AND map_object_id = 13968;";
		result = sqlite3_exec(db, kV32_groups, nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed v32 migration (group updates): %s\n", err);
			return;
		}

		const char* kV32_factories =
			"INSERT INTO obj_bot_factories (map_object_id, bot_spawn_table_id, task_force_number, mutator_number) VALUES "
			"(13807, 168, 2, 0),"
			"(13808, 167, 2, 0),"
			"(13836, 167, 2, 0),"
			"(13837, 167, 2, 0),"
			"(13838, 167, 2, 0),"
			"(13840, 167, 2, 0),"
			"(13841, 167, 2, 0),"
			"(13842, 167, 2, 0),"
			"(13850, 167, 2, 0),"
			"(13851,  99, 2, 0),"
			"(13852,  98, 2, 0),"
			"(13853, 173, 2, 0),"
			"(13966, 167, 2, 0),"
			"(13967, 167, 2, 0),"
			"(13968, 167, 2, 0),"
			"(13969, 148, 2, 0),"
			"(13970, 102, 2, 0),"
			"(13971, 147, 2, 0);";
		result = sqlite3_exec(db, kV32_factories, nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed v32 migration (factory inserts): %s\n", err);
			return;
		}

		// Mission objective 13854 → spawn table 174 (Boss Vulcan, tier-3
		// fallback in SpawnObjectiveBot), tf 2 to match the rest of the map.
		result = sqlite3_exec(db,
			"INSERT INTO obj_mission_objective_bots "
			"(map_object_id, spawn_table_id, task_force_number) "
			"VALUES (13854, 174, 2);",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed v32 migration (objective insert): %s\n", err);
			return;
		}
		Logger::Log("db", "v32: 1P_SDColony03_P — 9 group annotations + 18 factory mappings + "
		                  "boss objective 13854 → spawn table 174\n");
	}

	if (version < 33) {
		// v33: 34 map_* tables for raw map dump data. Populated by MapDataDumper
		// when the server boots with -dumpmapdata=1. Each table holds one row
		// per actor of that class (or one row per inherited ancestor for
		// subclass instances). map_object_id is the linker across tables.
		//
		// Common columns on every map_<class> table:
		//   id, map_name, map_object_id, class_name, tag, "group",
		//   location_x, location_y, location_z,
		//   rotation_pitch, rotation_yaw, rotation_roll
		// then class-specific scalar columns.

		const char* kV33_team_player_start =
			"CREATE TABLE IF NOT EXISTS map_tg_team_player_start ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  map_name TEXT NOT NULL,"
			"  map_object_id INTEGER,"
			"  class_name TEXT NOT NULL,"
			"  tag TEXT,"
			"  \"group\" TEXT,"
			"  location_x REAL, location_y REAL, location_z REAL,"
			"  rotation_pitch INTEGER, rotation_yaw INTEGER, rotation_roll INTEGER,"
			"  m_n_task_force INTEGER,"
			"  m_e_coalition INTEGER,"
			"  m_n_priority INTEGER,"
			"  n_prev_priority INTEGER,"
			"  m_n_min_level INTEGER"
			");"
			"CREATE INDEX IF NOT EXISTS idx_map_tg_team_player_start_map_object_id "
			"  ON map_tg_team_player_start(map_object_id);";
		result = sqlite3_exec(db, kV33_team_player_start, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v33 (team_player_start): %s\n", err); return; }

		const char* kV33_start_point =
			"CREATE TABLE IF NOT EXISTS map_tg_start_point ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  map_name TEXT NOT NULL,"
			"  map_object_id INTEGER,"
			"  class_name TEXT NOT NULL,"
			"  tag TEXT,"
			"  \"group\" TEXT,"
			"  location_x REAL, location_y REAL, location_z REAL,"
			"  rotation_pitch INTEGER, rotation_yaw INTEGER, rotation_roll INTEGER,"
			"  m_n_start_group INTEGER,"
			"  m_n_return_map_type INTEGER,"
			"  m_n_return_map_game_id INTEGER,"
			"  m_f_start_rating REAL,"
			"  m_f_current_rating REAL,"
			"  m_f_reset_rating REAL,"
			"  m_f_decrease_rate REAL,"
			"  availability_quest_group_id INTEGER"
			");"
			"CREATE INDEX IF NOT EXISTS idx_map_tg_start_point_map_object_id "
			"  ON map_tg_start_point(map_object_id);";
		result = sqlite3_exec(db, kV33_start_point, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v33 (start_point): %s\n", err); return; }

		const char* kV33_mission_objective =
			"CREATE TABLE IF NOT EXISTS map_tg_mission_objective ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  map_name TEXT NOT NULL,"
			"  map_object_id INTEGER,"
			"  class_name TEXT NOT NULL,"
			"  tag TEXT,"
			"  \"group\" TEXT,"
			"  location_x REAL, location_y REAL, location_z REAL,"
			"  rotation_pitch INTEGER, rotation_yaw INTEGER, rotation_roll INTEGER,"
			"  b_has_no_location INTEGER,"
			"  r_b_use_pending_state INTEGER,"
			"  s_b_change_coalition_when_captured INTEGER,"
			"  m_b_capture_only_once INTEGER,"
			"  r_b_has_been_captured_once INTEGER,"
			"  s_b_end_overtime_on_defender_progress INTEGER,"
			"  b_enabled INTEGER,"
			"  r_b_is_locked INTEGER,"
			"  r_b_is_active INTEGER,"
			"  r_b_is_pending INTEGER,"
			"  m_b_pause_on_capture INTEGER,"
			"  c_b_local_pawn_is_attacker INTEGER,"
			"  m_b_block_advance INTEGER,"
			"  m_b_is_base_objective INTEGER,"
			"  c_b_is_local_player_attacker INTEGER,"
			"  c_b_local_player_attacker_cached INTEGER,"
			"  m_b_in_matinee_update INTEGER,"
			"  m_b_start_objective INTEGER,"
			"  s_b_random_picked INTEGER,"
			"  m_b_teleport_bots INTEGER,"
			"  n_priority INTEGER,"
			"  n_message_id INTEGER,"
			"  n_default_owner_task_force INTEGER,"
			"  n_objective_id INTEGER,"
			"  hex_bonus_direction INTEGER,"
			"  r_open_world_player_default_role INTEGER,"
			"  r_e_default_coalition INTEGER,"
			"  r_e_status INTEGER,"
			"  r_e_owning_coalition INTEGER,"
			"  s_open_world_bot_task_force INTEGER,"
			"  m_f_proximity_radius INTEGER,"
			"  m_f_proximity_height REAL,"
			"  m_f_time_to_capture REAL,"
			"  m_f_time_to_hold REAL,"
			"  m_n_points_per_second INTEGER,"
			"  m_n_type_id INTEGER,"
			"  m_n_desc_msg_id INTEGER,"
			"  m_n_icon_id INTEGER,"
			"  m_n_min_agents INTEGER,"
			"  m_n_cooldown_seconds INTEGER,"
			"  m_n_points INTEGER,"
			"  m_n_credits INTEGER,"
			"  m_n_reward_xp INTEGER,"
			"  m_n_loot_table_id INTEGER,"
			"  m_f_curr_capture_time REAL,"
			"  m_n_curr_owner_taskforce INTEGER,"
			"  r_n_owner_task_force INTEGER,"
			"  r_f_curr_capture_time REAL,"
			"  m_n_capture_time_ext_secs INTEGER,"
			"  m_n_capture_time_reset_secs INTEGER,"
			"  s_f_attacker_captured_at REAL,"
			"  r_f_last_completed_time REAL,"
			"  c_f_percentage REAL,"
			"  s_f_previous_play_rate REAL,"
			"  m_f_start_time REAL,"
			"  m_f_stop_time REAL,"
			"  s_n_previous_priority INTEGER,"
			"  s_n_current_spawn_order INTEGER"
			");"
			"CREATE INDEX IF NOT EXISTS idx_map_tg_mission_objective_map_object_id "
			"  ON map_tg_mission_objective(map_object_id);";
		result = sqlite3_exec(db, kV33_mission_objective, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v33 (mission_objective): %s\n", err); return; }

		const char* kV33_mission_objective_bot =
			"CREATE TABLE IF NOT EXISTS map_tg_mission_objective_bot ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  map_name TEXT NOT NULL,"
			"  map_object_id INTEGER,"
			"  class_name TEXT NOT NULL,"
			"  tag TEXT,"
			"  \"group\" TEXT,"
			"  location_x REAL, location_y REAL, location_z REAL,"
			"  rotation_pitch INTEGER, rotation_yaw INTEGER, rotation_roll INTEGER,"
			"  s_n_bot_id INTEGER,"
			"  s_n_spawn_table_id INTEGER,"
			"  s_b_auto_spawn INTEGER,"
			"  b_patrol_loop INTEGER,"
			"  f_balance REAL,"
			"  n_global_alarm_id INTEGER"
			");"
			"CREATE INDEX IF NOT EXISTS idx_map_tg_mission_objective_bot_map_object_id "
			"  ON map_tg_mission_objective_bot(map_object_id);";
		result = sqlite3_exec(db, kV33_mission_objective_bot, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v33 (mission_objective_bot): %s\n", err); return; }

		const char* kV33_mission_objective_kismet =
			"CREATE TABLE IF NOT EXISTS map_tg_mission_objective_kismet ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  map_name TEXT NOT NULL,"
			"  map_object_id INTEGER,"
			"  class_name TEXT NOT NULL,"
			"  tag TEXT,"
			"  \"group\" TEXT,"
			"  location_x REAL, location_y REAL, location_z REAL,"
			"  rotation_pitch INTEGER, rotation_yaw INTEGER, rotation_roll INTEGER"
			");"
			"CREATE INDEX IF NOT EXISTS idx_map_tg_mission_objective_kismet_map_object_id "
			"  ON map_tg_mission_objective_kismet(map_object_id);";
		result = sqlite3_exec(db, kV33_mission_objective_kismet, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v33 (mission_objective_kismet): %s\n", err); return; }

		const char* kV33_mission_objective_proximity =
			"CREATE TABLE IF NOT EXISTS map_tg_mission_objective_proximity ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  map_name TEXT NOT NULL,"
			"  map_object_id INTEGER,"
			"  class_name TEXT NOT NULL,"
			"  tag TEXT,"
			"  \"group\" TEXT,"
			"  location_x REAL, location_y REAL, location_z REAL,"
			"  rotation_pitch INTEGER, rotation_yaw INTEGER, rotation_roll INTEGER,"
			"  s_b_allow_ai_bot_interaction INTEGER,"
			"  s_b_allow_remote_control_bot_interaction INTEGER,"
			"  m_f_def_capture_rate REAL,"
			"  m_f_capture_accel_rate REAL,"
			"  m_f_capture_accel_rate_cap REAL,"
			"  r_f_capture_rate REAL,"
			"  m_f_overtime_accel_rate REAL"
			");"
			"CREATE INDEX IF NOT EXISTS idx_map_tg_mission_objective_proximity_map_object_id "
			"  ON map_tg_mission_objective_proximity(map_object_id);";
		result = sqlite3_exec(db, kV33_mission_objective_proximity, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v33 (mission_objective_proximity): %s\n", err); return; }

		const char* kV33_base_objective_ctf_bot =
			"CREATE TABLE IF NOT EXISTS map_tg_base_objective_ctf_bot ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  map_name TEXT NOT NULL,"
			"  map_object_id INTEGER,"
			"  class_name TEXT NOT NULL,"
			"  tag TEXT,"
			"  \"group\" TEXT,"
			"  location_x REAL, location_y REAL, location_z REAL,"
			"  rotation_pitch INTEGER, rotation_yaw INTEGER, rotation_roll INTEGER,"
			"  m_f_flag_respawn_delay REAL,"
			"  m_f_scoring_radius REAL,"
			"  m_b_spawn_unaligned_bot INTEGER,"
			"  m_b_capture_on_death INTEGER"
			");"
			"CREATE INDEX IF NOT EXISTS idx_map_tg_base_objective_ctf_bot_map_object_id "
			"  ON map_tg_base_objective_ctf_bot(map_object_id);";
		result = sqlite3_exec(db, kV33_base_objective_ctf_bot, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v33 (base_objective_ctf_bot): %s\n", err); return; }

		const char* kV33_base_objective_koth =
			"CREATE TABLE IF NOT EXISTS map_tg_base_objective_koth ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  map_name TEXT NOT NULL,"
			"  map_object_id INTEGER,"
			"  class_name TEXT NOT NULL,"
			"  tag TEXT,"
			"  \"group\" TEXT,"
			"  location_x REAL, location_y REAL, location_z REAL,"
			"  rotation_pitch INTEGER, rotation_yaw INTEGER, rotation_roll INTEGER,"
			"  c_f_prev_client_capt_time REAL"
			");"
			"CREATE INDEX IF NOT EXISTS idx_map_tg_base_objective_koth_map_object_id "
			"  ON map_tg_base_objective_koth(map_object_id);";
		result = sqlite3_exec(db, kV33_base_objective_koth, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v33 (base_objective_koth): %s\n", err); return; }

		const char* kV33_mission_objective_escort =
			"CREATE TABLE IF NOT EXISTS map_tg_mission_objective_escort ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  map_name TEXT NOT NULL,"
			"  map_object_id INTEGER,"
			"  class_name TEXT NOT NULL,"
			"  tag TEXT,"
			"  \"group\" TEXT,"
			"  location_x REAL, location_y REAL, location_z REAL,"
			"  rotation_pitch INTEGER, rotation_yaw INTEGER, rotation_roll INTEGER"
			");"
			"CREATE INDEX IF NOT EXISTS idx_map_tg_mission_objective_escort_map_object_id "
			"  ON map_tg_mission_objective_escort(map_object_id);";
		result = sqlite3_exec(db, kV33_mission_objective_escort, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v33 (mission_objective_escort): %s\n", err); return; }

		const char* kV33_omega_volume =
			"CREATE TABLE IF NOT EXISTS map_tg_omega_volume ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  map_name TEXT NOT NULL,"
			"  map_object_id INTEGER,"
			"  class_name TEXT NOT NULL,"
			"  tag TEXT,"
			"  \"group\" TEXT,"
			"  location_x REAL, location_y REAL, location_z REAL,"
			"  rotation_pitch INTEGER, rotation_yaw INTEGER, rotation_roll INTEGER,"
			"  m_n_omega_alert_id INTEGER,"
			"  m_n_subzone_name_msg_id INTEGER,"
			"  m_n_subzone_secondary_name_msg_id INTEGER,"
			"  m_n_omega_priority INTEGER,"
			"  m_n_bilboard_key INTEGER,"
			"  m_b_enable_equip INTEGER,"
			"  m_b_enable_skills INTEGER,"
			"  m_b_enable_crafting INTEGER,"
			"  m_b_auto_kick_if_idle INTEGER,"
			"  m_e_visual_cue INTEGER"
			");"
			"CREATE INDEX IF NOT EXISTS idx_map_tg_omega_volume_map_object_id "
			"  ON map_tg_omega_volume(map_object_id);";
		result = sqlite3_exec(db, kV33_omega_volume, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v33 (omega_volume): %s\n", err); return; }

		const char* kV33_modify_pawn_properties_volume =
			"CREATE TABLE IF NOT EXISTS map_tg_modify_pawn_properties_volume ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  map_name TEXT NOT NULL,"
			"  map_object_id INTEGER,"
			"  class_name TEXT NOT NULL,"
			"  tag TEXT,"
			"  \"group\" TEXT,"
			"  location_x REAL, location_y REAL, location_z REAL,"
			"  rotation_pitch INTEGER, rotation_yaw INTEGER, rotation_roll INTEGER,"
			"  m_b_disable_jump INTEGER,"
			"  m_b_disable_block_actors INTEGER,"
			"  m_b_disable_hanging INTEGER,"
			"  m_b_disable_all_devices INTEGER,"
			"  m_b_trigger_use_event INTEGER,"
			"  m_b_one_way_movement INTEGER,"
			"  m_v_onew_way_pitch INTEGER,"
			"  m_v_onew_way_yaw INTEGER,"
			"  m_v_onew_way_roll INTEGER,"
			"  s_n_loot_table_id INTEGER"
			");"
			"CREATE INDEX IF NOT EXISTS idx_map_tg_modify_pawn_properties_volume_map_object_id "
			"  ON map_tg_modify_pawn_properties_volume(map_object_id);";
		result = sqlite3_exec(db, kV33_modify_pawn_properties_volume, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v33 (modify_pawn_properties_volume): %s\n", err); return; }

		const char* kV33_device_volume =
			"CREATE TABLE IF NOT EXISTS map_tg_device_volume ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  map_name TEXT NOT NULL,"
			"  map_object_id INTEGER,"
			"  class_name TEXT NOT NULL,"
			"  tag TEXT,"
			"  \"group\" TEXT,"
			"  location_x REAL, location_y REAL, location_z REAL,"
			"  rotation_pitch INTEGER, rotation_yaw INTEGER, rotation_roll INTEGER,"
			"  b_pain_causing INTEGER,"
			"  s_b_device_active INTEGER,"
			"  s_n_device_id INTEGER,"
			"  s_n_team_number INTEGER,"
			"  s_n_task_force INTEGER"
			");"
			"CREATE INDEX IF NOT EXISTS idx_map_tg_device_volume_map_object_id "
			"  ON map_tg_device_volume(map_object_id);";
		result = sqlite3_exec(db, kV33_device_volume, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v33 (device_volume): %s\n", err); return; }

		const char* kV33_flag_capture_volume =
			"CREATE TABLE IF NOT EXISTS map_tg_flag_capture_volume ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  map_name TEXT NOT NULL,"
			"  map_object_id INTEGER,"
			"  class_name TEXT NOT NULL,"
			"  tag TEXT,"
			"  \"group\" TEXT,"
			"  location_x REAL, location_y REAL, location_z REAL,"
			"  rotation_pitch INTEGER, rotation_yaw INTEGER, rotation_roll INTEGER,"
			"  r_n_task_force INTEGER,"
			"  r_e_coalition INTEGER"
			");"
			"CREATE INDEX IF NOT EXISTS idx_map_tg_flag_capture_volume_map_object_id "
			"  ON map_tg_flag_capture_volume(map_object_id);";
		result = sqlite3_exec(db, kV33_flag_capture_volume, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v33 (flag_capture_volume): %s\n", err); return; }

		const char* kV33_help_alert_volume =
			"CREATE TABLE IF NOT EXISTS map_tg_help_alert_volume ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  map_name TEXT NOT NULL,"
			"  map_object_id INTEGER,"
			"  class_name TEXT NOT NULL,"
			"  tag TEXT,"
			"  \"group\" TEXT,"
			"  location_x REAL, location_y REAL, location_z REAL,"
			"  rotation_pitch INTEGER, rotation_yaw INTEGER, rotation_roll INTEGER,"
			"  m_n_help_id INTEGER"
			");"
			"CREATE INDEX IF NOT EXISTS idx_map_tg_help_alert_volume_map_object_id "
			"  ON map_tg_help_alert_volume(map_object_id);";
		result = sqlite3_exec(db, kV33_help_alert_volume, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v33 (help_alert_volume): %s\n", err); return; }

		const char* kV33_mission_list_volume =
			"CREATE TABLE IF NOT EXISTS map_tg_mission_list_volume ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  map_name TEXT NOT NULL,"
			"  map_object_id INTEGER,"
			"  class_name TEXT NOT NULL,"
			"  tag TEXT,"
			"  \"group\" TEXT,"
			"  location_x REAL, location_y REAL, location_z REAL,"
			"  rotation_pitch INTEGER, rotation_yaw INTEGER, rotation_roll INTEGER,"
			"  s_n_queue_table_id INTEGER,"
			"  s_n_queue_table_msg_id INTEGER"
			");"
			"CREATE INDEX IF NOT EXISTS idx_map_tg_mission_list_volume_map_object_id "
			"  ON map_tg_mission_list_volume(map_object_id);";
		result = sqlite3_exec(db, kV33_mission_list_volume, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v33 (mission_list_volume): %s\n", err); return; }

		const char* kV33_actor_factory =
			"CREATE TABLE IF NOT EXISTS map_tg_actor_factory ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  map_name TEXT NOT NULL,"
			"  map_object_id INTEGER,"
			"  class_name TEXT NOT NULL,"
			"  tag TEXT,"
			"  \"group\" TEXT,"
			"  location_x REAL, location_y REAL, location_z REAL,"
			"  rotation_pitch INTEGER, rotation_yaw INTEGER, rotation_roll INTEGER,"
			"  s_b_auto_spawn INTEGER,"
			"  s_n_team_number INTEGER,"
			"  s_n_task_force INTEGER,"
			"  s_e_coalition INTEGER,"
			"  s_e_selection_method INTEGER,"
			"  s_n_selection_list_id INTEGER,"
			"  s_n_selected_object_id INTEGER,"
			"  m_n_selection_list_prop_id INTEGER,"
			"  s_n_name_id INTEGER,"
			"  s_n_cur_list_index INTEGER"
			");"
			"CREATE INDEX IF NOT EXISTS idx_map_tg_actor_factory_map_object_id "
			"  ON map_tg_actor_factory(map_object_id);";
		result = sqlite3_exec(db, kV33_actor_factory, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v33 (actor_factory): %s\n", err); return; }

		const char* kV33_bot_factory =
			"CREATE TABLE IF NOT EXISTS map_tg_bot_factory ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  map_name TEXT NOT NULL,"
			"  map_object_id INTEGER,"
			"  class_name TEXT NOT NULL,"
			"  tag TEXT,"
			"  \"group\" TEXT,"
			"  location_x REAL, location_y REAL, location_z REAL,"
			"  rotation_pitch INTEGER, rotation_yaw INTEGER, rotation_roll INTEGER,"
			"  location_selection INTEGER,"
			"  type_selection INTEGER,"
			"  s_n_cur_location_index INTEGER,"
			"  b_patrol_loop INTEGER,"
			"  b_always_patrol INTEGER,"
			"  b_spawn_on_alarm INTEGER,"
			"  b_auto_spawn INTEGER,"
			"  m_b_first_spawn INTEGER,"
			"  b_bulk_spawn INTEGER,"
			"  b_respawn INTEGER,"
			"  m_b_ignore_collision_on_spawn INTEGER,"
			"  n_global_alarm_id INTEGER,"
			"  n_bot_count INTEGER,"
			"  n_current_count INTEGER,"
			"  n_active_count INTEGER,"
			"  n_total_spawns INTEGER,"
			"  n_spawn_table_id INTEGER,"
			"  n_default_spawn_table_id INTEGER,"
			"  f_spawn_delay REAL,"
			"  m_n_last_group INTEGER,"
			"  n_priority INTEGER,"
			"  n_prev_priority INTEGER,"
			"  f_last_kill_time REAL,"
			"  f_balance REAL,"
			"  f_respawn_delay REAL,"
			"  m_n_spawn_order INTEGER"
			");"
			"CREATE INDEX IF NOT EXISTS idx_map_tg_bot_factory_map_object_id "
			"  ON map_tg_bot_factory(map_object_id);";
		result = sqlite3_exec(db, kV33_bot_factory, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v33 (bot_factory): %s\n", err); return; }

		const char* kV33_bot_factory_spawnable =
			"CREATE TABLE IF NOT EXISTS map_tg_bot_factory_spawnable ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  map_name TEXT NOT NULL,"
			"  map_object_id INTEGER,"
			"  class_name TEXT NOT NULL,"
			"  tag TEXT,"
			"  \"group\" TEXT,"
			"  location_x REAL, location_y REAL, location_z REAL,"
			"  rotation_pitch INTEGER, rotation_yaw INTEGER, rotation_roll INTEGER"
			");"
			"CREATE INDEX IF NOT EXISTS idx_map_tg_bot_factory_spawnable_map_object_id "
			"  ON map_tg_bot_factory_spawnable(map_object_id);";
		result = sqlite3_exec(db, kV33_bot_factory_spawnable, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v33 (bot_factory_spawnable): %s\n", err); return; }

		const char* kV33_beacon_factory =
			"CREATE TABLE IF NOT EXISTS map_tg_beacon_factory ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  map_name TEXT NOT NULL,"
			"  map_object_id INTEGER,"
			"  class_name TEXT NOT NULL,"
			"  tag TEXT,"
			"  \"group\" TEXT,"
			"  location_x REAL, location_y REAL, location_z REAL,"
			"  rotation_pitch INTEGER, rotation_yaw INTEGER, rotation_roll INTEGER,"
			"  m_n_priority INTEGER,"
			"  m_n_prev_priority INTEGER,"
			"  m_b_beacon_exit INTEGER,"
			"  m_b_is_fallback INTEGER"
			");"
			"CREATE INDEX IF NOT EXISTS idx_map_tg_beacon_factory_map_object_id "
			"  ON map_tg_beacon_factory(map_object_id);";
		result = sqlite3_exec(db, kV33_beacon_factory, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v33 (beacon_factory): %s\n", err); return; }

		const char* kV33_deployable_factory =
			"CREATE TABLE IF NOT EXISTS map_tg_deployable_factory ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  map_name TEXT NOT NULL,"
			"  map_object_id INTEGER,"
			"  class_name TEXT NOT NULL,"
			"  tag TEXT,"
			"  \"group\" TEXT,"
			"  location_x REAL, location_y REAL, location_z REAL,"
			"  rotation_pitch INTEGER, rotation_yaw INTEGER, rotation_roll INTEGER,"
			"  n_current_count INTEGER,"
			"  s_f_last_spawn_time REAL,"
			"  s_b_spawn_once INTEGER"
			");"
			"CREATE INDEX IF NOT EXISTS idx_map_tg_deployable_factory_map_object_id "
			"  ON map_tg_deployable_factory(map_object_id);";
		result = sqlite3_exec(db, kV33_deployable_factory, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v33 (deployable_factory): %s\n", err); return; }

		const char* kV33_destructible_factory =
			"CREATE TABLE IF NOT EXISTS map_tg_destructible_factory ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  map_name TEXT NOT NULL,"
			"  map_object_id INTEGER,"
			"  class_name TEXT NOT NULL,"
			"  tag TEXT,"
			"  \"group\" TEXT,"
			"  location_x REAL, location_y REAL, location_z REAL,"
			"  rotation_pitch INTEGER, rotation_yaw INTEGER, rotation_roll INTEGER"
			");"
			"CREATE INDEX IF NOT EXISTS idx_map_tg_destructible_factory_map_object_id "
			"  ON map_tg_destructible_factory(map_object_id);";
		result = sqlite3_exec(db, kV33_destructible_factory, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v33 (destructible_factory): %s\n", err); return; }

		const char* kV33_hex_item_factory =
			"CREATE TABLE IF NOT EXISTS map_tg_hex_item_factory ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  map_name TEXT NOT NULL,"
			"  map_object_id INTEGER,"
			"  class_name TEXT NOT NULL,"
			"  tag TEXT,"
			"  \"group\" TEXT,"
			"  location_x REAL, location_y REAL, location_z REAL,"
			"  rotation_pitch INTEGER, rotation_yaw INTEGER, rotation_roll INTEGER,"
			"  s_b_needs_spawn INTEGER"
			");"
			"CREATE INDEX IF NOT EXISTS idx_map_tg_hex_item_factory_map_object_id "
			"  ON map_tg_hex_item_factory(map_object_id);";
		result = sqlite3_exec(db, kV33_hex_item_factory, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v33 (hex_item_factory): %s\n", err); return; }

		const char* kV33_navigation_point =
			"CREATE TABLE IF NOT EXISTS map_tg_navigation_point ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  map_name TEXT NOT NULL,"
			"  map_object_id INTEGER,"
			"  class_name TEXT NOT NULL,"
			"  tag TEXT,"
			"  \"group\" TEXT,"
			"  location_x REAL, location_y REAL, location_z REAL,"
			"  rotation_pitch INTEGER, rotation_yaw INTEGER, rotation_roll INTEGER"
			");"
			"CREATE INDEX IF NOT EXISTS idx_map_tg_navigation_point_map_object_id "
			"  ON map_tg_navigation_point(map_object_id);";
		result = sqlite3_exec(db, kV33_navigation_point, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v33 (navigation_point): %s\n", err); return; }

		const char* kV33_bot_start =
			"CREATE TABLE IF NOT EXISTS map_tg_bot_start ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  map_name TEXT NOT NULL,"
			"  map_object_id INTEGER,"
			"  class_name TEXT NOT NULL,"
			"  tag TEXT,"
			"  \"group\" TEXT,"
			"  location_x REAL, location_y REAL, location_z REAL,"
			"  rotation_pitch INTEGER, rotation_yaw INTEGER, rotation_roll INTEGER"
			");"
			"CREATE INDEX IF NOT EXISTS idx_map_tg_bot_start_map_object_id "
			"  ON map_tg_bot_start(map_object_id);";
		result = sqlite3_exec(db, kV33_bot_start, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v33 (bot_start): %s\n", err); return; }

		const char* kV33_action_point =
			"CREATE TABLE IF NOT EXISTS map_tg_action_point ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  map_name TEXT NOT NULL,"
			"  map_object_id INTEGER,"
			"  class_name TEXT NOT NULL,"
			"  tag TEXT,"
			"  \"group\" TEXT,"
			"  location_x REAL, location_y REAL, location_z REAL,"
			"  rotation_pitch INTEGER, rotation_yaw INTEGER, rotation_roll INTEGER,"
			"  action_type INTEGER,"
			"  n_objective_num INTEGER,"
			"  n_task_force INTEGER,"
			"  b_use_rotation INTEGER"
			");"
			"CREATE INDEX IF NOT EXISTS idx_map_tg_action_point_map_object_id "
			"  ON map_tg_action_point(map_object_id);";
		result = sqlite3_exec(db, kV33_action_point, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v33 (action_point): %s\n", err); return; }

		const char* kV33_alarm_point =
			"CREATE TABLE IF NOT EXISTS map_tg_alarm_point ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  map_name TEXT NOT NULL,"
			"  map_object_id INTEGER,"
			"  class_name TEXT NOT NULL,"
			"  tag TEXT,"
			"  \"group\" TEXT,"
			"  location_x REAL, location_y REAL, location_z REAL,"
			"  rotation_pitch INTEGER, rotation_yaw INTEGER, rotation_roll INTEGER"
			");"
			"CREATE INDEX IF NOT EXISTS idx_map_tg_alarm_point_map_object_id "
			"  ON map_tg_alarm_point(map_object_id);";
		result = sqlite3_exec(db, kV33_alarm_point, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v33 (alarm_point): %s\n", err); return; }

		const char* kV33_cover_point =
			"CREATE TABLE IF NOT EXISTS map_tg_cover_point ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  map_name TEXT NOT NULL,"
			"  map_object_id INTEGER,"
			"  class_name TEXT NOT NULL,"
			"  tag TEXT,"
			"  \"group\" TEXT,"
			"  location_x REAL, location_y REAL, location_z REAL,"
			"  rotation_pitch INTEGER, rotation_yaw INTEGER, rotation_roll INTEGER,"
			"  m_b_lean_left INTEGER,"
			"  m_b_lean_right INTEGER,"
			"  m_b_allow_popup INTEGER,"
			"  m_b_allow_mantle INTEGER,"
			"  m_v_lean_left_x REAL, m_v_lean_left_y REAL, m_v_lean_left_z REAL,"
			"  m_v_lean_right_x REAL, m_v_lean_right_y REAL, m_v_lean_right_z REAL,"
			"  m_v_pop_up_x REAL, m_v_pop_up_y REAL, m_v_pop_up_z REAL"
			");"
			"CREATE INDEX IF NOT EXISTS idx_map_tg_cover_point_map_object_id "
			"  ON map_tg_cover_point(map_object_id);";
		result = sqlite3_exec(db, kV33_cover_point, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v33 (cover_point): %s\n", err); return; }

		const char* kV33_defense_point =
			"CREATE TABLE IF NOT EXISTS map_tg_defense_point ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  map_name TEXT NOT NULL,"
			"  map_object_id INTEGER,"
			"  class_name TEXT NOT NULL,"
			"  tag TEXT,"
			"  \"group\" TEXT,"
			"  location_x REAL, location_y REAL, location_z REAL,"
			"  rotation_pitch INTEGER, rotation_yaw INTEGER, rotation_roll INTEGER,"
			"  b_first_script INTEGER,"
			"  b_sniping INTEGER,"
			"  b_dont_change_position INTEGER,"
			"  b_avoid INTEGER,"
			"  b_disabled INTEGER,"
			"  b_not_in_vehicle INTEGER,"
			"  priority INTEGER,"
			"  num_checked REAL"
			");"
			"CREATE INDEX IF NOT EXISTS idx_map_tg_defense_point_map_object_id "
			"  ON map_tg_defense_point(map_object_id);";
		result = sqlite3_exec(db, kV33_defense_point, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v33 (defense_point): %s\n", err); return; }

		const char* kV33_hold_spot =
			"CREATE TABLE IF NOT EXISTS map_tg_hold_spot ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  map_name TEXT NOT NULL,"
			"  map_object_id INTEGER,"
			"  class_name TEXT NOT NULL,"
			"  tag TEXT,"
			"  \"group\" TEXT,"
			"  location_x REAL, location_y REAL, location_z REAL,"
			"  rotation_pitch INTEGER, rotation_yaw INTEGER, rotation_roll INTEGER"
			");"
			"CREATE INDEX IF NOT EXISTS idx_map_tg_hold_spot_map_object_id "
			"  ON map_tg_hold_spot(map_object_id);";
		result = sqlite3_exec(db, kV33_hold_spot, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v33 (hold_spot): %s\n", err); return; }

		const char* kV33_navigation_point_spawnable =
			"CREATE TABLE IF NOT EXISTS map_tg_navigation_point_spawnable ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  map_name TEXT NOT NULL,"
			"  map_object_id INTEGER,"
			"  class_name TEXT NOT NULL,"
			"  tag TEXT,"
			"  \"group\" TEXT,"
			"  location_x REAL, location_y REAL, location_z REAL,"
			"  rotation_pitch INTEGER, rotation_yaw INTEGER, rotation_roll INTEGER"
			");"
			"CREATE INDEX IF NOT EXISTS idx_map_tg_navigation_point_spawnable_map_object_id "
			"  ON map_tg_navigation_point_spawnable(map_object_id);";
		result = sqlite3_exec(db, kV33_navigation_point_spawnable, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v33 (navigation_point_spawnable): %s\n", err); return; }

		const char* kV33_point_of_interest =
			"CREATE TABLE IF NOT EXISTS map_tg_point_of_interest ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  map_name TEXT NOT NULL,"
			"  map_object_id INTEGER,"
			"  class_name TEXT NOT NULL,"
			"  tag TEXT,"
			"  \"group\" TEXT,"
			"  location_x REAL, location_y REAL, location_z REAL,"
			"  rotation_pitch INTEGER, rotation_yaw INTEGER, rotation_roll INTEGER,"
			"  m_n_name_msg_id INTEGER,"
			"  m_s_debug_name TEXT,"
			"  m_quest_radius_uu REAL,"
			"  m_b_show_when_quest_complete INTEGER"
			");"
			"CREATE INDEX IF NOT EXISTS idx_map_tg_point_of_interest_map_object_id "
			"  ON map_tg_point_of_interest(map_object_id);";
		result = sqlite3_exec(db, kV33_point_of_interest, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v33 (point_of_interest): %s\n", err); return; }

		const char* kV33_teleporter =
			"CREATE TABLE IF NOT EXISTS map_tg_teleporter ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  map_name TEXT NOT NULL,"
			"  map_object_id INTEGER,"
			"  class_name TEXT NOT NULL,"
			"  tag TEXT,"
			"  \"group\" TEXT,"
			"  location_x REAL, location_y REAL, location_z REAL,"
			"  rotation_pitch INTEGER, rotation_yaw INTEGER, rotation_roll INTEGER,"
			"  m_n_map_id INTEGER,"
			"  m_n_preload INTEGER,"
			"  m_b_set_task_force INTEGER,"
			"  m_b_balance_task_force INTEGER,"
			"  m_b_ignore_non_members INTEGER,"
			"  m_b_use_player_start INTEGER,"
			"  m_b_request_mission INTEGER,"
			"  m_n_start_group INTEGER,"
			"  m_n_task_force INTEGER"
			");"
			"CREATE INDEX IF NOT EXISTS idx_map_tg_teleporter_map_object_id "
			"  ON map_tg_teleporter(map_object_id);";
		result = sqlite3_exec(db, kV33_teleporter, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v33 (teleporter): %s\n", err); return; }

		const char* kV33_bot_factory_location_list =
			"CREATE TABLE IF NOT EXISTS map_tg_bot_factory_location_list ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  map_name TEXT NOT NULL,"
			"  map_object_id INTEGER NOT NULL,"
			"  array_index INTEGER NOT NULL,"
			"  location_x REAL, location_y REAL, location_z REAL,"
			"  nav_point_tag TEXT"
			");"
			"CREATE INDEX IF NOT EXISTS idx_map_tg_bot_factory_location_list_map_object_id "
			"  ON map_tg_bot_factory_location_list(map_object_id);";
		result = sqlite3_exec(db, kV33_bot_factory_location_list, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v33 (bot_factory_location_list): %s\n", err); return; }

		const char* kV33_bot_factory_patrol_path =
			"CREATE TABLE IF NOT EXISTS map_tg_bot_factory_patrol_path ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  map_name TEXT NOT NULL,"
			"  map_object_id INTEGER NOT NULL,"
			"  array_index INTEGER NOT NULL,"
			"  location_x REAL, location_y REAL, location_z REAL,"
			"  nav_point_tag TEXT"
			");"
			"CREATE INDEX IF NOT EXISTS idx_map_tg_bot_factory_patrol_path_map_object_id "
			"  ON map_tg_bot_factory_patrol_path(map_object_id);";
		result = sqlite3_exec(db, kV33_bot_factory_patrol_path, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v33 (bot_factory_patrol_path): %s\n", err); return; }

		Logger::Log("db", "v33: created 34 map_* tables for raw map dump data\n");
	}

	if (version < 34) {
		// v34: map_object_config — single EAV table for runtime overrides on top
		// of the raw map_* tables. Each row supplies a value for one column on
		// one map_object_id. variant_group/variant_id allow random variant
		// selection at registry init: rows sharing (map_object_id, variant_group)
		// compete, weighted by `weight`; one variant_id is picked per group and
		// all of its rows applied. Static overrides leave variant_group NULL.
		const char* kV34_map_object_config =
			"CREATE TABLE IF NOT EXISTS map_object_config ("
			"  id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  map_object_id INTEGER NOT NULL,"
			"  column_name TEXT NOT NULL,"
			"  value TEXT NOT NULL,"
			"  variant_group TEXT,"
			"  variant_id TEXT,"
			"  weight REAL NOT NULL DEFAULT 1.0"
			");"
			"CREATE INDEX IF NOT EXISTS idx_map_object_config_map_object_id "
			"  ON map_object_config(map_object_id);";
		result = sqlite3_exec(db, kV34_map_object_config, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v34 (map_object_config): %s\n", err); return; }
		Logger::Log("db", "v34: created map_object_config (EAV overrides on map_* tables)\n");
	}

	if (version < 35) {
		// v35: first round of map_object_config overrides — task force / team
		// assignments for actor factories (s_n_task_force / s_n_team_number)
		// and team-player-start spawn points (m_n_task_force). Exported from
		// the map planner.
		const char* kV35_overrides =
			"INSERT INTO map_object_config (map_object_id, column_name, value, variant_group, variant_id, weight) VALUES "
			"(13723, 's_n_team_number', '1', NULL, NULL, 1),"
			"(13723, 's_n_task_force', '1', NULL, NULL, 1),"
			"(13722, 's_n_team_number', '2', NULL, NULL, 1),"
			"(13722, 's_n_task_force', '2', NULL, NULL, 1),"
			"(13721, 's_n_team_number', '1', NULL, NULL, 1),"
			"(13721, 's_n_task_force', '1', NULL, NULL, 1),"
			"(13720, 's_n_task_force', '1', NULL, NULL, 1),"
			"(13720, 's_n_team_number', '1', NULL, NULL, 1),"
			"(7947, 's_n_team_number', '2', NULL, NULL, 1),"
			"(7947, 's_n_task_force', '2', NULL, NULL, 1),"
			"(7946, 's_n_team_number', '2', NULL, NULL, 1),"
			"(7946, 's_n_task_force', '2', NULL, NULL, 1),"
			"(7943, 's_n_team_number', '2', NULL, NULL, 1),"
			"(7943, 's_n_task_force', '2', NULL, NULL, 1),"
			"(7942, 's_n_team_number', '2', NULL, NULL, 1),"
			"(7942, 's_n_task_force', '2', NULL, NULL, 1),"
			"(7941, 's_n_team_number', '1', NULL, NULL, 1),"
			"(7941, 's_n_task_force', '1', NULL, NULL, 1),"
			"(7940, 's_n_team_number', '1', NULL, NULL, 1),"
			"(7940, 's_n_task_force', '1', NULL, NULL, 1),"
			"(7945, 's_n_team_number', '2', NULL, NULL, 1),"
			"(7945, 's_n_task_force', '2', NULL, NULL, 1),"
			"(7944, 's_n_team_number', '2', NULL, NULL, 1),"
			"(7944, 's_n_task_force', '2', NULL, NULL, 1),"
			"(7939, 's_n_team_number', '1', NULL, NULL, 1),"
			"(7939, 's_n_task_force', '1', NULL, NULL, 1),"
			"(7938, 's_n_team_number', '1', NULL, NULL, 1),"
			"(7938, 's_n_task_force', '1', NULL, NULL, 1),"
			"(7937, 's_n_team_number', '1', NULL, NULL, 1),"
			"(7937, 's_n_task_force', '1', NULL, NULL, 1),"
			"(7936, 's_n_team_number', '1', NULL, NULL, 1),"
			"(7936, 's_n_task_force', '1', NULL, NULL, 1),"
			"(13719, 'm_n_task_force', '1', NULL, NULL, 1),"
			"(7933, 'm_n_task_force', '1', NULL, NULL, 1),"
			"(7934, 'm_n_task_force', '2', NULL, NULL, 1),"
			"(7935, 'm_n_task_force', '2', NULL, NULL, 1),"
			"(7932, 'm_n_task_force', '1', NULL, NULL, 1),"
			"(13718, 'm_n_task_force', '1', NULL, NULL, 1),"
			"(7499, 'm_n_task_force', '2', NULL, NULL, 1),"
			"(7931, 'm_n_task_force', '1', NULL, NULL, 1);";
		result = sqlite3_exec(db, kV35_overrides, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v35 (map_object_config seed): %s\n", err); return; }
		Logger::Log("db", "v35: seeded 40 map_object_config rows (task force / team assignments)\n");
	}

	if (version < 36) {
		// v36: second round of map_object_config overrides — task force / team
		// assignments for actor factories and team-player-start spawn points on
		// another map (3P_Him_Arena_P / Breach). Same shape as v35.
		const char* kV36_overrides =
			"INSERT INTO map_object_config (map_object_id, column_name, value, variant_group, variant_id, weight) VALUES "
			"(13555, 'm_n_task_force',  '1', NULL, NULL, 1),"
			"(13554, 'm_n_task_force',  '2', NULL, NULL, 1),"
			"(13582, 's_n_team_number', '2', NULL, NULL, 1),"
			"(13582, 's_n_task_force',  '2', NULL, NULL, 1),"
			"(13581, 's_n_team_number', '2', NULL, NULL, 1),"
			"(13581, 's_n_task_force',  '2', NULL, NULL, 1),"
			"(13580, 's_n_team_number', '2', NULL, NULL, 1),"
			"(13580, 's_n_task_force',  '2', NULL, NULL, 1),"
			"(13579, 's_n_team_number', '2', NULL, NULL, 1),"
			"(13579, 's_n_task_force',  '2', NULL, NULL, 1),"
			"(13578, 's_n_task_force',  '2', NULL, NULL, 1),"
			"(13578, 's_n_team_number', '2', NULL, NULL, 1),"
			"(13577, 's_n_task_force',  '2', NULL, NULL, 1),"
			"(13577, 's_n_team_number', '2', NULL, NULL, 1),"
			"(13570, 's_n_task_force',  '1', NULL, NULL, 1),"
			"(13570, 's_n_team_number', '1', NULL, NULL, 1),"
			"(13569, 's_n_team_number', '1', NULL, NULL, 1),"
			"(13569, 's_n_task_force',  '1', NULL, NULL, 1),"
			"(13568, 's_n_team_number', '1', NULL, NULL, 1),"
			"(13568, 's_n_task_force',  '1', NULL, NULL, 1),"
			"(13566, 's_n_team_number', '1', NULL, NULL, 1),"
			"(13566, 's_n_task_force',  '1', NULL, NULL, 1),"
			"(13564, 's_n_team_number', '1', NULL, NULL, 1),"
			"(13564, 's_n_task_force',  '1', NULL, NULL, 1),"
			"(13565, 's_n_team_number', '1', NULL, NULL, 1),"
			"(13565, 's_n_task_force',  '1', NULL, NULL, 1);";
		result = sqlite3_exec(db, kV36_overrides, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v36 (map_object_config seed): %s\n", err); return; }
		Logger::Log("db", "v36: seeded 27 map_object_config rows (task force / team assignments)\n");
	}

	if (version < 37) {
		// v37: map_object_config overrides — m_n_priority bumps for objective/
		// player-start map objects (forces them to the top of priority-ordered
		// selection).
		const char* kV37_overrides =
			"INSERT INTO map_object_config (map_object_id, column_name, value, variant_group, variant_id, weight) VALUES "
			"(7938, 'm_n_priority', '999', NULL, NULL, 1),"
			"(7939, 'm_n_priority', '999', NULL, NULL, 1),"
			"(7932, 'm_n_priority', '999', NULL, NULL, 1),"
			"(7941,  'm_n_priority', '999', NULL, NULL, 1),"
			"(7940,  'm_n_priority', '999', NULL, NULL, 1),"
			"(7933,  'm_n_priority', '999', NULL, NULL, 1);";
		result = sqlite3_exec(db, kV37_overrides, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v37 (map_object_config seed): %s\n", err); return; }
		Logger::Log("db", "v37: seeded 6 map_object_config rows (m_n_priority bumps)\n");
	}

	if (version < 38) {
		// v38: map_object_config overrides — task force / team assignments for
		// actor factories (s_n_task_force / s_n_team_number) and team-player-
		// start spawn points (m_n_task_force) on another map. Same shape as
		// v35/v36. Exported from the map planner.
		const char* kV38_overrides =
			"INSERT INTO map_object_config (map_object_id, column_name, value, variant_group, variant_id, weight) VALUES "
			"(10683, 'm_n_task_force',  '1', NULL, NULL, 1),"
			"(10715, 's_n_team_number', '1', NULL, NULL, 1),"
			"(10715, 's_n_task_force',  '1', NULL, NULL, 1),"
			"(10714, 's_n_team_number', '1', NULL, NULL, 1),"
			"(10714, 's_n_task_force',  '1', NULL, NULL, 1),"
			"(10770, 's_n_task_force',  '1', NULL, NULL, 1),"
			"(10770, 's_n_team_number', '1', NULL, NULL, 1),"
			"(10771, 's_n_team_number', '1', NULL, NULL, 1),"
			"(10771, 's_n_task_force',  '1', NULL, NULL, 1),"
			"(10684, 'm_n_task_force',  '1', NULL, NULL, 1),"
			"(10718, 's_n_task_force',  '2', NULL, NULL, 1),"
			"(10718, 's_n_team_number', '2', NULL, NULL, 1),"
			"(10716, 's_n_team_number', '2', NULL, NULL, 1),"
			"(10716, 's_n_task_force',  '2', NULL, NULL, 1),"
			"(10687, 'm_n_task_force',  '2', NULL, NULL, 1),"
			"(10685, 'm_n_task_force',  '1', NULL, NULL, 1),"
			"(10711, 's_n_team_number', '1', NULL, NULL, 1),"
			"(10711, 's_n_task_force',  '1', NULL, NULL, 1),"
			"(10710, 's_n_task_force',  '1', NULL, NULL, 1),"
			"(10710, 's_n_team_number', '1', NULL, NULL, 1),"
			"(10708, 's_n_task_force',  '2', NULL, NULL, 1),"
			"(10708, 's_n_team_number', '2', NULL, NULL, 1),"
			"(10709, 's_n_task_force',  '2', NULL, NULL, 1),"
			"(10709, 's_n_team_number', '2', NULL, NULL, 1),"
			"(10688, 'm_n_task_force',  '2', NULL, NULL, 1),"
			"(10689, 'm_n_task_force',  '2', NULL, NULL, 1),"
			"(10707, 's_n_task_force',  '2', NULL, NULL, 1),"
			"(10707, 's_n_team_number', '2', NULL, NULL, 1),"
			"(10706, 's_n_task_force',  '2', NULL, NULL, 1),"
			"(10706, 's_n_team_number', '2', NULL, NULL, 1);";
		result = sqlite3_exec(db, kV38_overrides, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v38 (map_object_config seed): %s\n", err); return; }
		Logger::Log("db", "v38: seeded 30 map_object_config rows (task force / team assignments)\n");
	}

	if (version < 39) {
		// v39: map_object_config overrides — task force / team assignments for
		// actor factories (s_n_task_force / s_n_team_number) and team-player-
		// start spawn points (m_n_task_force) on another map. Same shape as
		// v35/v36/v38. Exported from the map planner.
		const char* kV39_overrides =
			"INSERT INTO map_object_config (map_object_id, column_name, value, variant_group, variant_id, weight) VALUES "
			"(11311, 's_n_team_number', '1', NULL, NULL, 1),"
			"(11311, 's_n_task_force',  '1', NULL, NULL, 1),"
			"(11310, 's_n_team_number', '1', NULL, NULL, 1),"
			"(11310, 's_n_task_force',  '1', NULL, NULL, 1),"
			"(11289, 'm_n_task_force',  '1', NULL, NULL, 1),"
			"(11318, 'm_n_task_force',  '2', NULL, NULL, 1),"
			"(11317, 's_n_task_force',  '2', NULL, NULL, 1),"
			"(11317, 's_n_team_number', '2', NULL, NULL, 1),"
			"(11316, 's_n_task_force',  '2', NULL, NULL, 1),"
			"(11316, 's_n_team_number', '2', NULL, NULL, 1);";
		result = sqlite3_exec(db, kV39_overrides, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v39 (map_object_config seed): %s\n", err); return; }
		Logger::Log("db", "v39: seeded 10 map_object_config rows (task force / team assignments)\n");
	}

	if (version < 40) {
		// v40: map_object_config overrides — task force / team assignments for
		// another map (player-start m_n_task_force + actor-factory s_n_task_force /
		// s_n_team_number). Same shape as v35/v36/v38/v39. Exported from the
		// map planner.
		const char* kV40_overrides =
			"INSERT INTO map_object_config (map_object_id, column_name, value, variant_group, variant_id, weight) VALUES "
			"(11032, 'm_n_task_force',  '1', NULL, NULL, 1),"
			"(13223, 's_n_team_number', '1', NULL, NULL, 1),"
			"(13223, 's_n_task_force',  '1', NULL, NULL, 1),"
			"(13224, 's_n_team_number', '1', NULL, NULL, 1),"
			"(13224, 's_n_task_force',  '1', NULL, NULL, 1),"
			"(13221, 's_n_team_number', '2', NULL, NULL, 1),"
			"(13221, 's_n_task_force',  '2', NULL, NULL, 1),"
			"(13222, 's_n_task_force',  '2', NULL, NULL, 1),"
			"(13222, 's_n_team_number', '2', NULL, NULL, 1),"
			"(11036, 'm_n_task_force',  '2', NULL, NULL, 1);";
		result = sqlite3_exec(db, kV40_overrides, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v40 (map_object_config seed): %s\n", err); return; }
		Logger::Log("db", "v40: seeded 10 map_object_config rows (task force / team assignments)\n");
	}

	if (version < 41) {
		// v41: map_object_config overrides — task force / team assignments for
		// yet another map (player-start m_n_task_force + actor-factory
		// s_n_task_force / s_n_team_number). Same shape as v40. Exported from
		// the map planner.
		const char* kV41_overrides =
			"INSERT INTO map_object_config (map_object_id, column_name, value, variant_group, variant_id, weight) VALUES "
			"(13137, 'm_n_task_force',  '2', NULL, NULL, 1),"
			"(13138, 's_n_task_force',  '2', NULL, NULL, 1),"
			"(13138, 's_n_team_number', '2', NULL, NULL, 1),"
			"(13139, 's_n_task_force',  '2', NULL, NULL, 1),"
			"(13139, 's_n_team_number', '2', NULL, NULL, 1),"
			"(13134, 's_n_team_number', '1', NULL, NULL, 1),"
			"(13134, 's_n_task_force',  '1', NULL, NULL, 1),"
			"(13135, 's_n_team_number', '1', NULL, NULL, 1),"
			"(13135, 's_n_task_force',  '1', NULL, NULL, 1),"
			"(13075, 'm_n_task_force',  '1', NULL, NULL, 1);";
		result = sqlite3_exec(db, kV41_overrides, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v41 (map_object_config seed): %s\n", err); return; }
		Logger::Log("db", "v41: seeded 10 map_object_config rows (task force / team assignments)\n");
	}

	if (version < 42) {
		// v42: map_object_config overrides — task force / team assignments for
		// another map with three attacker spawn clusters and three defender
		// clusters (player-start m_n_task_force + actor-factory s_n_task_force /
		// s_n_team_number). Same shape as v40/v41. Exported from the map planner.
		const char* kV42_overrides =
			"INSERT INTO map_object_config (map_object_id, column_name, value, variant_group, variant_id, weight) VALUES "
			"(11548, 's_n_task_force',  '1', NULL, NULL, 1),"
			"(11548, 's_n_team_number', '1', NULL, NULL, 1),"
			"(11549, 's_n_task_force',  '1', NULL, NULL, 1),"
			"(11549, 's_n_team_number', '1', NULL, NULL, 1),"
			"(11550, 'm_n_task_force',  '1', NULL, NULL, 1),"
			"(12295, 's_n_task_force',  '1', NULL, NULL, 1),"
			"(12295, 's_n_team_number', '1', NULL, NULL, 1),"
			"(12296, 's_n_task_force',  '1', NULL, NULL, 1),"
			"(12296, 's_n_team_number', '1', NULL, NULL, 1),"
			"(12294, 'm_n_task_force',  '1', NULL, NULL, 1),"
			"(12299, 's_n_task_force',  '1', NULL, NULL, 1),"
			"(12299, 's_n_team_number', '1', NULL, NULL, 1),"
			"(12298, 's_n_task_force',  '1', NULL, NULL, 1),"
			"(12298, 's_n_team_number', '1', NULL, NULL, 1),"
			"(12297, 'm_n_task_force',  '1', NULL, NULL, 1),"
			"(12302, 's_n_task_force',  '2', NULL, NULL, 1),"
			"(12302, 's_n_team_number', '2', NULL, NULL, 1),"
			"(12301, 's_n_task_force',  '2', NULL, NULL, 1),"
			"(12301, 's_n_team_number', '2', NULL, NULL, 1),"
			"(12305, 's_n_task_force',  '2', NULL, NULL, 1),"
			"(12305, 's_n_team_number', '2', NULL, NULL, 1),"
			"(12304, 's_n_team_number', '2', NULL, NULL, 1),"
			"(12304, 's_n_task_force',  '2', NULL, NULL, 1),"
			"(11552, 's_n_task_force',  '2', NULL, NULL, 1),"
			"(11552, 's_n_team_number', '2', NULL, NULL, 1),"
			"(11553, 's_n_task_force',  '2', NULL, NULL, 1),"
			"(11553, 's_n_team_number', '2', NULL, NULL, 1),"
			"(11551, 'm_n_task_force',  '2', NULL, NULL, 1),"
			"(12303, 'm_n_task_force',  '2', NULL, NULL, 1),"
			"(12300, 'm_n_task_force',  '2', NULL, NULL, 1);";
		result = sqlite3_exec(db, kV42_overrides, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v42 (map_object_config seed): %s\n", err); return; }
		Logger::Log("db", "v42: seeded 30 map_object_config rows (task force / team assignments)\n");
	}

	if (version < 43) {
		// v43: map_object_config overrides — task force / team assignments for
		// another map (player-start m_n_task_force + actor-factory s_n_task_force /
		// s_n_team_number). Same shape as v40/v41/v42. Exported from the map planner.
		const char* kV43_overrides =
			"INSERT INTO map_object_config (map_object_id, column_name, value, variant_group, variant_id, weight) VALUES "
			"(8589, 'm_n_task_force',  '1', NULL, NULL, 1),"
			"(8598, 'm_n_task_force',  '2', NULL, NULL, 1),"
			"(8590, 's_n_task_force',  '1', NULL, NULL, 1),"
			"(8590, 's_n_team_number', '1', NULL, NULL, 1),"
			"(8600, 's_n_task_force',  '2', NULL, NULL, 1),"
			"(8600, 's_n_team_number', '2', NULL, NULL, 1);";
		result = sqlite3_exec(db, kV43_overrides, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v43 (map_object_config seed): %s\n", err); return; }
		Logger::Log("db", "v43: seeded 6 map_object_config rows (task force / team assignments)\n");
	}

	if (version < 44) {
		// v44: map_object_config overrides — task force / team assignments for
		// another map (player-start m_n_task_force + actor-factory s_n_task_force /
		// s_n_team_number). Same shape as v43. Exported from the map planner.
		const char* kV44_overrides =
			"INSERT INTO map_object_config (map_object_id, column_name, value, variant_group, variant_id, weight) VALUES "
			"(8589, 'm_n_task_force',  '1', NULL, NULL, 1),"
			"(8591, 's_n_task_force',  '1', NULL, NULL, 1),"
			"(8591, 's_n_team_number', '1', NULL, NULL, 1),"
			"(8590, 's_n_task_force',  '1', NULL, NULL, 1),"
			"(8590, 's_n_team_number', '1', NULL, NULL, 1),"
			"(8600, 's_n_team_number', '2', NULL, NULL, 1),"
			"(8600, 's_n_task_force',  '2', NULL, NULL, 1),"
			"(8599, 's_n_team_number', '2', NULL, NULL, 1),"
			"(8599, 's_n_task_force',  '2', NULL, NULL, 1),"
			"(8598, 'm_n_task_force',  '2', NULL, NULL, 1);";
		result = sqlite3_exec(db, kV44_overrides, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v44 (map_object_config seed): %s\n", err); return; }
		Logger::Log("db", "v44: seeded 10 map_object_config rows (task force / team assignments)\n");
	}

	if (version < 45) {
		// v45: map_object_config overrides — task force / team assignments for
		// another map with multiple defender spawn clusters (player-start
		// m_n_task_force + actor-factory s_n_task_force / s_n_team_number).
		// Same shape as v43/v44. Exported from the map planner.
		const char* kV45_overrides =
			"INSERT INTO map_object_config (map_object_id, column_name, value, variant_group, variant_id, weight) VALUES "
			"(11955, 'm_n_task_force',  '1', NULL, NULL, 1),"
			"(11956, 'm_n_task_force',  '2', NULL, NULL, 1),"
			"(12111, 'm_n_task_force',  '2', NULL, NULL, 1),"
			"(12112, 'm_n_task_force',  '2', NULL, NULL, 1),"
			"(11954, 's_n_task_force',  '1', NULL, NULL, 1),"
			"(11954, 's_n_team_number', '1', NULL, NULL, 1),"
			"(11953, 's_n_task_force',  '1', NULL, NULL, 1),"
			"(11953, 's_n_team_number', '1', NULL, NULL, 1),"
			"(11957, 's_n_team_number', '2', NULL, NULL, 1),"
			"(11957, 's_n_task_force',  '2', NULL, NULL, 1),"
			"(11958, 's_n_task_force',  '2', NULL, NULL, 1),"
			"(11958, 's_n_team_number', '2', NULL, NULL, 1),"
			"(11962, 's_n_task_force',  '2', NULL, NULL, 1),"
			"(11962, 's_n_team_number', '2', NULL, NULL, 1),"
			"(11963, 's_n_task_force',  '2', NULL, NULL, 1),"
			"(11963, 's_n_team_number', '2', NULL, NULL, 1),"
			"(11964, 's_n_task_force',  '2', NULL, NULL, 1),"
			"(11964, 's_n_team_number', '2', NULL, NULL, 1),"
			"(11965, 's_n_task_force',  '2', NULL, NULL, 1),"
			"(11965, 's_n_team_number', '2', NULL, NULL, 1);";
		result = sqlite3_exec(db, kV45_overrides, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v45 (map_object_config seed): %s\n", err); return; }
		Logger::Log("db", "v45: seeded 20 map_object_config rows (task force / team assignments)\n");
	}

	if (version < 46) {
		// v46: scope map_object_config to a specific map. map_object_id turns
		// out NOT to be unique across maps, so the EAV table needs a map_name
		// discriminator. Column is nullable — a NULL row applies to every map
		// (legacy / global). MapObjectConfig::Init prefers rows where
		// (map_name, map_object_id) match the current map and only falls back
		// to NULL-map rows for the same map_object_id when no map-specific
		// row exists. ALTER appends the column to the end; that's fine, the
		// in-memory registry doesn't care about column order.
		if (!TableColumnExists(db, "map_object_config", "map_name")) {
			result = sqlite3_exec(db, "ALTER TABLE map_object_config ADD COLUMN map_name TEXT;", nullptr, nullptr, &err);
			if (result != SQLITE_OK) { Logger::Log("db", "Failed v46 (add map_name column): %s\n", err); return; }
			Logger::Log("db", "v46: added map_name column to map_object_config (nullable, scoped lookups)\n");
		} else {
			Logger::Log("db", "v46: map_name column already present on map_object_config\n");
		}
		result = sqlite3_exec(db,
			"CREATE INDEX IF NOT EXISTS idx_map_object_config_map_name "
			"ON map_object_config(map_name, map_object_id);",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v46 (map_name index): %s\n", err); return; }
	}

	if (version < 47) {
		// v47: first wave of MAP-SCOPED overrides — Ice_GorgeA01_v2 (30 rows)
		// and 3P_VolcanoAssault_P (30 rows). The planner export for
		// 3P_VolcanoAssault_P originally included an `UPDATE … WHERE id = 163`
		// targeting (11551, 'm_n_task_force') = '2' from v42 (intended for a
		// different map); now expressed as a proper map-scoped INSERT so the
		// global row stays put and v46 precedence shadows it on
		// 3P_VolcanoAssault_P only.
		const char* kV47_ice_gorge =
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES "
			"('Ice_GorgeA01_v2', 7438, 'm_n_task_force',  '1', NULL, NULL, 1),"
			"('Ice_GorgeA01_v2', 7472, 'm_n_task_force',  '2', NULL, NULL, 1),"
			"('Ice_GorgeA01_v2', 7930, 'm_n_task_force',  '2', NULL, NULL, 1),"
			"('Ice_GorgeA01_v2', 7953, 'm_n_task_force',  '1', NULL, NULL, 1),"
			"('Ice_GorgeA01_v2', 7958, 'm_n_task_force',  '2', NULL, NULL, 1),"
			"('Ice_GorgeA01_v2', 7952, 'm_n_task_force',  '1', NULL, NULL, 1),"
			"('Ice_GorgeA01_v2', 7962, 's_n_task_force',  '2', NULL, NULL, 1),"
			"('Ice_GorgeA01_v2', 7962, 's_n_team_number', '2', NULL, NULL, 1),"
			"('Ice_GorgeA01_v2', 7961, 's_n_task_force',  '2', NULL, NULL, 1),"
			"('Ice_GorgeA01_v2', 7961, 's_n_team_number', '2', NULL, NULL, 1),"
			"('Ice_GorgeA01_v2', 7960, 's_n_task_force',  '2', NULL, NULL, 1),"
			"('Ice_GorgeA01_v2', 7960, 's_n_team_number', '2', NULL, NULL, 1),"
			"('Ice_GorgeA01_v2', 7959, 's_n_team_number', '2', NULL, NULL, 1),"
			"('Ice_GorgeA01_v2', 7959, 's_n_task_force',  '2', NULL, NULL, 1),"
			"('Ice_GorgeA01_v2', 7957, 's_n_task_force',  '1', NULL, NULL, 1),"
			"('Ice_GorgeA01_v2', 7957, 's_n_team_number', '1', NULL, NULL, 1),"
			"('Ice_GorgeA01_v2', 7956, 's_n_team_number', '1', NULL, NULL, 1),"
			"('Ice_GorgeA01_v2', 7956, 's_n_task_force',  '1', NULL, NULL, 1),"
			"('Ice_GorgeA01_v2', 7955, 's_n_task_force',  '1', NULL, NULL, 1),"
			"('Ice_GorgeA01_v2', 7955, 's_n_team_number', '1', NULL, NULL, 1),"
			"('Ice_GorgeA01_v2', 7954, 's_n_task_force',  '1', NULL, NULL, 1),"
			"('Ice_GorgeA01_v2', 7954, 's_n_team_number', '1', NULL, NULL, 1),"
			"('Ice_GorgeA01_v2', 7951, 's_n_team_number', '1', NULL, NULL, 1),"
			"('Ice_GorgeA01_v2', 7951, 's_n_task_force',  '1', NULL, NULL, 1),"
			"('Ice_GorgeA01_v2', 7950, 's_n_team_number', '1', NULL, NULL, 1),"
			"('Ice_GorgeA01_v2', 7950, 's_n_task_force',  '1', NULL, NULL, 1),"
			"('Ice_GorgeA01_v2', 7949, 's_n_team_number', '2', NULL, NULL, 1),"
			"('Ice_GorgeA01_v2', 7949, 's_n_task_force',  '2', NULL, NULL, 1),"
			"('Ice_GorgeA01_v2', 7948, 's_n_team_number', '2', NULL, NULL, 1),"
			"('Ice_GorgeA01_v2', 7948, 's_n_task_force',  '2', NULL, NULL, 1);";
		result = sqlite3_exec(db, kV47_ice_gorge, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v47 (Ice_GorgeA01_v2 seed): %s\n", err); return; }

		const char* kV47_volcano =
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES "
			"('3P_VolcanoAssault_P', 13627, 's_n_task_force',  '2', NULL, NULL, 1),"
			"('3P_VolcanoAssault_P', 13627, 's_n_team_number', '2', NULL, NULL, 1),"
			"('3P_VolcanoAssault_P', 13600, 's_n_team_number', '2', NULL, NULL, 1),"
			"('3P_VolcanoAssault_P', 13600, 's_n_task_force',  '2', NULL, NULL, 1),"
			"('3P_VolcanoAssault_P', 13594, 's_n_team_number', '2', NULL, NULL, 1),"
			"('3P_VolcanoAssault_P', 13594, 's_n_task_force',  '2', NULL, NULL, 1),"
			"('3P_VolcanoAssault_P', 13591, 's_n_team_number', '2', NULL, NULL, 1),"
			"('3P_VolcanoAssault_P', 13591, 's_n_task_force',  '2', NULL, NULL, 1),"
			"('3P_VolcanoAssault_P', 13590, 's_n_task_force',  '2', NULL, NULL, 1),"
			"('3P_VolcanoAssault_P', 13590, 's_n_team_number', '2', NULL, NULL, 1),"
			"('3P_VolcanoAssault_P', 13588, 's_n_team_number', '2', NULL, NULL, 1),"
			"('3P_VolcanoAssault_P', 13588, 's_n_task_force',  '2', NULL, NULL, 1),"
			"('3P_VolcanoAssault_P', 13587, 's_n_task_force',  '1', NULL, NULL, 1),"
			"('3P_VolcanoAssault_P', 13587, 's_n_team_number', '1', NULL, NULL, 1),"
			"('3P_VolcanoAssault_P', 13586, 's_n_task_force',  '1', NULL, NULL, 1),"
			"('3P_VolcanoAssault_P', 13586, 's_n_team_number', '1', NULL, NULL, 1),"
			"('3P_VolcanoAssault_P', 13584, 's_n_task_force',  '1', NULL, NULL, 1),"
			"('3P_VolcanoAssault_P', 13584, 's_n_team_number', '1', NULL, NULL, 1),"
			"('3P_VolcanoAssault_P', 13585, 's_n_task_force',  '1', NULL, NULL, 1),"
			"('3P_VolcanoAssault_P', 13585, 's_n_team_number', '1', NULL, NULL, 1),"
			"('3P_VolcanoAssault_P', 13558, 's_n_task_force',  '1', NULL, NULL, 1),"
			"('3P_VolcanoAssault_P', 13558, 's_n_team_number', '1', NULL, NULL, 1),"
			"('3P_VolcanoAssault_P', 13557, 's_n_task_force',  '1', NULL, NULL, 1),"
			"('3P_VolcanoAssault_P', 13557, 's_n_team_number', '1', NULL, NULL, 1),"
			"('3P_VolcanoAssault_P', 13599, 'm_n_task_force',  '2', NULL, NULL, 1),"
			"('3P_VolcanoAssault_P', 13598, 'm_n_task_force',  '2', NULL, NULL, 1),"
			"('3P_VolcanoAssault_P', 13612, 'm_n_task_force',  '2', NULL, NULL, 1),"
			"('3P_VolcanoAssault_P', 13596, 'm_n_task_force',  '1', NULL, NULL, 1),"
			"('3P_VolcanoAssault_P', 13595, 'm_n_task_force',  '1', NULL, NULL, 1),"
			// Originally `UPDATE ... WHERE id = 163` (= the v42 global row for
			// (11551, m_n_task_force)='2'). Re-expressed as a map-scoped INSERT
			// so the v42 row keeps applying to its own map and v46 precedence
			// shadows it with '1' on 3P_VolcanoAssault_P only.
			"('3P_VolcanoAssault_P', 11551, 'm_n_task_force',  '1', NULL, NULL, 1);";
		result = sqlite3_exec(db, kV47_volcano, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v47 (3P_VolcanoAssault_P seed): %s\n", err); return; }

		Logger::Log("db", "v47: seeded 30 map_object_config rows for Ice_GorgeA01_v2 + 30 for 3P_VolcanoAssault_P\n");
	}

	if (version < 48) {
		// v48: 3P_Climate_Control3_P — task force / team assignments for
		// actor-factory (s_n_task_force / s_n_team_number) and team-player-
		// start (m_n_task_force) spawn points, plus one m_n_priority bump
		// on objective 11573. All map-scoped (map_name set) per v46 rules.
		// Exported from the map planner.
		const char* kV48_climate =
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES "
			"('3P_Climate_Control3_P', 11568, 's_n_team_number', '1',   NULL, NULL, 1),"
			"('3P_Climate_Control3_P', 11568, 's_n_task_force',  '1',   NULL, NULL, 1),"
			"('3P_Climate_Control3_P', 11586, 's_n_team_number', '2',   NULL, NULL, 1),"
			"('3P_Climate_Control3_P', 11586, 's_n_task_force',  '2',   NULL, NULL, 1),"
			"('3P_Climate_Control3_P', 11585, 's_n_team_number', '2',   NULL, NULL, 1),"
			"('3P_Climate_Control3_P', 11585, 's_n_task_force',  '2',   NULL, NULL, 1),"
			"('3P_Climate_Control3_P', 11582, 's_n_team_number', '2',   NULL, NULL, 1),"
			"('3P_Climate_Control3_P', 11582, 's_n_task_force',  '2',   NULL, NULL, 1),"
			"('3P_Climate_Control3_P', 11581, 's_n_team_number', '2',   NULL, NULL, 1),"
			"('3P_Climate_Control3_P', 11581, 's_n_task_force',  '2',   NULL, NULL, 1),"
			"('3P_Climate_Control3_P', 11579, 's_n_team_number', '2',   NULL, NULL, 1),"
			"('3P_Climate_Control3_P', 11579, 's_n_task_force',  '2',   NULL, NULL, 1),"
			"('3P_Climate_Control3_P', 11577, 's_n_team_number', '2',   NULL, NULL, 1),"
			"('3P_Climate_Control3_P', 11577, 's_n_task_force',  '2',   NULL, NULL, 1),"
			"('3P_Climate_Control3_P', 11574, 's_n_team_number', '1',   NULL, NULL, 1),"
			"('3P_Climate_Control3_P', 11574, 's_n_task_force',  '1',   NULL, NULL, 1),"
			"('3P_Climate_Control3_P', 11573, 'm_n_priority',    '999', NULL, NULL, 1),"
			"('3P_Climate_Control3_P', 11572, 's_n_task_force',  '1',   NULL, NULL, 1),"
			"('3P_Climate_Control3_P', 11572, 's_n_team_number', '1',   NULL, NULL, 1),"
			"('3P_Climate_Control3_P', 11569, 's_n_team_number', '1',   NULL, NULL, 1),"
			"('3P_Climate_Control3_P', 11569, 's_n_task_force',  '1',   NULL, NULL, 1),"
			"('3P_Climate_Control3_P', 11570, 's_n_task_force',  '1',   NULL, NULL, 1),"
			"('3P_Climate_Control3_P', 11570, 's_n_team_number', '1',   NULL, NULL, 1),"
			"('3P_Climate_Control3_P', 11571, 's_n_task_force',  '1',   NULL, NULL, 1),"
			"('3P_Climate_Control3_P', 11571, 's_n_team_number', '1',   NULL, NULL, 1),"
			"('3P_Climate_Control3_P', 11564, 'm_n_task_force',  '2',   NULL, NULL, 1),"
			"('3P_Climate_Control3_P', 11563, 'm_n_task_force',  '2',   NULL, NULL, 1),"
			"('3P_Climate_Control3_P', 11562, 'm_n_task_force',  '1',   NULL, NULL, 1),"
			"('3P_Climate_Control3_P', 11559, 'm_n_task_force',  '1',   NULL, NULL, 1),"
			"('3P_Climate_Control3_P', 11566, 'm_n_task_force',  '2',   NULL, NULL, 1),"
			"('3P_Climate_Control3_P', 11556, 'm_n_task_force',  '1',   NULL, NULL, 1);";
		result = sqlite3_exec(db, kV48_climate, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v48 (3P_Climate_Control3_P seed): %s\n", err); return; }
		Logger::Log("db", "v48: seeded 31 map_object_config rows for 3P_Climate_Control3_P\n");
	}

	if (version < 49) {
		// v49: Push_Dust_P — task force / team assignments for actor-factory
		// (s_n_task_force / s_n_team_number) and team-player-start
		// (m_n_task_force) spawn points. All map-scoped per v46 rules.
		// Exported from the map planner.
		const char* kV49_push_dust =
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES "
			"('Push_Dust_P', 13774, 's_n_team_number', '2', NULL, NULL, 1),"
			"('Push_Dust_P', 13774, 's_n_task_force',  '2', NULL, NULL, 1),"
			"('Push_Dust_P', 13773, 's_n_team_number', '2', NULL, NULL, 1),"
			"('Push_Dust_P', 13773, 's_n_task_force',  '2', NULL, NULL, 1),"
			"('Push_Dust_P', 13772, 's_n_team_number', '2', NULL, NULL, 1),"
			"('Push_Dust_P', 13772, 's_n_task_force',  '2', NULL, NULL, 1),"
			"('Push_Dust_P', 13771, 's_n_team_number', '2', NULL, NULL, 1),"
			"('Push_Dust_P', 13771, 's_n_task_force',  '2', NULL, NULL, 1),"
			"('Push_Dust_P', 13770, 's_n_team_number', '1', NULL, NULL, 1),"
			"('Push_Dust_P', 13770, 's_n_task_force',  '1', NULL, NULL, 1),"
			"('Push_Dust_P', 13769, 's_n_team_number', '1', NULL, NULL, 1),"
			"('Push_Dust_P', 13769, 's_n_task_force',  '1', NULL, NULL, 1),"
			"('Push_Dust_P', 13767, 's_n_team_number', '1', NULL, NULL, 1),"
			"('Push_Dust_P', 13767, 's_n_task_force',  '1', NULL, NULL, 1),"
			"('Push_Dust_P', 13768, 's_n_team_number', '1', NULL, NULL, 1),"
			"('Push_Dust_P', 13768, 's_n_task_force',  '1', NULL, NULL, 1),"
			"('Push_Dust_P', 13766, 's_n_task_force',  '2', NULL, NULL, 1),"
			"('Push_Dust_P', 13766, 's_n_team_number', '2', NULL, NULL, 1),"
			"('Push_Dust_P', 13765, 's_n_task_force',  '2', NULL, NULL, 1),"
			"('Push_Dust_P', 13765, 's_n_team_number', '2', NULL, NULL, 1),"
			"('Push_Dust_P', 13764, 's_n_team_number', '1', NULL, NULL, 1),"
			"('Push_Dust_P', 13764, 's_n_task_force',  '1', NULL, NULL, 1),"
			"('Push_Dust_P', 13763, 's_n_team_number', '1', NULL, NULL, 1),"
			"('Push_Dust_P', 13763, 's_n_task_force',  '1', NULL, NULL, 1),"
			"('Push_Dust_P', 13759, 'm_n_task_force',  '2', NULL, NULL, 1),"
			"('Push_Dust_P', 13758, 'm_n_task_force',  '1', NULL, NULL, 1);";
		result = sqlite3_exec(db, kV49_push_dust, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v49 (Push_Dust_P seed): %s\n", err); return; }
		Logger::Log("db", "v49: seeded 26 map_object_config rows for Push_Dust_P\n");
	}

	if (version < 50) {
		// v50: Push_IceFloe_P — task force / team assignments for actor-factory
		// (s_n_task_force / s_n_team_number) and team-player-start
		// (m_n_task_force) spawn points. All map-scoped per v46 rules.
		// Exported from the map planner.
		const char* kV50_ice_floe =
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES "
			"('Push_IceFloe_P', 5648, 'm_n_task_force',  '2', NULL, NULL, 1),"
			"('Push_IceFloe_P', 8131, 's_n_task_force',  '2', NULL, NULL, 1),"
			"('Push_IceFloe_P', 8131, 's_n_team_number', '2', NULL, NULL, 1),"
			"('Push_IceFloe_P', 8132, 's_n_task_force',  '2', NULL, NULL, 1),"
			"('Push_IceFloe_P', 8132, 's_n_team_number', '2', NULL, NULL, 1),"
			"('Push_IceFloe_P', 8136, 's_n_team_number', '2', NULL, NULL, 1),"
			"('Push_IceFloe_P', 8136, 's_n_task_force',  '2', NULL, NULL, 1),"
			"('Push_IceFloe_P', 8137, 's_n_team_number', '2', NULL, NULL, 1),"
			"('Push_IceFloe_P', 8137, 's_n_task_force',  '2', NULL, NULL, 1),"
			"('Push_IceFloe_P', 8135, 's_n_team_number', '2', NULL, NULL, 1),"
			"('Push_IceFloe_P', 8135, 's_n_task_force',  '2', NULL, NULL, 1),"
			"('Push_IceFloe_P', 8133, 's_n_team_number', '2', NULL, NULL, 1),"
			"('Push_IceFloe_P', 8133, 's_n_task_force',  '2', NULL, NULL, 1),"
			"('Push_IceFloe_P', 8129, 's_n_team_number', '1', NULL, NULL, 1),"
			"('Push_IceFloe_P', 8129, 's_n_task_force',  '1', NULL, NULL, 1),"
			"('Push_IceFloe_P', 8130, 's_n_team_number', '1', NULL, NULL, 1),"
			"('Push_IceFloe_P', 8130, 's_n_task_force',  '1', NULL, NULL, 1),"
			"('Push_IceFloe_P', 8125, 's_n_team_number', '1', NULL, NULL, 1),"
			"('Push_IceFloe_P', 8125, 's_n_task_force',  '1', NULL, NULL, 1),"
			"('Push_IceFloe_P', 8123, 's_n_team_number', '1', NULL, NULL, 1),"
			"('Push_IceFloe_P', 8123, 's_n_task_force',  '1', NULL, NULL, 1),"
			"('Push_IceFloe_P', 8124, 's_n_task_force',  '1', NULL, NULL, 1),"
			"('Push_IceFloe_P', 8124, 's_n_team_number', '1', NULL, NULL, 1),"
			"('Push_IceFloe_P', 8126, 's_n_team_number', '1', NULL, NULL, 1),"
			"('Push_IceFloe_P', 8126, 's_n_task_force',  '1', NULL, NULL, 1),"
			"('Push_IceFloe_P', 8140, 'm_n_task_force',  '2', NULL, NULL, 1),"
			"('Push_IceFloe_P', 8139, 'm_n_task_force',  '2', NULL, NULL, 1),"
			"('Push_IceFloe_P', 8128, 'm_n_task_force',  '1', NULL, NULL, 1),"
			"('Push_IceFloe_P', 5646, 'm_n_task_force',  '1', NULL, NULL, 1),"
			"('Push_IceFloe_P', 8127, 'm_n_task_force',  '1', NULL, NULL, 1);";
		result = sqlite3_exec(db, kV50_ice_floe, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v50 (Push_IceFloe_P seed): %s\n", err); return; }
		Logger::Log("db", "v50: seeded 30 map_object_config rows for Push_IceFloe_P\n");
	}

	if (version < 51) {
		// v51: map_game_info — manual lookup table that closes the gap between
		// the wire-protocol map_game_id and the five fields the GO_PLAY-family
		// messages need: map_name (the .upk filename), game_class (the UC
		// game-mode class), gameplay_type_value_id, friendly_name_msg_id,
		// entry_background_image_res_id.
		//
		// The (map_game_id, friendly_name_msg_id) pairs below are AUTHORITATIVE
		// — extracted from the original server's DATA_SET_MAP_CONFIGS (0x0170)
		// packet captured during a real login session. All 30 PvP playable maps
		// are represented. The other 66 production map_game_ids (lobbies,
		// quest maps, training, etc.) are not in this packet and not seeded.
		//
		// gameplay_type_value_id is inferred from the friendly-name prefix per
		// asm_data_set_valid_values group 171:
		//   1544 BREACH, 1545 CONTROL, 1546 ACQUISITION, 1547 PAYLOAD,
		//   1548 SCRAMBLE. The four 4v4 "Arena" maps are guessed as 1569
		//   (PVP- Arena) — verify when wiring up the first arena match.
		//
		// map_name and game_class come from the project owner's manual mapping.
		// Notes:
		//   - ACQUISITION maps (Hart Station / The Crossroads / Kimerial Point)
		//     use TgGame_DualCTF; filenames are PLACEHOLDERS (CTR_DuelStrike*)
		//     to be corrected by hand.
		//   - Six of the seven CONTROL maps got PLACEHOLDER filenames
		//     (SeaSide_Ticket*/Ticket_Datafarm*) because the original maps
		//     looked alike and the project owner planned to correct them by
		//     hand; only Magmarock (Ticket_Volcano_P) is confirmed.
		//
		// entry_background_image_res_id is now seeded inline from a hand-curated
		// pass over asm_data_set_resources WHERE res_type_value_id = 664. The
		// naming convention varies: some maps use HUD_MissionLoads.PvP.<name>,
		// most use HUD_MissionLoads.loading_<theme><N>, arenas use
		// control_4v4_*/ticket_4v4_*. Each value below was matched by friendly
		// name + numeric suffix; medium-confidence rows are flagged so they're
		// easy to spot. Two ACQUISITION maps share the same res_id because the
		// DB only has two dualstrike-themed loading screens for three maps.
		const char* kV51_schema =
			"CREATE TABLE IF NOT EXISTS map_game_info ("
			"  map_game_id                   INTEGER PRIMARY KEY,"
			"  map_name                      TEXT,"
			"  game_class                    TEXT,"
			"  gameplay_type_value_id        INTEGER,"
			"  friendly_name_msg_id          INTEGER NOT NULL,"
			"  entry_background_image_res_id INTEGER"
			");"
			"CREATE INDEX IF NOT EXISTS idx_map_game_info_map_name "
			"  ON map_game_info(map_name);";
		result = sqlite3_exec(db, kV51_schema, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v51 (map_game_info schema): %s\n", err); return; }

		const char* kV51_seed =
			"INSERT OR REPLACE INTO map_game_info (map_game_id, map_name, game_class, gameplay_type_value_id, friendly_name_msg_id, entry_background_image_res_id) VALUES "
			// Arena / 4v4 (4 maps) — 1569 PVP- Arena (best guess)
			"(1131, 'Ticket_HimLab_4v4',      'TgGame.TgGame_PointRotation', 1569, 43925, 6027),"  // HM-44 ARENA              -> control_4v4_him_loading
			"(1171, 'Ticket_Silo_4v4_P',      'TgGame.TgGame_PointRotation', 1569, 50819, 6028),"  // Z13-R SILO ARENA         -> control_4v4_silo_loading
			"(1369, 'Ticket_Osprey_4v4_P',    'TgGame.TgGame_PointRotation', 1569, 52695, 6166),"  // X1 Osprey Arena          -> ticket_4v4_osprey_loading
			"(1439, 'MissileComplex_4v4_P',   'TgGame.TgGame_Mission',       1569, 64636, 7373),"  // Triumph 9 Missile        -> PvP.2P_4v4MissileComplex
			// ACQUISITION (3 maps) — 1546 PVP - Aquisition Merc (TgGame_DualCTF)
			// PLACEHOLDER filenames; only 2 dualstrike res_ids exist so Hart and Kimerial share.
			"(1287, 'CTR_DuelStrike_P',       'TgGame.TgGame_DualCTF',       1546, 35942, 5146),"  // Hart Station   (placeholder) -> loading_dualstrike_a
			"(1112, 'CTR_DuelStrike2_P',      'TgGame.TgGame_DualCTF',       1546, 34477, 5713),"  // The Crossroads (placeholder) -> loading_dualstrike2
			"(1304, 'CTR_DuelStrike3_P',      'TgGame.TgGame_DualCTF',       1546, 36824, 5146),"  // Kimerial Point (placeholder) -> loading_dualstrike_a (shared, no 3rd)
			// PAYLOAD (5 maps) — 1547 PVP- Payload Merc (TgGame_Escort)
			"( 795, 'Push_IceFloe_P',         'TgGame.TgGame_Escort',        1547, 33843, 4959),"  // Ice Floe         -> loading_icefloe_01
			"(1272, 'Push_IceFloe3_P',        'TgGame.TgGame_Escort',        1547, 34206, 5712),"  // Tundra           -> loading_icefloe3
			"(1445, 'Push_Dust_P',            'TgGame.TgGame_Escort',        1547, 64890, 7411),"  // Haulin' Acid     -> PvP.Haulin_Acid_P
			"(1245, 'push_Ravine_P',          'TgGame.TgGame_Escort',        1547, 33851, 4976),"  // Ravine           -> loading_ravine_a
			"(1417, 'Push_Toxicity',          'TgGame.TgGame_Escort',        1547, 60144, 7346),"  // Toxicity         -> PvP.Push_Toxicity
			// SCRAMBLE (5 maps) — 1548 PVP- Scramble Merc (TgGame_PointRotation)
			// Rot_Redistribution{03,04,05} -> loading_redistribution{1,2,3} is a monotonic guess; verify.
			"(1373, 'Rot_BlackwaterLoch_P',   'TgGame.TgGame_PointRotation', 1548, 55501, 7508),"  // Blackwater Loch  -> PvP.Blackwater_Loading
			"(1205, 'Rot_Redistribution04',   'TgGame.TgGame_PointRotation', 1548, 36025, 5714),"  // Redistribution   -> loading_redistribution1 (guess)
			"(1291, 'Rot_Redistribution05',   'TgGame.TgGame_PointRotation', 1548, 36021, 5716),"  // Tetra Pier       -> loading_redistribution3 (guess)
			"(1307, 'Rot_Redistribution03',   'TgGame.TgGame_PointRotation', 1548, 37050, 5715),"  // Stockpile        -> loading_redistribution2 (guess)
			"(1372, 'Rot_Trafalgar_P',        'TgGame.TgGame_PointRotation', 1548, 52820, 7507),"  // CNS Trafalgar    -> PvP.Trafalgar_Loading
			// BREACH (6 maps) — 1544 PVP- Breach Merc (TgGame_Mission)
			"(1303, '3P_Beachhead3_P',        'TgGame.TgGame_Mission',       1544, 36815, 5461),"  // Beachhead        -> loading_beachhead3_a
			"(1227, 'Climate_Control_P',      'TgGame.TgGame_Mission',       1544, 33849, 5709),"  // Climate Control  -> loading_climatecontrol1
			"(1295, '3P_Climate_Control3_P',  'TgGame.TgGame_Mission',       1544, 40458, 5710),"  // Silent Thunder   -> loading_climatecontrol3
			"(1438, '3P_Him_Arena_P',         'TgGame.TgGame_Mission',       1544, 64339, 7348),"  // Himalayan Point  -> PvP.3P_Him_Arena
			"(1308, 'Ice_GorgeA01_v2',        'TgGame.TgGame_Mission',       1544, 37052, 5711),"  // Ice Gorge        -> loading_icegorge1
			"(1419, '3P_VolcanoAssault_P',    'TgGame.TgGame_Mission',       1544, 60172, 7347),"  // Volcano Assault  -> PvP.3P_VolcanoAssault_P
			// CONTROL (7 maps) — 1545 PVP- Control Merc (TgGame_Ticket)
			// Map_name PLACEHOLDERS; res_ids are best-guess by friendly-name<->theme match.
			"(1224, 'Ticket_Datafarm_P',      'TgGame.TgGame_Ticket',        1545, 33847, 5706),"  // Data Farm        (placeholder) -> loading_datafarm1
			"(1243, 'Ticket_Datafarm2',       'TgGame.TgGame_Ticket',        1545, 33848, 5707),"  // Sun Spot         (placeholder) -> loading_datafarm2
			"(1270, 'Ticket_Datafarm3',       'TgGame.TgGame_Ticket',        1545, 34189, 5708),"  // Harvest          (placeholder) -> loading_datafarm3
			"( 963, 'SeaSide_Ticket_P',       'TgGame.TgGame_Ticket',        1545, 33844, 5703),"  // Seaside          (placeholder) -> loading_seaside1
			"(1241, 'SeaSide_Ticket2_P',      'TgGame.TgGame_Ticket',        1545, 33845, 5704),"  // Azores Complex   (placeholder) -> loading_seaside2
			"(1246, 'SeaSide_Ticket3',        'TgGame.TgGame_Ticket',        1545, 33846, 5705),"  // Brine Complex    (placeholder) -> loading_seaside3
			"(1450, 'Ticket_Volcano_P',       'TgGame.TgGame_Ticket',        1545, 65001, 7605);"; // Magmarock         -> PvP.Ticket_Volcano
		result = sqlite3_exec(db, kV51_seed, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v51 (map_game_info seed): %s\n", err); return; }
		Logger::Log("db", "v51: created map_game_info + seeded 30 PvP map_game_ids "
		                  "(4 Arena, 3 Acquisition, 5 Payload, 5 Scramble, 6 Breach, 7 Control)\n");
	}

	if (version < 52) {
		// v52: map_object_config overrides — task force / team assignments for
		// the seven Control (TgGame_Ticket) maps. Same shape as v47/v48/v49/v50.
		// All map-scoped per v46 rules. The three Datafarm and three SeaSide
		// variants share map_object_ids within their theme (e.g. 11548 appears
		// on all three Datafarm maps with the same value); v46 precedence
		// correctly resolves per-map. Note: Ticket_Volcano_P/13612 collides
		// with v47's 3P_VolcanoAssault_P/13612 (different value), which is the
		// exact case map-scoping was designed for.
		const char* kV52_datafarm1 =
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES "
			"('Ticket_Datafarm_P', 11548, 's_n_task_force',  '2', NULL, NULL, 1),"
			"('Ticket_Datafarm_P', 11548, 's_n_team_number', '2', NULL, NULL, 1),"
			"('Ticket_Datafarm_P', 11549, 's_n_task_force',  '2', NULL, NULL, 1),"
			"('Ticket_Datafarm_P', 11549, 's_n_team_number', '2', NULL, NULL, 1),"
			"('Ticket_Datafarm_P', 11550, 'm_n_task_force',  '2', NULL, NULL, 1),"
			"('Ticket_Datafarm_P', 11551, 'm_n_task_force',  '1', NULL, NULL, 1),"
			"('Ticket_Datafarm_P', 11552, 's_n_task_force',  '1', NULL, NULL, 1),"
			"('Ticket_Datafarm_P', 11552, 's_n_team_number', '1', NULL, NULL, 1),"
			"('Ticket_Datafarm_P', 11553, 's_n_task_force',  '1', NULL, NULL, 1),"
			"('Ticket_Datafarm_P', 11553, 's_n_team_number', '1', NULL, NULL, 1);";
		result = sqlite3_exec(db, kV52_datafarm1, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v52 (Ticket_Datafarm_P seed): %s\n", err); return; }

		const char* kV52_datafarm2 =
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES "
			"('Ticket_Datafarm2', 11548, 's_n_task_force',  '2', NULL, NULL, 1),"
			"('Ticket_Datafarm2', 11548, 's_n_team_number', '2', NULL, NULL, 1),"
			"('Ticket_Datafarm2', 11549, 's_n_task_force',  '2', NULL, NULL, 1),"
			"('Ticket_Datafarm2', 11549, 's_n_team_number', '2', NULL, NULL, 1),"
			"('Ticket_Datafarm2', 11550, 'm_n_task_force',  '2', NULL, NULL, 1),"
			"('Ticket_Datafarm2', 11551, 'm_n_task_force',  '1', NULL, NULL, 1),"
			"('Ticket_Datafarm2', 11552, 's_n_task_force',  '1', NULL, NULL, 1),"
			"('Ticket_Datafarm2', 11552, 's_n_team_number', '1', NULL, NULL, 1),"
			"('Ticket_Datafarm2', 11553, 's_n_task_force',  '1', NULL, NULL, 1),"
			"('Ticket_Datafarm2', 11553, 's_n_team_number', '1', NULL, NULL, 1);";
		result = sqlite3_exec(db, kV52_datafarm2, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v52 (Ticket_Datafarm2 seed): %s\n", err); return; }

		const char* kV52_datafarm3 =
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES "
			"('Ticket_Datafarm3', 11548, 's_n_task_force',  '2', NULL, NULL, 1),"
			"('Ticket_Datafarm3', 11548, 's_n_team_number', '2', NULL, NULL, 1),"
			"('Ticket_Datafarm3', 11549, 's_n_task_force',  '2', NULL, NULL, 1),"
			"('Ticket_Datafarm3', 11549, 's_n_team_number', '2', NULL, NULL, 1),"
			"('Ticket_Datafarm3', 11550, 'm_n_task_force',  '2', NULL, NULL, 1),"
			"('Ticket_Datafarm3', 11551, 'm_n_task_force',  '1', NULL, NULL, 1),"
			"('Ticket_Datafarm3', 11552, 's_n_task_force',  '1', NULL, NULL, 1),"
			"('Ticket_Datafarm3', 11552, 's_n_team_number', '1', NULL, NULL, 1),"
			"('Ticket_Datafarm3', 11553, 's_n_task_force',  '1', NULL, NULL, 1),"
			"('Ticket_Datafarm3', 11553, 's_n_team_number', '1', NULL, NULL, 1);";
		result = sqlite3_exec(db, kV52_datafarm3, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v52 (Ticket_Datafarm3 seed): %s\n", err); return; }

		const char* kV52_seaside1 =
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES "
			"('SeaSide_Ticket_P', 9076, 's_n_team_number', '1', NULL, NULL, 1),"
			"('SeaSide_Ticket_P', 9076, 's_n_task_force',  '1', NULL, NULL, 1),"
			"('SeaSide_Ticket_P', 9075, 's_n_team_number', '1', NULL, NULL, 1),"
			"('SeaSide_Ticket_P', 9075, 's_n_task_force',  '1', NULL, NULL, 1),"
			"('SeaSide_Ticket_P', 9074, 's_n_team_number', '2', NULL, NULL, 1),"
			"('SeaSide_Ticket_P', 9074, 's_n_task_force',  '2', NULL, NULL, 1),"
			"('SeaSide_Ticket_P', 9073, 's_n_team_number', '2', NULL, NULL, 1),"
			"('SeaSide_Ticket_P', 9073, 's_n_task_force',  '2', NULL, NULL, 1),"
			"('SeaSide_Ticket_P', 8207, 'm_n_task_force',  '2', NULL, NULL, 1),"
			"('SeaSide_Ticket_P', 8208, 'm_n_task_force',  '1', NULL, NULL, 1);";
		result = sqlite3_exec(db, kV52_seaside1, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v52 (SeaSide_Ticket_P seed): %s\n", err); return; }

		const char* kV52_seaside2 =
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES "
			"('SeaSide_Ticket2_P', 9076, 's_n_team_number', '1', NULL, NULL, 1),"
			"('SeaSide_Ticket2_P', 9076, 's_n_task_force',  '1', NULL, NULL, 1),"
			"('SeaSide_Ticket2_P', 9075, 's_n_task_force',  '1', NULL, NULL, 1),"
			"('SeaSide_Ticket2_P', 9075, 's_n_team_number', '1', NULL, NULL, 1),"
			"('SeaSide_Ticket2_P', 9074, 's_n_team_number', '2', NULL, NULL, 1),"
			"('SeaSide_Ticket2_P', 9074, 's_n_task_force',  '2', NULL, NULL, 1),"
			"('SeaSide_Ticket2_P', 9073, 's_n_task_force',  '2', NULL, NULL, 1),"
			"('SeaSide_Ticket2_P', 9073, 's_n_team_number', '2', NULL, NULL, 1),"
			"('SeaSide_Ticket2_P', 8208, 'm_n_task_force',  '1', NULL, NULL, 1),"
			"('SeaSide_Ticket2_P', 8207, 'm_n_task_force',  '2', NULL, NULL, 1);";
		result = sqlite3_exec(db, kV52_seaside2, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v52 (SeaSide_Ticket2_P seed): %s\n", err); return; }

		const char* kV52_seaside3 =
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES "
			"('SeaSide_Ticket3', 9076, 's_n_team_number', '1', NULL, NULL, 1),"
			"('SeaSide_Ticket3', 9076, 's_n_task_force',  '1', NULL, NULL, 1),"
			"('SeaSide_Ticket3', 9075, 's_n_team_number', '1', NULL, NULL, 1),"
			"('SeaSide_Ticket3', 9075, 's_n_task_force',  '1', NULL, NULL, 1),"
			"('SeaSide_Ticket3', 9074, 's_n_team_number', '2', NULL, NULL, 1),"
			"('SeaSide_Ticket3', 9074, 's_n_task_force',  '2', NULL, NULL, 1),"
			"('SeaSide_Ticket3', 9073, 's_n_team_number', '2', NULL, NULL, 1),"
			"('SeaSide_Ticket3', 9073, 's_n_task_force',  '2', NULL, NULL, 1),"
			"('SeaSide_Ticket3', 8208, 'm_n_task_force',  '1', NULL, NULL, 1),"
			"('SeaSide_Ticket3', 8207, 'm_n_task_force',  '2', NULL, NULL, 1);";
		result = sqlite3_exec(db, kV52_seaside3, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v52 (SeaSide_Ticket3 seed): %s\n", err); return; }

		const char* kV52_volcano =
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES "
			"('Ticket_Volcano_P', 13864, 's_n_team_number', '1', NULL, NULL, 1),"
			"('Ticket_Volcano_P', 13864, 's_n_task_force',  '1', NULL, NULL, 1),"
			"('Ticket_Volcano_P', 13865, 's_n_task_force',  '1', NULL, NULL, 1),"
			"('Ticket_Volcano_P', 13865, 's_n_team_number', '1', NULL, NULL, 1),"
			"('Ticket_Volcano_P', 13821, 's_n_task_force',  '2', NULL, NULL, 1),"
			"('Ticket_Volcano_P', 13821, 's_n_team_number', '2', NULL, NULL, 1),"
			"('Ticket_Volcano_P', 13820, 's_n_task_force',  '2', NULL, NULL, 1),"
			"('Ticket_Volcano_P', 13820, 's_n_team_number', '2', NULL, NULL, 1),"
			"('Ticket_Volcano_P', 13612, 'm_n_task_force',  '2', NULL, NULL, 1),"
			"('Ticket_Volcano_P', 13863, 'm_n_task_force',  '1', NULL, NULL, 1);";
		result = sqlite3_exec(db, kV52_volcano, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v52 (Ticket_Volcano_P seed): %s\n", err); return; }

		Logger::Log("db", "v52: seeded 70 map_object_config rows across 7 Control maps "
		                  "(3 Datafarm + 3 SeaSide + Volcano)\n");
	}

	if (version < 53) {
		// v53: account-scoped inventory + simplified equipped-devices mapping.
		//
		// BEFORE: ga_character_devices held the full item record (device_id,
		// quality, mods, oc) per character — items were re-seeded on every
		// character select from ClassLoadouts.cpp. Inventory was effectively
		// just "what's currently equipped" with no notion of unequipped items
		// to choose from.
		//
		// AFTER:
		//  - ga_players_inventory: ACCOUNT-scoped (user_id) item pool. Each
		//    item knows its profile_id (0=shared, else medic/recon/etc) and
		//    `allowed_slots` (CSV of slot_value_ids — usually one, but offhand
		//    devices list all three of 203/204/385). This is the new source of
		//    truth for `inventory_id` (the primary key).
		//  - ga_character_devices: thin join row pointing into the inventory
		//    pool — only carries the equip slot. UNIQUE(character_id,
		//    equipped_slot) prevents two items competing for the same slot.
		//    (When we add loadout-profile support later, the unique key gets
		//    `loadout_profile` mixed in and we hardcode 1 in the interim.)
		//
		// Wipe of the old ga_character_devices is expected — the row shape is
		// different and the seed will repopulate inventory + equipment from
		// ClassLoadouts.cpp on next login. SQLite has no "DROP COLUMN" in this
		// build, so we drop+recreate.
		const char* kV53_inventory =
			"CREATE TABLE IF NOT EXISTS ga_players_inventory ("
			"  id                   INTEGER PRIMARY KEY AUTOINCREMENT,"
			"  user_id              INTEGER NOT NULL REFERENCES ga_users(id),"
			"  profile_id           INTEGER NOT NULL DEFAULT 0,"
			"  device_id            INTEGER NOT NULL,"
			"  quality              INTEGER NOT NULL DEFAULT 0,"
			"  mod_effect_group_ids TEXT    NOT NULL DEFAULT '',"
			"  oc                   INTEGER NOT NULL DEFAULT 0,"
			"  allowed_slots        TEXT    NOT NULL,"
			"  created_at           INTEGER NOT NULL DEFAULT (strftime('%s','now'))"
			");"
			"CREATE INDEX IF NOT EXISTS idx_ga_players_inventory_user "
			"  ON ga_players_inventory(user_id, profile_id);";
		result = sqlite3_exec(db, kV53_inventory, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v53 (ga_players_inventory): %s\n", err); sqlite3_free(err); return; }

		const bool devices_modern =
			TableColumnExists(db, "ga_character_devices", "item_profile_id") &&
			TableColumnExists(db, "ga_character_devices", "equipped_slot");
		if (devices_modern) {
			result = sqlite3_exec(db,
				"CREATE INDEX IF NOT EXISTS idx_ga_character_devices_char "
				"ON ga_character_devices(character_id);",
				nullptr, nullptr, &err);
			if (result != SQLITE_OK) { Logger::Log("db", "Failed v53 (ga_character_devices index): %s\n", err); sqlite3_free(err); return; }
			Logger::Log("db", "v53: ga_character_devices already modern; preserved player equipment\n");
		} else {
			const char* kV53_devices_rebuild =
				"DROP TABLE IF EXISTS ga_character_devices_v2;"
				"CREATE TABLE ga_character_devices_v2 ("
				"  id              INTEGER PRIMARY KEY AUTOINCREMENT,"
				"  character_id    INTEGER NOT NULL REFERENCES ga_characters(id),"
				"  item_profile_id INTEGER NOT NULL,"
				"  inventory_id    INTEGER NOT NULL REFERENCES ga_players_inventory(id),"
				"  equipped_slot   INTEGER NOT NULL,"
				"  UNIQUE(character_id, item_profile_id, equipped_slot)"
				");"
				"INSERT INTO ga_character_devices_v2 (character_id, item_profile_id, inventory_id, equipped_slot)"
				" SELECT character_id, 1, inventory_id, equipped_slot FROM ga_character_devices;"
				"DROP TABLE IF EXISTS ga_character_devices;"
				"ALTER TABLE ga_character_devices_v2 RENAME TO ga_character_devices;"
				"CREATE INDEX IF NOT EXISTS idx_ga_character_devices_char "
				"  ON ga_character_devices(character_id);";
			result = sqlite3_exec(db, kV53_devices_rebuild, nullptr, nullptr, &err);
			if (result != SQLITE_OK) { Logger::Log("db", "Failed v53 (ga_character_devices modern rebuild): %s\n", err); sqlite3_free(err); return; }
			Logger::Log("db", "v53: rebuilt ga_character_devices to modern profile-aware schema\n");
		}
	}

	if (version < 54) {
		// v54: Push_IceFloe3_P — task force / team assignments for actor-factory
		// (s_n_task_force / s_n_team_number) and team-player-start
		// (m_n_task_force) spawn points. Same shape as v50 (Push_IceFloe_P).
		// Exported from the map planner.
		const char* kV54_ice_floe3 =
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES "
			"('Push_IceFloe3_P',  5646, 'm_n_task_force',  '1', NULL, NULL, 1),"
			"('Push_IceFloe3_P',  8126, 's_n_task_force',  '1', NULL, NULL, 1),"
			"('Push_IceFloe3_P',  8126, 's_n_team_number', '1', NULL, NULL, 1),"
			"('Push_IceFloe3_P',  8124, 's_n_task_force',  '1', NULL, NULL, 1),"
			"('Push_IceFloe3_P',  8124, 's_n_team_number', '1', NULL, NULL, 1),"
			"('Push_IceFloe3_P',  8125, 's_n_task_force',  '1', NULL, NULL, 1),"
			"('Push_IceFloe3_P',  8125, 's_n_team_number', '1', NULL, NULL, 1),"
			"('Push_IceFloe3_P',  8123, 's_n_task_force',  '1', NULL, NULL, 1),"
			"('Push_IceFloe3_P',  8123, 's_n_team_number', '1', NULL, NULL, 1),"
			"('Push_IceFloe3_P',  8133, 's_n_task_force',  '2', NULL, NULL, 1),"
			"('Push_IceFloe3_P',  8133, 's_n_team_number', '2', NULL, NULL, 1),"
			"('Push_IceFloe3_P',  8135, 's_n_team_number', '2', NULL, NULL, 1),"
			"('Push_IceFloe3_P',  8135, 's_n_task_force',  '2', NULL, NULL, 1),"
			"('Push_IceFloe3_P', 12078, 's_n_task_force',  '1', NULL, NULL, 1),"
			"('Push_IceFloe3_P', 12078, 's_n_team_number', '1', NULL, NULL, 1),"
			"('Push_IceFloe3_P', 12079, 's_n_task_force',  '1', NULL, NULL, 1),"
			"('Push_IceFloe3_P', 12079, 's_n_team_number', '1', NULL, NULL, 1),"
			"('Push_IceFloe3_P', 12004, 's_n_task_force',  '2', NULL, NULL, 1),"
			"('Push_IceFloe3_P', 12004, 's_n_team_number', '2', NULL, NULL, 1),"
			"('Push_IceFloe3_P', 12006, 's_n_task_force',  '2', NULL, NULL, 1),"
			"('Push_IceFloe3_P', 12006, 's_n_team_number', '2', NULL, NULL, 1),"
			"('Push_IceFloe3_P', 12100, 's_n_task_force',  '2', NULL, NULL, 1),"
			"('Push_IceFloe3_P', 12100, 's_n_team_number', '2', NULL, NULL, 1),"
			"('Push_IceFloe3_P', 12101, 's_n_team_number', '2', NULL, NULL, 1),"
			"('Push_IceFloe3_P', 12101, 's_n_task_force',  '2', NULL, NULL, 1),"
			"('Push_IceFloe3_P',  8127, 'm_n_task_force',  '1', NULL, NULL, 1),"
			"('Push_IceFloe3_P', 12099, 'm_n_task_force',  '2', NULL, NULL, 1),"
			"('Push_IceFloe3_P',  5648, 'm_n_task_force',  '2', NULL, NULL, 1),"
			"('Push_IceFloe3_P',  8140, 'm_n_task_force',  '2', NULL, NULL, 1),"
			"('Push_IceFloe3_P', 12077, 'm_n_task_force',  '1', NULL, NULL, 1);";
		result = sqlite3_exec(db, kV54_ice_floe3, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v54 (Push_IceFloe3_P seed): %s\n", err); return; }
		Logger::Log("db", "v54: seeded 30 map_object_config rows for Push_IceFloe3_P\n");
	}


	if (version < 55) {
		// v55: HEX_AVA_2pt_Theft_Factory1_P — task force / team assignments for
		// two actor factories (8600 → TF2, 8590 → TF1) and one team-player-start
		// (13074 → TF2). Same shape as v54.
		const char* kV55_hex_ava_2pt_theft_factory1 =
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES "
			"('HEX_AVA_2pt_Theft_Factory1_P',  8600, 's_n_team_number', '2', NULL, NULL, 1),"
			"('HEX_AVA_2pt_Theft_Factory1_P',  8600, 's_n_task_force',  '2', NULL, NULL, 1),"
			"('HEX_AVA_2pt_Theft_Factory1_P',  8590, 's_n_team_number', '1', NULL, NULL, 1),"
			"('HEX_AVA_2pt_Theft_Factory1_P',  8590, 's_n_task_force',  '1', NULL, NULL, 1),"
			"('HEX_AVA_2pt_Theft_Factory1_P', 13074, 'm_n_task_force',  '2', NULL, NULL, 1);";
		result = sqlite3_exec(db, kV55_hex_ava_2pt_theft_factory1, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v55 (HEX_AVA_2pt_Theft_Factory1_P seed): %s\n", err); return; }
		Logger::Log("db", "v55: seeded 5 map_object_config rows for HEX_AVA_2pt_Theft_Factory1_P\n");
	}

	if (version < 56) {
		const char* kV56_missile_complex_4v4 =
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES"
			" ('MissileComplex_4v4_P', 5646, 'm_n_task_force', '1', NULL, NULL, 1),    "
			" ('MissileComplex_4v4_P', 13748, 's_n_task_force', '1', NULL, NULL, 1),   "
			" ('MissileComplex_4v4_P', 13748, 's_n_team_number', '1', NULL, NULL, 1),  "
			" ('MissileComplex_4v4_P', 13746, 's_n_task_force', '1', NULL, NULL, 1),   "
			" ('MissileComplex_4v4_P', 13746, 's_n_team_number', '1', NULL, NULL, 1),  "
			" ('MissileComplex_4v4_P', 7820, 'm_n_task_force', '2', NULL, NULL, 1),    "
			" ('MissileComplex_4v4_P', 7818, 's_n_team_number', '2', NULL, NULL, 1),   "
			" ('MissileComplex_4v4_P', 7818, 's_n_task_force', '2', NULL, NULL, 1),    "
			" ('MissileComplex_4v4_P', 7819, 's_n_task_force', '2', NULL, NULL, 1),    "
			" ('MissileComplex_4v4_P', 7819, 's_n_team_number', '2', NULL, NULL, 1),   "
			" ('MissileComplex_4v4_P', 5648, 'm_n_task_force', '2', NULL, NULL, 1),    "
			" ('MissileComplex_4v4_P', 7828, 's_n_team_number', '2', NULL, NULL, 1),   "
			" ('MissileComplex_4v4_P', 7828, 's_n_task_force', '2', NULL, NULL, 1),    "
			" ('MissileComplex_4v4_P', 7827, 's_n_team_number', '2', NULL, NULL, 1),   "
			" ('MissileComplex_4v4_P', 7827, 's_n_task_force', '2', NULL, NULL, 1),    "
			" ('MissileComplex_4v4_P', 13747, 's_n_team_number', '1', NULL, NULL, 1),  "
			" ('MissileComplex_4v4_P', 13747, 's_n_task_force', '1', NULL, NULL, 1),   "
			" ('MissileComplex_4v4_P', 7814, 's_n_task_force', '1', NULL, NULL, 1),    "
			" ('MissileComplex_4v4_P', 7814, 's_n_team_number', '1', NULL, NULL, 1);   ";

		result = sqlite3_exec(db, kV56_missile_complex_4v4, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v56 (kV56_missile_complex_4v4 seed): %s\n", err); return; }
		Logger::Log("db", "v56: seeded map_object_config for MissileComplex_4v4_P\n");
	}

	if (version < 57) {
		const char* kV57_ticket_himlab_4v4 =
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES "
			"('Ticket_HimLab_4v4', 10723, 'm_n_task_force', '2', NULL, NULL, 1),"
			"('Ticket_HimLab_4v4', 10659, 'm_n_task_force', '1', NULL, NULL, 1);";
		result = sqlite3_exec(db, kV57_ticket_himlab_4v4, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v57 (Ticket_HimLab_4v4 seed): %s\n", err); return; }
		Logger::Log("db", "v57: seeded map_object_config for Ticket_HimLab_4v4\n");
	}


	if (version < 58) {
		const char* kV58_seed =
			"INSERT OR REPLACE INTO map_game_info (map_game_id, map_name, game_class, gameplay_type_value_id, friendly_name_msg_id, entry_background_image_res_id) VALUES "
			"(100001, '3P_Beachhead_P',      'TgGame.TgGame_Mission', 1544, 36260, 5339),"  // surfside
			"(100002, '3P_Beachhead2_P',      'TgGame.TgGame_Mission', 1544, 36565, 5389);";  // atoll
		result = sqlite3_exec(db, kV58_seed, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v58 (map_game_info seed): %s\n", err); return; }
		Logger::Log("db", "v58: seeded map_game_ids\n");
	}

	if (version < 59) {
		const char* kV59_seed =
			"INSERT OR REPLACE INTO map_game_info (map_game_id, map_name, game_class, gameplay_type_value_id, friendly_name_msg_id, entry_background_image_res_id) VALUES "
			"(100003, 'CTR_Recursive_P',      'TgGame.TgGame_Mission', 1546, 65824, 6064);";  // surfside
		result = sqlite3_exec(db, kV59_seed, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v59 (map_game_info seed): %s\n", err); return; }
		Logger::Log("db", "v59: seeded map_game_ids\n");
	}

	if (version < 60) {
		// v60: 1P_CPFactory01_P — task force / team assignments for actor
		// factories + team-player-starts, plus spawn-table bindings
		// (n_spawn_table_id / n_default_spawn_table_id) for the 18 bot
		// factories on this map. 76 rows total, all map-scoped per v46.
		// Exported from the map planner 2026-05-23T09:38:38.995Z.
		// First map authored against the post-refactor TgBotFactory hooks
		// that consume n_spawn_table_id from MapObjectConfig directly.
		const char* kV60_cpfactory01 =
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES "
			"('1P_CPFactory01_P', 11717, 'm_n_task_force', '1', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 11716, 's_n_task_force', '1', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 11716, 's_n_team_number', '1', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12286, 's_n_task_force', '1', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12286, 's_n_team_number', '1', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12316, 'm_n_task_force', '2', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12289, 's_n_team_number', '2', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12289, 's_n_task_force', '2', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12287, 's_n_team_number', '2', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12287, 's_n_task_force', '2', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 11664, 's_n_task_force', '2', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 11664, 's_n_team_number', '2', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 11663, 's_n_task_force', '2', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 11663, 's_n_team_number', '2', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12283, 's_n_task_force', '2', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12283, 's_n_team_number', '2', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12274, 's_n_task_force', '2', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12274, 's_n_team_number', '2', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12273, 's_n_team_number', '2', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12273, 's_n_task_force', '2', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12272, 's_n_task_force', '2', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12272, 's_n_team_number', '2', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12271, 's_n_task_force', '2', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12271, 's_n_team_number', '2', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12270, 's_n_task_force', '2', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12270, 's_n_team_number', '2', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12275, 's_n_task_force', '2', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12275, 's_n_team_number', '2', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12282, 's_n_task_force', '2', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12282, 's_n_team_number', '2', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12276, 's_n_task_force', '2', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12276, 's_n_team_number', '2', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12269, 's_n_task_force', '2', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12269, 's_n_team_number', '2', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12280, 's_n_task_force', '2', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12280, 's_n_team_number', '2', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12267, 's_n_team_number', '2', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12267, 's_n_task_force', '2', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12266, 's_n_task_force', '2', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12266, 's_n_team_number', '2', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12265, 's_n_task_force', '2', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12265, 's_n_team_number', '2', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12264, 's_n_task_force', '2', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12264, 's_n_team_number', '2', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12263, 's_n_task_force', '2', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12263, 's_n_team_number', '2', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12278, 's_n_task_force', '2', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12278, 's_n_team_number', '2', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12277, 's_n_task_force', '2', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12277, 's_n_team_number', '2', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12276, 'n_default_spawn_table_id', '29', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12276, 'n_spawn_table_id', '29', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12282, 'n_default_spawn_table_id', '28', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12282, 'n_spawn_table_id', '28', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12275, 'n_default_spawn_table_id', '40', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12275, 'n_spawn_table_id', '40', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12270, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12270, 'n_spawn_table_id', '33', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12271, 'n_spawn_table_id', '51', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12271, 'n_default_spawn_table_id', '51', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12272, 'n_spawn_table_id', '51', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12272, 'n_default_spawn_table_id', '51', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12273, 'n_spawn_table_id', '33', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12273, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12274, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12274, 'n_spawn_table_id', '33', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12283, 'n_default_spawn_table_id', '59', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12283, 'n_spawn_table_id', '59', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 11663, 'n_spawn_table_id', '28', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 11663, 'n_default_spawn_table_id', '28', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 11664, 'n_spawn_table_id', '40', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 11664, 'n_default_spawn_table_id', '40', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12287, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12287, 'n_spawn_table_id', '33', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12289, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12289, 'n_spawn_table_id', '33', NULL, NULL, 1);";
		result = sqlite3_exec(db, kV60_cpfactory01, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v60 (1P_CPFactory01_P seed): %s\n", err); return; }
		Logger::Log("db", "v60: seeded 76 map_object_config rows for 1P_CPFactory01_P\n");
	}

	if (version < 61) {
		// v61: 1P_CPFactory01_P — fill in spawn_table_id for the 9 factories
		// that v60 didn't cover (12263–12269, 12277, 12278, 12280). Without
		// these the corresponding factories silently spawn nothing because
		// LoadObjectConfig bails on `nSpawnTableId <= 0`.
		const char* kV61_cpfactory01_more =
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES "
			"('1P_CPFactory01_P', 12277, 'n_spawn_table_id',         '28', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12277, 'n_default_spawn_table_id', '28', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12278, 'n_spawn_table_id',         '28', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12278, 'n_default_spawn_table_id', '28', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12263, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12263, 'n_spawn_table_id',         '33', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12264, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12264, 'n_spawn_table_id',         '33', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12265, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12265, 'n_spawn_table_id',         '33', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12266, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12266, 'n_spawn_table_id',         '33', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12267, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12267, 'n_spawn_table_id',         '33', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12280, 'n_default_spawn_table_id', '46', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12280, 'n_spawn_table_id',         '46', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12269, 'n_spawn_table_id',         '33', NULL, NULL, 1),"
			"('1P_CPFactory01_P', 12269, 'n_default_spawn_table_id', '33', NULL, NULL, 1);";
		result = sqlite3_exec(db, kV61_cpfactory01_more, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v61 (1P_CPFactory01_P fill-in seed): %s\n", err); return; }
		Logger::Log("db", "v61: seeded 18 map_object_config rows for 1P_CPFactory01_P (fill-in factories)\n");
	}

	if (version < 62) {
		// v62: 1P_CPLab05_P — port factory configuration from legacy
		// obj_bot_factories (mutator_number=0 rows) into map_object_config.
		// 25 TgBotFactory actors, all defenders (task_force=2). User-validated
		// map for testing the refactored BotFactory hooks against known-good
		// behaviour.
		const char* kV62_cplab05 =
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES "
			"('1P_CPLab05_P', 12698, 's_n_task_force',           '2',   NULL, NULL, 1),"
			"('1P_CPLab05_P', 12698, 's_n_team_number',          '2',   NULL, NULL, 1),"
			"('1P_CPLab05_P', 12698, 'n_spawn_table_id',         '41',  NULL, NULL, 1),"
			"('1P_CPLab05_P', 12698, 'n_default_spawn_table_id', '41',  NULL, NULL, 1),"
			"('1P_CPLab05_P', 12708, 's_n_task_force',           '2',   NULL, NULL, 1),"
			"('1P_CPLab05_P', 12708, 's_n_team_number',          '2',   NULL, NULL, 1),"
			"('1P_CPLab05_P', 12708, 'n_spawn_table_id',         '29',  NULL, NULL, 1),"
			"('1P_CPLab05_P', 12708, 'n_default_spawn_table_id', '29',  NULL, NULL, 1),"
			"('1P_CPLab05_P', 12709, 's_n_task_force',           '2',   NULL, NULL, 1),"
			"('1P_CPLab05_P', 12709, 's_n_team_number',          '2',   NULL, NULL, 1),"
			"('1P_CPLab05_P', 12709, 'n_spawn_table_id',         '28',  NULL, NULL, 1),"
			"('1P_CPLab05_P', 12709, 'n_default_spawn_table_id', '28',  NULL, NULL, 1),"
			"('1P_CPLab05_P', 12710, 's_n_task_force',           '2',   NULL, NULL, 1),"
			"('1P_CPLab05_P', 12710, 's_n_team_number',          '2',   NULL, NULL, 1),"
			"('1P_CPLab05_P', 12710, 'n_spawn_table_id',         '58',  NULL, NULL, 1),"
			"('1P_CPLab05_P', 12710, 'n_default_spawn_table_id', '58',  NULL, NULL, 1),"
			"('1P_CPLab05_P', 12711, 's_n_task_force',           '2',   NULL, NULL, 1),"
			"('1P_CPLab05_P', 12711, 's_n_team_number',          '2',   NULL, NULL, 1),"
			"('1P_CPLab05_P', 12711, 'n_spawn_table_id',         '29',  NULL, NULL, 1),"
			"('1P_CPLab05_P', 12711, 'n_default_spawn_table_id', '29',  NULL, NULL, 1),"
			"('1P_CPLab05_P', 12712, 's_n_task_force',           '2',   NULL, NULL, 1),"
			"('1P_CPLab05_P', 12712, 's_n_team_number',          '2',   NULL, NULL, 1),"
			"('1P_CPLab05_P', 12712, 'n_spawn_table_id',         '29',  NULL, NULL, 1),"
			"('1P_CPLab05_P', 12712, 'n_default_spawn_table_id', '29',  NULL, NULL, 1),"
			"('1P_CPLab05_P', 12713, 's_n_task_force',           '2',   NULL, NULL, 1),"
			"('1P_CPLab05_P', 12713, 's_n_team_number',          '2',   NULL, NULL, 1),"
			"('1P_CPLab05_P', 12713, 'n_spawn_table_id',         '34',  NULL, NULL, 1),"
			"('1P_CPLab05_P', 12713, 'n_default_spawn_table_id', '34',  NULL, NULL, 1),"
			"('1P_CPLab05_P', 12714, 's_n_task_force',           '2',   NULL, NULL, 1),"
			"('1P_CPLab05_P', 12714, 's_n_team_number',          '2',   NULL, NULL, 1),"
			"('1P_CPLab05_P', 12714, 'n_spawn_table_id',         '34',  NULL, NULL, 1),"
			"('1P_CPLab05_P', 12714, 'n_default_spawn_table_id', '34',  NULL, NULL, 1),"
			"('1P_CPLab05_P', 12724, 's_n_task_force',           '2',   NULL, NULL, 1),"
			"('1P_CPLab05_P', 12724, 's_n_team_number',          '2',   NULL, NULL, 1),"
			"('1P_CPLab05_P', 12724, 'n_spawn_table_id',         '33',  NULL, NULL, 1),"
			"('1P_CPLab05_P', 12724, 'n_default_spawn_table_id', '33',  NULL, NULL, 1),"
			"('1P_CPLab05_P', 12727, 's_n_task_force',           '2',   NULL, NULL, 1),"
			"('1P_CPLab05_P', 12727, 's_n_team_number',          '2',   NULL, NULL, 1),"
			"('1P_CPLab05_P', 12727, 'n_spawn_table_id',         '33',  NULL, NULL, 1),"
			"('1P_CPLab05_P', 12727, 'n_default_spawn_table_id', '33',  NULL, NULL, 1),"
			"('1P_CPLab05_P', 12728, 's_n_task_force',           '2',   NULL, NULL, 1),"
			"('1P_CPLab05_P', 12728, 's_n_team_number',          '2',   NULL, NULL, 1),"
			"('1P_CPLab05_P', 12728, 'n_spawn_table_id',         '33',  NULL, NULL, 1),"
			"('1P_CPLab05_P', 12728, 'n_default_spawn_table_id', '33',  NULL, NULL, 1),"
			"('1P_CPLab05_P', 12729, 's_n_task_force',           '2',   NULL, NULL, 1),"
			"('1P_CPLab05_P', 12729, 's_n_team_number',          '2',   NULL, NULL, 1),"
			"('1P_CPLab05_P', 12729, 'n_spawn_table_id',         '33',  NULL, NULL, 1),"
			"('1P_CPLab05_P', 12729, 'n_default_spawn_table_id', '33',  NULL, NULL, 1),"
			"('1P_CPLab05_P', 12730, 's_n_task_force',           '2',   NULL, NULL, 1),"
			"('1P_CPLab05_P', 12730, 's_n_team_number',          '2',   NULL, NULL, 1),"
			"('1P_CPLab05_P', 12730, 'n_spawn_table_id',         '33',  NULL, NULL, 1),"
			"('1P_CPLab05_P', 12730, 'n_default_spawn_table_id', '33',  NULL, NULL, 1),"
			"('1P_CPLab05_P', 12731, 's_n_task_force',           '2',   NULL, NULL, 1),"
			"('1P_CPLab05_P', 12731, 's_n_team_number',          '2',   NULL, NULL, 1),"
			"('1P_CPLab05_P', 12731, 'n_spawn_table_id',         '33',  NULL, NULL, 1),"
			"('1P_CPLab05_P', 12731, 'n_default_spawn_table_id', '33',  NULL, NULL, 1),"
			"('1P_CPLab05_P', 12732, 's_n_task_force',           '2',   NULL, NULL, 1),"
			"('1P_CPLab05_P', 12732, 's_n_team_number',          '2',   NULL, NULL, 1),"
			"('1P_CPLab05_P', 12732, 'n_spawn_table_id',         '33',  NULL, NULL, 1),"
			"('1P_CPLab05_P', 12732, 'n_default_spawn_table_id', '33',  NULL, NULL, 1),"
			"('1P_CPLab05_P', 12733, 's_n_task_force',           '2',   NULL, NULL, 1),"
			"('1P_CPLab05_P', 12733, 's_n_team_number',          '2',   NULL, NULL, 1),"
			"('1P_CPLab05_P', 12733, 'n_spawn_table_id',         '33',  NULL, NULL, 1),"
			"('1P_CPLab05_P', 12733, 'n_default_spawn_table_id', '33',  NULL, NULL, 1),"
			"('1P_CPLab05_P', 12734, 's_n_task_force',           '2',   NULL, NULL, 1),"
			"('1P_CPLab05_P', 12734, 's_n_team_number',          '2',   NULL, NULL, 1),"
			"('1P_CPLab05_P', 12734, 'n_spawn_table_id',         '33',  NULL, NULL, 1),"
			"('1P_CPLab05_P', 12734, 'n_default_spawn_table_id', '33',  NULL, NULL, 1),"
			"('1P_CPLab05_P', 12735, 's_n_task_force',           '2',   NULL, NULL, 1),"
			"('1P_CPLab05_P', 12735, 's_n_team_number',          '2',   NULL, NULL, 1),"
			"('1P_CPLab05_P', 12735, 'n_spawn_table_id',         '33',  NULL, NULL, 1),"
			"('1P_CPLab05_P', 12735, 'n_default_spawn_table_id', '33',  NULL, NULL, 1),"
			"('1P_CPLab05_P', 12736, 's_n_task_force',           '2',   NULL, NULL, 1),"
			"('1P_CPLab05_P', 12736, 's_n_team_number',          '2',   NULL, NULL, 1),"
			"('1P_CPLab05_P', 12736, 'n_spawn_table_id',         '33',  NULL, NULL, 1),"
			"('1P_CPLab05_P', 12736, 'n_default_spawn_table_id', '33',  NULL, NULL, 1),"
			"('1P_CPLab05_P', 12737, 's_n_task_force',           '2',   NULL, NULL, 1),"
			"('1P_CPLab05_P', 12737, 's_n_team_number',          '2',   NULL, NULL, 1),"
			"('1P_CPLab05_P', 12737, 'n_spawn_table_id',         '33',  NULL, NULL, 1),"
			"('1P_CPLab05_P', 12737, 'n_default_spawn_table_id', '33',  NULL, NULL, 1),"
			"('1P_CPLab05_P', 12738, 's_n_task_force',           '2',   NULL, NULL, 1),"
			"('1P_CPLab05_P', 12738, 's_n_team_number',          '2',   NULL, NULL, 1),"
			"('1P_CPLab05_P', 12738, 'n_spawn_table_id',         '59',  NULL, NULL, 1),"
			"('1P_CPLab05_P', 12738, 'n_default_spawn_table_id', '59',  NULL, NULL, 1),"
			"('1P_CPLab05_P', 12739, 's_n_task_force',           '2',   NULL, NULL, 1),"
			"('1P_CPLab05_P', 12739, 's_n_team_number',          '2',   NULL, NULL, 1),"
			"('1P_CPLab05_P', 12739, 'n_spawn_table_id',         '59',  NULL, NULL, 1),"
			"('1P_CPLab05_P', 12739, 'n_default_spawn_table_id', '59',  NULL, NULL, 1),"
			"('1P_CPLab05_P', 12740, 's_n_task_force',           '2',   NULL, NULL, 1),"
			"('1P_CPLab05_P', 12740, 's_n_team_number',          '2',   NULL, NULL, 1),"
			"('1P_CPLab05_P', 12740, 'n_spawn_table_id',         '59',  NULL, NULL, 1),"
			"('1P_CPLab05_P', 12740, 'n_default_spawn_table_id', '59',  NULL, NULL, 1),"
			"('1P_CPLab05_P', 12741, 's_n_task_force',           '2',   NULL, NULL, 1),"
			"('1P_CPLab05_P', 12741, 's_n_team_number',          '2',   NULL, NULL, 1),"
			"('1P_CPLab05_P', 12741, 'n_spawn_table_id',         '46',  NULL, NULL, 1),"
			"('1P_CPLab05_P', 12741, 'n_default_spawn_table_id', '46',  NULL, NULL, 1),"
			"('1P_CPLab05_P', 12744, 's_n_task_force',           '2',   NULL, NULL, 1),"
			"('1P_CPLab05_P', 12744, 's_n_team_number',          '2',   NULL, NULL, 1),"
			"('1P_CPLab05_P', 12744, 'n_spawn_table_id',         '59',  NULL, NULL, 1),"
			"('1P_CPLab05_P', 12744, 'n_default_spawn_table_id', '59',  NULL, NULL, 1);";
		result = sqlite3_exec(db, kV62_cplab05, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v62 (1P_CPLab05_P seed): %s\n", err); return; }
		Logger::Log("db", "v62: seeded 100 map_object_config rows for 1P_CPLab05_P (25 factories)\n");
	}

	if (version < 63) {
		const char* kV63_ddr =
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES"
			" ('Raid_DomeCityDefense_P', 13849, 's_n_task_force', '2', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13849, 's_n_team_number', '2', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13849, 'n_spawn_table_id', '166', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13849, 'n_default_spawn_table_id', '166', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13848, 's_n_task_force', '2', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13848, 's_n_team_number', '2', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13848, 'n_spawn_table_id', '166', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13848, 'n_default_spawn_table_id', '166', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13847, 's_n_task_force', '2', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13847, 's_n_team_number', '2', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13847, 'n_spawn_table_id', '166', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13847, 'n_default_spawn_table_id', '166', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13846, 's_n_task_force', '2', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13846, 's_n_team_number', '2', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13846, 'n_spawn_table_id', '166', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13846, 'n_default_spawn_table_id', '166', NULL, NULL, 1),    "


			" ('Raid_DomeCityDefense_P', 13809, 's_n_task_force', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13809, 's_n_team_number', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13809, 'n_spawn_table_id', '102', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13809, 'n_default_spawn_table_id', '102', NULL, NULL, 1),    "

			" ('Raid_DomeCityDefense_P', 13809, 's_n_task_force', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13809, 's_n_team_number', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13809, 'n_spawn_table_id', '102', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13809, 'n_default_spawn_table_id', '102', NULL, NULL, 1),    "

			" ('Raid_DomeCityDefense_P', 13806, 's_n_task_force', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13806, 's_n_team_number', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13806, 'n_spawn_table_id', '102', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13806, 'n_default_spawn_table_id', '102', NULL, NULL, 1),    "

			" ('Raid_DomeCityDefense_P', 13805, 's_n_task_force', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13805, 's_n_team_number', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13805, 'n_spawn_table_id', '102', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13805, 'n_default_spawn_table_id', '102', NULL, NULL, 1),    "

			" ('Raid_DomeCityDefense_P', 13804, 's_n_task_force', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13804, 's_n_team_number', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13804, 'n_spawn_table_id', '102', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13804, 'n_default_spawn_table_id', '102', NULL, NULL, 1),    "

			" ('Raid_DomeCityDefense_P', 13803, 's_n_task_force', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13803, 's_n_team_number', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13803, 'n_spawn_table_id', '102', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13803, 'n_default_spawn_table_id', '102', NULL, NULL, 1),    "

			" ('Raid_DomeCityDefense_P', 13810, 's_n_task_force', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13810, 's_n_team_number', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13810, 'n_spawn_table_id', '102', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13810, 'n_default_spawn_table_id', '102', NULL, NULL, 1),    "

			" ('Raid_DomeCityDefense_P', 13802, 's_n_task_force', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13802, 's_n_team_number', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13802, 'n_spawn_table_id', '102', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13802, 'n_default_spawn_table_id', '102', NULL, NULL, 1),    "

			" ('Raid_DomeCityDefense_P', 13709, 's_n_task_force', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13709, 's_n_team_number', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13709, 'n_spawn_table_id', '102', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13709, 'n_default_spawn_table_id', '102', NULL, NULL, 1),    "

			" ('Raid_DomeCityDefense_P', 13708, 's_n_task_force', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13708, 's_n_team_number', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13708, 'n_spawn_table_id', '86', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13708, 'n_default_spawn_table_id', '86', NULL, NULL, 1),    "

			" ('Raid_DomeCityDefense_P', 13704, 's_n_task_force', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13704, 's_n_team_number', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13704, 'n_spawn_table_id', '86', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13704, 'n_default_spawn_table_id', '86', NULL, NULL, 1),    "

			" ('Raid_DomeCityDefense_P', 13703, 's_n_task_force', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13703, 's_n_team_number', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13703, 'n_spawn_table_id', '149', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13703, 'n_default_spawn_table_id', '149', NULL, NULL, 1),    "

			" ('Raid_DomeCityDefense_P', 13700, 's_n_task_force', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13700, 's_n_team_number', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13700, 'n_spawn_table_id', '102', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13700, 'n_default_spawn_table_id', '102', NULL, NULL, 1),    "

			" ('Raid_DomeCityDefense_P', 13694, 's_n_task_force', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13694, 's_n_team_number', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13694, 'n_spawn_table_id', '102', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13694, 'n_default_spawn_table_id', '102', NULL, NULL, 1),    "

			" ('Raid_DomeCityDefense_P', 13692, 's_n_task_force', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13692, 's_n_team_number', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13692, 'n_spawn_table_id', '147', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13692, 'n_default_spawn_table_id', '147', NULL, NULL, 1),    "

			" ('Raid_DomeCityDefense_P', 13691, 's_n_task_force', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13691, 's_n_team_number', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13691, 'n_spawn_table_id', '87', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13691, 'n_default_spawn_table_id', '87', NULL, NULL, 1),    "

			" ('Raid_DomeCityDefense_P', 13673, 's_n_task_force', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13673, 's_n_team_number', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13673, 'n_spawn_table_id', '149', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13673, 'n_default_spawn_table_id', '149', NULL, NULL, 1),    "

			" ('Raid_DomeCityDefense_P', 13650, 's_n_task_force', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13650, 's_n_team_number', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13650, 'n_spawn_table_id', '148', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13650, 'n_default_spawn_table_id', '148', NULL, NULL, 1),    "

			" ('Raid_DomeCityDefense_P', 13665, 's_n_task_force', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13665, 's_n_team_number', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13665, 'n_spawn_table_id', '102', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13665, 'n_default_spawn_table_id', '102', NULL, NULL, 1),    "

			" ('Raid_DomeCityDefense_P', 13664, 's_n_task_force', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13664, 's_n_team_number', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13664, 'n_spawn_table_id', '99', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13664, 'n_default_spawn_table_id', '99', NULL, NULL, 1),    "

			" ('Raid_DomeCityDefense_P', 13662, 's_n_task_force', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13662, 's_n_team_number', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13662, 'n_spawn_table_id', '149', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13662, 'n_default_spawn_table_id', '149', NULL, NULL, 1),    "

			" ('Raid_DomeCityDefense_P', 13661, 's_n_task_force', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13661, 's_n_team_number', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13661, 'n_spawn_table_id', '86', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13661, 'n_default_spawn_table_id', '86', NULL, NULL, 1),    "

			" ('Raid_DomeCityDefense_P', 13660, 's_n_task_force', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13660, 's_n_team_number', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13660, 'n_spawn_table_id', '99', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13660, 'n_default_spawn_table_id', '99', NULL, NULL, 1),    "

			" ('Raid_DomeCityDefense_P', 13659, 's_n_task_force', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13659, 's_n_team_number', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13659, 'n_spawn_table_id', '102', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13659, 'n_default_spawn_table_id', '102', NULL, NULL, 1),    "

			" ('Raid_DomeCityDefense_P', 13657, 's_n_task_force', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13657, 's_n_team_number', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13657, 'n_spawn_table_id', '87', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13657, 'n_default_spawn_table_id', '87', NULL, NULL, 1),    "

			" ('Raid_DomeCityDefense_P', 13656, 's_n_task_force', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13656, 's_n_team_number', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13656, 'n_spawn_table_id', '99', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13656, 'n_default_spawn_table_id', '99', NULL, NULL, 1),    "

			" ('Raid_DomeCityDefense_P', 13655, 's_n_task_force', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13655, 's_n_team_number', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13655, 'n_spawn_table_id', '99', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13655, 'n_default_spawn_table_id', '99', NULL, NULL, 1),    "

			" ('Raid_DomeCityDefense_P', 13654, 's_n_task_force', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13654, 's_n_team_number', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13654, 'n_spawn_table_id', '99', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13654, 'n_default_spawn_table_id', '99', NULL, NULL, 1),    "

			" ('Raid_DomeCityDefense_P', 13653, 's_n_task_force', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13653, 's_n_team_number', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13653, 'n_spawn_table_id', '149', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13653, 'n_default_spawn_table_id', '149', NULL, NULL, 1),    "

			" ('Raid_DomeCityDefense_P', 13658, 's_n_task_force', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13658, 's_n_team_number', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13658, 'n_spawn_table_id', '87', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13658, 'n_default_spawn_table_id', '87', NULL, NULL, 1),    "

			" ('Raid_DomeCityDefense_P', 13652, 's_n_task_force', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13652, 's_n_team_number', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13652, 'n_spawn_table_id', '148', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13652, 'n_default_spawn_table_id', '148', NULL, NULL, 1),    "

			" ('Raid_DomeCityDefense_P', 13651, 's_n_task_force', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13651, 's_n_team_number', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13651, 'n_spawn_table_id', '99', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13651, 'n_default_spawn_table_id', '99', NULL, NULL, 1),    "

			" ('Raid_DomeCityDefense_P', 13649, 's_n_task_force', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13649, 's_n_team_number', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13649, 'n_spawn_table_id', '99', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13649, 'n_default_spawn_table_id', '99', NULL, NULL, 1),    "

			" ('Raid_DomeCityDefense_P', 13648, 's_n_task_force', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13648, 's_n_team_number', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13648, 'n_spawn_table_id', '99', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13648, 'n_default_spawn_table_id', '99', NULL, NULL, 1),    "

			" ('Raid_DomeCityDefense_P', 13647, 's_n_task_force', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13647, 's_n_team_number', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13647, 'n_spawn_table_id', '87', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13647, 'n_default_spawn_table_id', '87', NULL, NULL, 1),    "

			" ('Raid_DomeCityDefense_P', 13646, 's_n_task_force', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13646, 's_n_team_number', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13646, 'n_spawn_table_id', '99', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13646, 'n_default_spawn_table_id', '99', NULL, NULL, 1),    "

			" ('Raid_DomeCityDefense_P', 13644, 's_n_task_force', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13644, 's_n_team_number', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13644, 'n_spawn_table_id', '86', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13644, 'n_default_spawn_table_id', '86', NULL, NULL, 1),    "

			" ('Raid_DomeCityDefense_P', 13645, 's_n_task_force', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13645, 's_n_team_number', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13645, 'n_spawn_table_id', '99', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13645, 'n_default_spawn_table_id', '99', NULL, NULL, 1),    "

			" ('Raid_DomeCityDefense_P', 13643, 's_n_task_force', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13643, 's_n_team_number', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13643, 'n_spawn_table_id', '148', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13643, 'n_default_spawn_table_id', '148', NULL, NULL, 1),    "

			" ('Raid_DomeCityDefense_P', 13642, 's_n_task_force', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13642, 's_n_team_number', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13642, 'n_spawn_table_id', '86', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13642, 'n_default_spawn_table_id', '86', NULL, NULL, 1),    "

			" ('Raid_DomeCityDefense_P', 13641, 's_n_task_force', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13641, 's_n_team_number', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13641, 'n_spawn_table_id', '99', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13641, 'n_default_spawn_table_id', '99', NULL, NULL, 1),    "

			" ('Raid_DomeCityDefense_P', 13640, 's_n_task_force', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13640, 's_n_team_number', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13640, 'n_spawn_table_id', '148', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13640, 'n_default_spawn_table_id', '148', NULL, NULL, 1),    "

			" ('Raid_DomeCityDefense_P', 13639, 's_n_task_force', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13639, 's_n_team_number', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13639, 'n_spawn_table_id', '147', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13639, 'n_default_spawn_table_id', '147', NULL, NULL, 1),    "

			" ('Raid_DomeCityDefense_P', 13637, 's_n_task_force', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13637, 's_n_team_number', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13637, 'n_spawn_table_id', '148', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13637, 'n_default_spawn_table_id', '148', NULL, NULL, 1),    "

			" ('Raid_DomeCityDefense_P', 13636, 's_n_task_force', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13636, 's_n_team_number', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13636, 'n_spawn_table_id', '99', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13636, 'n_default_spawn_table_id', '99', NULL, NULL, 1),    "

			" ('Raid_DomeCityDefense_P', 13635, 's_n_task_force', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13635, 's_n_team_number', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13635, 'n_spawn_table_id', '102', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13635, 'n_default_spawn_table_id', '102', NULL, NULL, 1),    "

			" ('Raid_DomeCityDefense_P', 13634, 's_n_task_force', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13634, 's_n_team_number', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13634, 'n_spawn_table_id', '148', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13634, 'n_default_spawn_table_id', '148', NULL, NULL, 1),    "

			" ('Raid_DomeCityDefense_P', 13633, 's_n_task_force', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13633, 's_n_team_number', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13633, 'n_spawn_table_id', '99', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13633, 'n_default_spawn_table_id', '99', NULL, NULL, 1),    "

			" ('Raid_DomeCityDefense_P', 13629, 's_n_task_force', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13629, 's_n_team_number', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13629, 'n_spawn_table_id', '99', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13629, 'n_default_spawn_table_id', '99', NULL, NULL, 1),    "

			" ('Raid_DomeCityDefense_P', 13638, 's_n_task_force', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13638, 's_n_team_number', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13638, 'n_spawn_table_id', '148', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13638, 'n_default_spawn_table_id', '148', NULL, NULL, 1),    "

			" ('Raid_DomeCityDefense_P', 13630, 's_n_task_force', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13630, 's_n_team_number', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13630, 'n_spawn_table_id', '148', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13630, 'n_default_spawn_table_id', '148', NULL, NULL, 1),    "

			" ('Raid_DomeCityDefense_P', 13632, 's_n_task_force', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13632, 's_n_team_number', '1', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13632, 'n_spawn_table_id', '86', NULL, NULL, 1),    "
			" ('Raid_DomeCityDefense_P', 13632, 'n_default_spawn_table_id', '86', NULL, NULL, 1);    ";

		bool has_ddr_map_config = false;
		sqlite3_stmt* ddr_probe = nullptr;
		if (sqlite3_prepare_v2(db,
				"SELECT 1 FROM map_object_config WHERE map_name = 'Raid_DomeCityDefense_P' LIMIT 1",
				-1, &ddr_probe, nullptr) == SQLITE_OK && ddr_probe) {
			has_ddr_map_config = sqlite3_step(ddr_probe) == SQLITE_ROW;
			sqlite3_finalize(ddr_probe);
		}
		if (!has_ddr_map_config) {
			result = sqlite3_exec(db, kV63_ddr, nullptr, nullptr, &err);
			if (result != SQLITE_OK) { Logger::Log("db", "Failed v63: %s\n", err); return; }
		}
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
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v63 (ddr map_object_config dedupe): %s\n", err); return; }

		// Seed the matching map pool for the ddr queue (post-pool-inversion).
		// queue_id 3 is assumed for ddr (specops=1, pvp=2 already seeded).
		const char* q_pool = "INSERT OR IGNORE INTO ga_map_pools (map_pool_id, name) VALUES (3, 'ddr');";
		result = sqlite3_exec(db, q_pool, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v63 (ddr map pool): %s\n", err); return; }

		const char* q1 =
			"UPDATE ga_queues SET queue_id = 3 "
			"WHERE queue_id = (SELECT MIN(queue_id) FROM ga_queues WHERE name = 'ddr' "
			"AND COALESCE(rule_class, '') = '' AND taskforce_policy = 'pinned_2' "
			"AND continue_in_queue = 0 AND enabled = 1 AND queue_type_value_id = 1454 "
			"AND status_msg_id = 0 AND name_msg_id = 62550 AND desc_msg_id = 64938 "
			"AND icon_id = 1714 AND max_players_per_side = 10 AND min_players_per_team = 1 "
			"AND max_players_per_team = 10 AND level_min = 5 AND level_max = 50 "
			"AND tab = 231 AND map_x = 0 AND map_y = 0 AND map_active_flag = 1 "
			"AND map_icon_texture_res_id = 5126 AND video_res_id = 0 "
			"AND location_value_id = 0 AND double_agent_flag = 1 AND sys_site_id = 0 "
			"AND sort_order = 0 AND bonus_queue_flag = 1 AND difficulty_value_id = 1471 "
			"AND access_flags = 0 AND active_flag = 1 AND locked_flag = 0 "
			"AND COALESCE(map_pool_id, 0) = 3) "
			"AND NOT EXISTS (SELECT 1 FROM ga_queues WHERE queue_id = 3);"
			"DELETE FROM ga_queues WHERE name = 'ddr' "
			"AND COALESCE(rule_class, '') = '' AND taskforce_policy = 'pinned_2' "
			"AND continue_in_queue = 0 AND enabled = 1 AND queue_type_value_id = 1454 "
			"AND status_msg_id = 0 AND name_msg_id = 62550 AND desc_msg_id = 64938 "
			"AND icon_id = 1714 AND max_players_per_side = 10 AND min_players_per_team = 1 "
			"AND max_players_per_team = 10 AND level_min = 5 AND level_max = 50 "
			"AND tab = 231 AND map_x = 0 AND map_y = 0 AND map_active_flag = 1 "
			"AND map_icon_texture_res_id = 5126 AND video_res_id = 0 "
			"AND location_value_id = 0 AND double_agent_flag = 1 AND sys_site_id = 0 "
			"AND sort_order = 0 AND bonus_queue_flag = 1 AND difficulty_value_id = 1471 "
			"AND access_flags = 0 AND active_flag = 1 AND locked_flag = 0 "
			"AND COALESCE(map_pool_id, 0) = 3 "
			"AND queue_id <> COALESCE((SELECT queue_id FROM ga_queues WHERE queue_id = 3 "
			"AND name = 'ddr' AND COALESCE(rule_class, '') = '' AND taskforce_policy = 'pinned_2' "
			"AND continue_in_queue = 0 AND enabled = 1 AND queue_type_value_id = 1454 "
			"AND status_msg_id = 0 AND name_msg_id = 62550 AND desc_msg_id = 64938 "
			"AND icon_id = 1714 AND max_players_per_side = 10 AND min_players_per_team = 1 "
			"AND max_players_per_team = 10 AND level_min = 5 AND level_max = 50 "
			"AND tab = 231 AND map_x = 0 AND map_y = 0 AND map_active_flag = 1 "
			"AND map_icon_texture_res_id = 5126 AND video_res_id = 0 "
			"AND location_value_id = 0 AND double_agent_flag = 1 AND sys_site_id = 0 "
			"AND sort_order = 0 AND bonus_queue_flag = 1 AND difficulty_value_id = 1471 "
			"AND access_flags = 0 AND active_flag = 1 AND locked_flag = 0 "
			"AND COALESCE(map_pool_id, 0) = 3), "
			"(SELECT MIN(queue_id) FROM ga_queues WHERE name = 'ddr' "
			"AND COALESCE(rule_class, '') = '' AND taskforce_policy = 'pinned_2' "
			"AND continue_in_queue = 0 AND enabled = 1 AND queue_type_value_id = 1454 "
			"AND status_msg_id = 0 AND name_msg_id = 62550 AND desc_msg_id = 64938 "
			"AND icon_id = 1714 AND max_players_per_side = 10 AND min_players_per_team = 1 "
			"AND max_players_per_team = 10 AND level_min = 5 AND level_max = 50 "
			"AND tab = 231 AND map_x = 0 AND map_y = 0 AND map_active_flag = 1 "
			"AND map_icon_texture_res_id = 5126 AND video_res_id = 0 "
			"AND location_value_id = 0 AND double_agent_flag = 1 AND sys_site_id = 0 "
			"AND sort_order = 0 AND bonus_queue_flag = 1 AND difficulty_value_id = 1471 "
			"AND access_flags = 0 AND active_flag = 1 AND locked_flag = 0 "
			"AND COALESCE(map_pool_id, 0) = 3));"
			"INSERT OR IGNORE INTO ga_queues (queue_id, name, taskforce_policy, continue_in_queue, enabled, queue_type_value_id, status_msg_id, name_msg_id, desc_msg_id, icon_id, max_players_per_side, min_players_per_team, max_players_per_team, level_min, level_max, tab, map_x, map_y, map_active_flag, map_icon_texture_res_id, video_res_id, location_value_id, double_agent_flag, sys_site_id, sort_order, bonus_queue_flag, difficulty_value_id, access_flags, active_flag, locked_flag, map_pool_id) VALUES (3, 'ddr', 'pinned_2', 0, 1, 1454, 0, 62550, 64938, 1714, 10, 1, 10, 5, 50, 231, 0, 0, 1, 5126, 0, 0, 1, 0, 0, 1, 1471, 0, 1, 0, 3);"
			"";
		result = sqlite3_exec(db, q1, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v63: %s\n", err); return; }

		const char* q2  = "INSERT OR IGNORE INTO ga_map_pool_entries (map_pool_id, map_name, game_mode, weight, enabled) VALUES (3, 'Raid_DomeCityDefense_P', 'TgGame.TgGame_Defense', 1, 1);";
		result = sqlite3_exec(db, q2, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v63: %s\n", err); return; }

		const char* q3 = "INSERT OR REPLACE INTO map_game_info (map_game_id, map_name, game_class, gameplay_type_value_id, friendly_name_msg_id, entry_background_image_res_id) VALUES (100004, 'Raid_DomeCityDefense_P', 'TgGame.TgGame_Defense', 1550, 60534, 7420);";
		result = sqlite3_exec(db, q3, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v63: %s\n", err); return; }
	}
	if (version < 64) {

		const char* q4 = "INSERT OR REPLACE INTO map_game_info (map_game_id, map_name, game_class, gameplay_type_value_id, friendly_name_msg_id, entry_background_image_res_id) VALUES (100005, 'Dome3_VR_Arena_P', 'TgGame.TgGame_Mission', 1542, 22845, 4901);";
		result = sqlite3_exec(db, q4, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v63: %s\n", err); return; }

		Logger::Log("db", "v63: seeded map_object_config for ddr\n");
	}
	if (version < 65) {
		const char* kV65_cpmine05 =
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES"
			" ('1P_CPMine05_P', 12512, 's_n_team_number',            '1',  NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12512, 's_n_task_force',             '1',  NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12514, 'm_n_task_force',             '1',  NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12581, 's_n_task_force',             '1',  NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12581, 's_n_team_number',            '1',  NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12450, 'm_n_task_force',             '2',  NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12573, 'n_default_spawn_table_id',   '29', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12573, 'n_spawn_table_id',           '29', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12574, 'n_default_spawn_table_id',   '28', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12574, 'n_spawn_table_id',           '28', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12575, 'n_spawn_table_id',           '28', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12575, 'n_default_spawn_table_id',   '28', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12576, 'n_default_spawn_table_id',   '28', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12576, 'n_spawn_table_id',           '28', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12577, 'n_default_spawn_table_id',   '28', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12577, 'n_spawn_table_id',           '28', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12578, 'n_spawn_table_id',           '51', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12578, 'n_default_spawn_table_id',   '51', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12579, 'n_spawn_table_id',           '40', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12579, 'n_default_spawn_table_id',   '40', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12580, 'n_default_spawn_table_id',   '45', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12580, 'n_spawn_table_id',           '45', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12583, 'n_spawn_table_id',           '33', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12583, 'n_default_spawn_table_id',   '33', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12584, 'n_default_spawn_table_id',   '33', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12584, 'n_spawn_table_id',           '33', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12585, 'n_default_spawn_table_id',   '33', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12585, 'n_spawn_table_id',           '33', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12586, 'n_default_spawn_table_id',   '40', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12586, 'n_spawn_table_id',           '40', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12587, 'n_default_spawn_table_id',   '45', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12587, 'n_spawn_table_id',           '45', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12524, 'n_default_spawn_table_id',   '46', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12524, 'n_spawn_table_id',           '46', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12525, 'n_default_spawn_table_id',   '59', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12525, 'n_spawn_table_id',           '59', NULL, NULL, 1);";
		result = sqlite3_exec(db, kV65_cpmine05, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v65 (map_object_config cpmine05): %s\n", err); return; }

		// Add the map to the specops pool (map_pool_id=1 — seeded in
		// ControlServer/Database/Database.cpp alongside 1P_CPLab05_P / 1P_CPLab03).
		const char* kV65_specops_pool =
			"INSERT OR IGNORE INTO ga_map_pool_entries (map_pool_id, map_name, game_mode) VALUES"
			" (1, '1P_CPMine05_P', 'TgGame.TgGame_Mission');";
		result = sqlite3_exec(db, kV65_specops_pool, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v65 (specops pool): %s\n", err); return; }

		Logger::Log("db", "v65: seeded map_object_config + specops pool entry for 1P_CPMine05_P\n");
	}
	if (version < 66) {
		// v65 set spawn_table_id on the 1P_CPMine05_P bot factories but
		// missed s_n_task_force / s_n_team_number, so the factories spawned
		// nothing (1P_CPLab05_P has all four columns per factory). All 15
		// factories belong to task force 2.
		const char* kV66_cpmine05_taskforce =
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES"
			" ('1P_CPMine05_P', 12524, 's_n_task_force',  '2', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12524, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12525, 's_n_task_force',  '2', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12525, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12573, 's_n_task_force',  '2', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12573, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12574, 's_n_task_force',  '2', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12574, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12575, 's_n_task_force',  '2', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12575, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12576, 's_n_task_force',  '2', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12576, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12577, 's_n_task_force',  '2', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12577, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12578, 's_n_task_force',  '2', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12578, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12579, 's_n_task_force',  '2', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12579, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12580, 's_n_task_force',  '2', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12580, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12583, 's_n_task_force',  '2', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12583, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12584, 's_n_task_force',  '2', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12584, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12585, 's_n_task_force',  '2', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12585, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12586, 's_n_task_force',  '2', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12586, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12587, 's_n_task_force',  '2', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12587, 's_n_team_number', '2', NULL, NULL, 1);";
		result = sqlite3_exec(db, kV66_cpmine05_taskforce, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v66 (cpmine05 task force): %s\n", err); return; }

		Logger::Log("db", "v66: backfilled s_n_task_force / s_n_team_number on 1P_CPMine05_P bot factories\n");
	}
	if (version < 67) {
		// SpawnBotById gate (TgGame__SpawnBotById.cpp:484): rejects every
		// spawn whose pFactory->nPriority is 0. CPMine05's bot factories ship
		// with n_priority=0 in the editor data (CPLab05's all ship with 1),
		// so without this override every spawn returned nullptr and nothing
		// appeared. Match CPLab05 — n_priority=1 across all 15 factories.
		const char* kV67_cpmine05_priority =
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES"
			" ('1P_CPMine05_P', 12524, 'n_priority', '1', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12525, 'n_priority', '1', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12573, 'n_priority', '1', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12574, 'n_priority', '1', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12575, 'n_priority', '1', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12576, 'n_priority', '1', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12577, 'n_priority', '1', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12578, 'n_priority', '1', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12579, 'n_priority', '1', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12580, 'n_priority', '1', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12583, 'n_priority', '1', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12584, 'n_priority', '1', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12585, 'n_priority', '1', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12586, 'n_priority', '1', NULL, NULL, 1),"
			" ('1P_CPMine05_P', 12587, 'n_priority', '1', NULL, NULL, 1);";
		result = sqlite3_exec(db, kV67_cpmine05_priority, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v67 (cpmine05 priority): %s\n", err); return; }

		Logger::Log("db", "v67: backfilled n_priority=1 on 1P_CPMine05_P bot factories\n");
	}
	if (version < 68) {
		// map_planner output for 1P_CPMine04_P (2026-05-28).
		// Mirrors the CPMine05 setup from v65/v66/v67 but bundled: bot-factory
		// task_force + team_number + spawn_table_id + priority, plus the
		// objective task_force pair, all in one INSERT.
		const char* kV68_cpmine04 =
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES"
			" ('1P_CPMine04_P', 12514, 'm_n_task_force',           '1',  NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12450, 'm_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12512, 's_n_team_number',          '1',  NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12512, 's_n_task_force',           '1',  NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12508, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12508, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12509, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12509, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12510, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12510, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12511, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12511, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12515, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12515, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12516, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12516, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12517, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12517, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12518, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12518, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12519, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12519, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12520, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12520, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12521, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12521, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12522, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12522, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12524, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12524, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12525, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12525, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12542, 's_n_team_number',          '1',  NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12542, 's_n_task_force',           '1',  NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12508, 'n_spawn_table_id',         '29', NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12508, 'n_default_spawn_table_id', '29', NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12509, 'n_default_spawn_table_id', '28', NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12509, 'n_spawn_table_id',         '28', NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12510, 'n_spawn_table_id',         '28', NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12510, 'n_default_spawn_table_id', '28', NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12511, 'n_spawn_table_id',         '28', NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12511, 'n_default_spawn_table_id', '28', NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12515, 'n_default_spawn_table_id', '28', NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12515, 'n_spawn_table_id',         '28', NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12516, 'n_spawn_table_id',         '40', NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12516, 'n_default_spawn_table_id', '40', NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12517, 'n_default_spawn_table_id', '40', NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12517, 'n_spawn_table_id',         '40', NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12518, 'n_spawn_table_id',         '33', NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12518, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12519, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12519, 'n_spawn_table_id',         '33', NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12520, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12520, 'n_spawn_table_id',         '33', NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12521, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12521, 'n_spawn_table_id',         '33', NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12522, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12522, 'n_spawn_table_id',         '33', NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12524, 'n_default_spawn_table_id', '46', NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12524, 'n_spawn_table_id',         '46', NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12525, 'n_default_spawn_table_id', '59', NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12525, 'n_spawn_table_id',         '59', NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12508, 'n_priority',               '1',  NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12509, 'n_priority',               '1',  NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12510, 'n_priority',               '1',  NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12511, 'n_priority',               '1',  NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12515, 'n_priority',               '1',  NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12516, 'n_priority',               '1',  NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12517, 'n_priority',               '1',  NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12518, 'n_priority',               '1',  NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12519, 'n_priority',               '1',  NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12520, 'n_priority',               '1',  NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12521, 'n_priority',               '1',  NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12522, 'n_priority',               '1',  NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12524, 'n_priority',               '1',  NULL, NULL, 1),"
			" ('1P_CPMine04_P', 12525, 'n_priority',               '1',  NULL, NULL, 1);";
		result = sqlite3_exec(db, kV68_cpmine04, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v68 (map_object_config cpmine04): %s\n", err); return; }

		// Add the map to the specops pool (map_pool_id=1 — see v65 for context).
		const char* kV68_specops_pool =
			"INSERT OR IGNORE INTO ga_map_pool_entries (map_pool_id, map_name, game_mode) VALUES"
			" (1, '1P_CPMine04_P', 'TgGame.TgGame_Mission');";
		result = sqlite3_exec(db, kV68_specops_pool, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v68 (specops pool): %s\n", err); return; }

		Logger::Log("db", "v68: seeded map_object_config + specops pool entry for 1P_CPMine04_P\n");
	}
	if (version < 69) {
		// 1P_CPFactory01_P was already pre-configured (map_object_config seeded
		// back in v60) but never added to the specops pool. Add it now.
		const char* kV69_specops_pool =
			"INSERT OR IGNORE INTO ga_map_pool_entries (map_pool_id, map_name, game_mode) VALUES"
			" (1, '1P_CPFactory01_P', 'TgGame.TgGame_Mission');";
		result = sqlite3_exec(db, kV69_specops_pool, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v69 (specops pool): %s\n", err); return; }

		Logger::Log("db", "v69: added 1P_CPFactory01_P to specops pool\n");
	}
	if (version < 70) {
		// map_planner output for 1P_CPLab03 (2026-05-28). Map is already in the
		// specops pool (seeded by ControlServer); this migration only replaces
		// the legacy task-force/spawn config with the planner-generated rows.
		const char* kV70_cplab03 =
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES"
			" ('1P_CPLab03', 12505, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPLab03', 12505, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPLab03', 12504, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPLab03', 12504, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPLab03', 12503, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPLab03', 12503, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPLab03', 12502, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPLab03', 12502, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPLab03', 12501, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPLab03', 12501, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPLab03', 12500, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPLab03', 12500, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPLab03', 12499, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPLab03', 12499, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPLab03', 12498, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPLab03', 12498, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPLab03', 12497, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPLab03', 12497, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPLab03', 12496, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPLab03', 12496, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPLab03', 12495, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPLab03', 12495, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPLab03', 12494, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPLab03', 12494, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPLab03', 12493, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPLab03', 12493, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPLab03', 12492, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPLab03', 12492, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPLab03', 12491, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPLab03', 12491, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPLab03', 12490, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPLab03', 12490, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPLab03', 12489, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPLab03', 12489, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPLab03', 12488, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPLab03', 12488, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPLab03', 12484, 'm_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPLab03', 12481, 'm_n_task_force',           '1',  NULL, NULL, 1),"
			" ('1P_CPLab03', 12541, 's_n_task_force',           '1',  NULL, NULL, 1),"
			" ('1P_CPLab03', 12541, 's_n_team_number',          '1',  NULL, NULL, 1),"
			" ('1P_CPLab03', 12480, 's_n_task_force',           '1',  NULL, NULL, 1),"
			" ('1P_CPLab03', 12480, 's_n_team_number',          '1',  NULL, NULL, 1),"
			" ('1P_CPLab03', 12488, 'n_priority',               '1',  NULL, NULL, 1),"
			" ('1P_CPLab03', 12489, 'n_priority',               '1',  NULL, NULL, 1),"
			" ('1P_CPLab03', 12490, 'n_priority',               '1',  NULL, NULL, 1),"
			" ('1P_CPLab03', 12491, 'n_priority',               '1',  NULL, NULL, 1),"
			" ('1P_CPLab03', 12492, 'n_priority',               '1',  NULL, NULL, 1),"
			" ('1P_CPLab03', 12493, 'n_priority',               '1',  NULL, NULL, 1),"
			" ('1P_CPLab03', 12494, 'n_priority',               '1',  NULL, NULL, 1),"
			" ('1P_CPLab03', 12495, 'n_priority',               '1',  NULL, NULL, 1),"
			" ('1P_CPLab03', 12496, 'n_priority',               '1',  NULL, NULL, 1),"
			" ('1P_CPLab03', 12497, 'n_priority',               '1',  NULL, NULL, 1),"
			" ('1P_CPLab03', 12498, 'n_priority',               '1',  NULL, NULL, 1),"
			" ('1P_CPLab03', 12499, 'n_priority',               '1',  NULL, NULL, 1),"
			" ('1P_CPLab03', 12500, 'n_priority',               '1',  NULL, NULL, 1),"
			" ('1P_CPLab03', 12501, 'n_priority',               '1',  NULL, NULL, 1),"
			" ('1P_CPLab03', 12502, 'n_priority',               '1',  NULL, NULL, 1),"
			" ('1P_CPLab03', 12503, 'n_priority',               '1',  NULL, NULL, 1),"
			" ('1P_CPLab03', 12504, 'n_priority',               '1',  NULL, NULL, 1),"
			" ('1P_CPLab03', 12505, 'n_priority',               '1',  NULL, NULL, 1),"
			" ('1P_CPLab03', 12488, 'n_default_spawn_table_id', '29', NULL, NULL, 1),"
			" ('1P_CPLab03', 12488, 'n_spawn_table_id',         '29', NULL, NULL, 1),"
			" ('1P_CPLab03', 12489, 'n_spawn_table_id',         '28', NULL, NULL, 1),"
			" ('1P_CPLab03', 12489, 'n_default_spawn_table_id', '28', NULL, NULL, 1),"
			" ('1P_CPLab03', 12490, 'n_default_spawn_table_id', '28', NULL, NULL, 1),"
			" ('1P_CPLab03', 12490, 'n_spawn_table_id',         '28', NULL, NULL, 1),"
			" ('1P_CPLab03', 12491, 'n_spawn_table_id',         '28', NULL, NULL, 1),"
			" ('1P_CPLab03', 12491, 'n_default_spawn_table_id', '28', NULL, NULL, 1),"
			" ('1P_CPLab03', 12492, 'n_default_spawn_table_id', '28', NULL, NULL, 1),"
			" ('1P_CPLab03', 12492, 'n_spawn_table_id',         '28', NULL, NULL, 1),"
			" ('1P_CPLab03', 12493, 'n_default_spawn_table_id', '46', NULL, NULL, 1),"
			" ('1P_CPLab03', 12493, 'n_spawn_table_id',         '46', NULL, NULL, 1),"
			" ('1P_CPLab03', 12494, 'n_default_spawn_table_id', '65', NULL, NULL, 1),"
			" ('1P_CPLab03', 12494, 'n_spawn_table_id',         '65', NULL, NULL, 1),"
			" ('1P_CPLab03', 12495, 'n_default_spawn_table_id', '34', NULL, NULL, 1),"
			" ('1P_CPLab03', 12495, 'n_spawn_table_id',         '34', NULL, NULL, 1),"
			" ('1P_CPLab03', 12496, 'n_default_spawn_table_id', '34', NULL, NULL, 1),"
			" ('1P_CPLab03', 12496, 'n_spawn_table_id',         '34', NULL, NULL, 1),"
			" ('1P_CPLab03', 12497, 'n_spawn_table_id',         '33', NULL, NULL, 1),"
			" ('1P_CPLab03', 12497, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			" ('1P_CPLab03', 12498, 'n_spawn_table_id',         '33', NULL, NULL, 1),"
			" ('1P_CPLab03', 12498, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			" ('1P_CPLab03', 12499, 'n_spawn_table_id',         '33', NULL, NULL, 1),"
			" ('1P_CPLab03', 12499, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			" ('1P_CPLab03', 12500, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			" ('1P_CPLab03', 12500, 'n_spawn_table_id',         '33', NULL, NULL, 1),"
			" ('1P_CPLab03', 12501, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			" ('1P_CPLab03', 12501, 'n_spawn_table_id',         '33', NULL, NULL, 1),"
			" ('1P_CPLab03', 12502, 'n_spawn_table_id',         '33', NULL, NULL, 1),"
			" ('1P_CPLab03', 12502, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			" ('1P_CPLab03', 12503, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			" ('1P_CPLab03', 12503, 'n_spawn_table_id',         '33', NULL, NULL, 1),"
			" ('1P_CPLab03', 12504, 'n_spawn_table_id',         '33', NULL, NULL, 1),"
			" ('1P_CPLab03', 12504, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			" ('1P_CPLab03', 12505, 'n_spawn_table_id',         '33', NULL, NULL, 1),"
			" ('1P_CPLab03', 12505, 'n_default_spawn_table_id', '33', NULL, NULL, 1);";
		result = sqlite3_exec(db, kV70_cplab03, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v70 (map_object_config cplab03): %s\n", err); return; }

		Logger::Log("db", "v70: seeded map_object_config for 1P_CPLab03\n");
	}
	if (version < 71) {
		// Retune four CPMine05 factories to spawn_table 40.
		// 12580 and 12587 were 45 in v65; 12579 and 12586 were already 40
		// (no-op) — UPDATE covers all four for clarity.
		const char* kV71_cpmine05_spawn = "UPDATE map_object_config "
			"SET value = '40' "
			"WHERE map_name = '1P_CPMine05_P' "
			"  AND column_name IN ('n_spawn_table_id', 'n_default_spawn_table_id') "
			"  AND map_object_id IN (12587, 12586, 12580, 12579);";
		result = sqlite3_exec(db, kV71_cpmine05_spawn, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v71 (cpmine05 spawn retune): %s\n", err); return; }

		Logger::Log("db", "v71: retuned CPMine05 factories 12579/12580/12586/12587 to spawn_table 40\n");
	}
	if (version < 72) {
		// map_planner output for 1P_CPFactory05_P (2026-05-28).
		// Bundled task_force + team_number + spawn_table_id, plus the objective
		// task_force pair, plus the specops pool entry — same shape as v68.
		const char* kV72_cpfactory05 =
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES"
			" ('1P_CPFactory05_P', 12771, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12771, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12770, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12770, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12769, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12769, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12768, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12768, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12767, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12767, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12766, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12766, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12765, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12765, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12764, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12764, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12568, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12568, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12561, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12561, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12560, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12560, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12559, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12559, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12558, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12558, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12286, 's_n_task_force',           '1',  NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12286, 's_n_team_number',          '1',  NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 11716, 's_n_team_number',          '1',  NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 11716, 's_n_task_force',           '1',  NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12557, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12557, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12556, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12556, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12555, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12555, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12106, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12106, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12105, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12105, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12102, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12102, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 11717, 'm_n_task_force',           '1',  NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12553, 'm_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12102, 'n_default_spawn_table_id', '29', NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12102, 'n_spawn_table_id',         '29', NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12105, 'n_spawn_table_id',         '40', NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12105, 'n_default_spawn_table_id', '40', NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12106, 'n_spawn_table_id',         '33', NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12106, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12555, 'n_default_spawn_table_id', '28', NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12555, 'n_spawn_table_id',         '28', NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12556, 'n_default_spawn_table_id', '28', NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12556, 'n_spawn_table_id',         '28', NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12557, 'n_default_spawn_table_id', '28', NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12557, 'n_spawn_table_id',         '28', NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12558, 'n_default_spawn_table_id', '28', NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12558, 'n_spawn_table_id',         '28', NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12559, 'n_spawn_table_id',         '59', NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12559, 'n_default_spawn_table_id', '59', NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12560, 'n_default_spawn_table_id', '59', NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12560, 'n_spawn_table_id',         '59', NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12561, 'n_spawn_table_id',         '46', NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12561, 'n_default_spawn_table_id', '46', NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12568, 'n_default_spawn_table_id', '40', NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12568, 'n_spawn_table_id',         '40', NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12764, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12764, 'n_spawn_table_id',         '33', NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12765, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12765, 'n_spawn_table_id',         '33', NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12766, 'n_spawn_table_id',         '33', NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12766, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12767, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12767, 'n_spawn_table_id',         '33', NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12768, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12768, 'n_spawn_table_id',         '33', NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12769, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12769, 'n_spawn_table_id',         '33', NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12770, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12770, 'n_spawn_table_id',         '33', NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12771, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			" ('1P_CPFactory05_P', 12771, 'n_spawn_table_id',         '33', NULL, NULL, 1);";
		result = sqlite3_exec(db, kV72_cpfactory05, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v72 (map_object_config cpfactory05): %s\n", err); return; }

		// Add the map to the specops pool (map_pool_id=1 — see v65 for context).
		const char* kV72_specops_pool =
			"INSERT OR IGNORE INTO ga_map_pool_entries (map_pool_id, map_name, game_mode) VALUES"
			" (1, '1P_CPFactory05_P', 'TgGame.TgGame_Mission');";
		result = sqlite3_exec(db, kV72_specops_pool, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v72 (specops pool): %s\n", err); return; }

		Logger::Log("db", "v72: seeded map_object_config + specops pool entry for 1P_CPFactory05_P\n");
	}
	if (version < 73) {
		// map_game_info for the 6 specops pool maps so the loading screen +
		// friendly name resolve correctly (currently they fall back because
		// these map_game_ids weren't seeded by v51's PvP-only pass).
		//
		// gameplay_type_value_id 1553 = PVE- Team Mission (asm_data_set_valid_values
		// group 171). game_class matches the pool seed in
		// ControlServer/Database/Database.cpp. friendly_name_msg_id from
		// asm_data_set_msg_translations (exact "1P_CP*" labels).
		// entry_background_image_res_id is the dedicated HUD_MissionLoads.PvE_CP
		// loading screen res_id (one per map).
		const char* kV73_specops_map_game_info =
			"INSERT OR REPLACE INTO map_game_info (map_game_id, map_name, game_class, gameplay_type_value_id, friendly_name_msg_id, entry_background_image_res_id) VALUES "
			"(1296, '1P_CPFactory01_P', 'TgGame.TgGame_Mission', 1553, 36871, 6511),"  // 1P_CPFactory01 -> HUD_MissionLoads.PvE_CP.1P_CPFactory01_P
			"(1319, '1P_CPLab03',       'TgGame.TgGame_Mission', 1553, 38027, 6518),"  // 1P_CPLab03     -> HUD_MissionLoads.PvE_CP.1P_CPLab03_P
			"(1322, '1P_CPMine04_P',    'TgGame.TgGame_Mission', 1553, 38153, 6524),"  // 1P_CPMine04    -> HUD_MissionLoads.PvE_CP.1P_CPMine04_P
			"(1323, '1P_CPFactory05_P', 'TgGame.TgGame_Mission', 1553, 39504, 6515),"  // 1P_CPFactory05 -> HUD_MissionLoads.PvE_CP.1P_CPFactory05_P
			"(1327, '1P_CPMine05_P',    'TgGame.TgGame_Mission', 1553, 39053, 6525),"  // 1P_CPMine05    -> HUD_MissionLoads.PvE_CP.1P_CPMine05_P
			"(1336, '1P_CPLab05_P',     'TgGame.TgGame_Mission', 1553, 39870, 6520);"; // 1P_CPLab05     -> HUD_MissionLoads.PvE_CP.1P_CPLab05_P
		result = sqlite3_exec(db, kV73_specops_map_game_info, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v73 (specops map_game_info seed): %s\n", err); return; }

		Logger::Log("db", "v73: seeded map_game_info for 6 specops maps\n");
	}
	if (version < 74) {
		// map_planner output for 1P_SDColony01_P (2026-05-29). Same bundled
		// shape as v68/v70/v72: bot-factory + objective task_force/team_number
		// pairs and the spawn_table_id / default_spawn_table_id pair per
		// factory, plus the specops pool entry.
		const char* kV74_sdcolony01 =
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES"
			" ('1P_SDColony01_P', 12456, 's_n_task_force',           '1',   NULL, NULL, 1),"
			" ('1P_SDColony01_P', 12456, 's_n_team_number',          '1',   NULL, NULL, 1),"
			" ('1P_SDColony01_P', 13285, 'm_n_task_force',           '1',   NULL, NULL, 1),"
			" ('1P_SDColony01_P', 13440, 's_n_task_force',           '1',   NULL, NULL, 1),"
			" ('1P_SDColony01_P', 13440, 's_n_team_number',          '1',   NULL, NULL, 1),"
			" ('1P_SDColony01_P', 13394, 's_n_task_force',           '2',   NULL, NULL, 1),"
			" ('1P_SDColony01_P', 13394, 's_n_team_number',          '2',   NULL, NULL, 1),"
			" ('1P_SDColony01_P', 13416, 's_n_task_force',           '2',   NULL, NULL, 1),"
			" ('1P_SDColony01_P', 13416, 's_n_team_number',          '2',   NULL, NULL, 1),"
			" ('1P_SDColony01_P', 13419, 's_n_task_force',           '2',   NULL, NULL, 1),"
			" ('1P_SDColony01_P', 13419, 's_n_team_number',          '2',   NULL, NULL, 1),"
			" ('1P_SDColony01_P', 13431, 's_n_task_force',           '2',   NULL, NULL, 1),"
			" ('1P_SDColony01_P', 13431, 's_n_team_number',          '2',   NULL, NULL, 1),"
			" ('1P_SDColony01_P', 13432, 's_n_task_force',           '2',   NULL, NULL, 1),"
			" ('1P_SDColony01_P', 13432, 's_n_team_number',          '2',   NULL, NULL, 1),"
			" ('1P_SDColony01_P', 13434, 's_n_task_force',           '2',   NULL, NULL, 1),"
			" ('1P_SDColony01_P', 13434, 's_n_team_number',          '2',   NULL, NULL, 1),"
			" ('1P_SDColony01_P', 13394, 'n_spawn_table_id',         '100', NULL, NULL, 1),"
			" ('1P_SDColony01_P', 13394, 'n_default_spawn_table_id', '100', NULL, NULL, 1),"
			" ('1P_SDColony01_P', 13416, 'n_default_spawn_table_id', '167', NULL, NULL, 1),"
			" ('1P_SDColony01_P', 13416, 'n_spawn_table_id',         '167', NULL, NULL, 1),"
			" ('1P_SDColony01_P', 13419, 'n_spawn_table_id',         '147', NULL, NULL, 1),"
			" ('1P_SDColony01_P', 13419, 'n_default_spawn_table_id', '147', NULL, NULL, 1),"
			" ('1P_SDColony01_P', 13431, 'n_spawn_table_id',         '85',  NULL, NULL, 1),"
			" ('1P_SDColony01_P', 13431, 'n_default_spawn_table_id', '85',  NULL, NULL, 1),"
			" ('1P_SDColony01_P', 13432, 'n_spawn_table_id',         '99',  NULL, NULL, 1),"
			" ('1P_SDColony01_P', 13432, 'n_default_spawn_table_id', '99',  NULL, NULL, 1),"
			" ('1P_SDColony01_P', 13434, 'n_spawn_table_id',         '167', NULL, NULL, 1),"
			" ('1P_SDColony01_P', 13434, 'n_default_spawn_table_id', '167', NULL, NULL, 1);";
		result = sqlite3_exec(db, kV74_sdcolony01, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v74 (map_object_config sdcolony01): %s\n", err); return; }

		// Add the map to the specops pool (map_pool_id=1 — see v65 for context).
		const char* kV74_specops_pool =
			"INSERT OR IGNORE INTO ga_map_pool_entries (map_pool_id, map_name, game_mode) VALUES"
			" (1, '1P_SDColony01_P', 'TgGame.TgGame_Mission');";
		result = sqlite3_exec(db, kV74_specops_pool, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v74 (specops pool): %s\n", err); return; }

		Logger::Log("db", "v74: seeded map_object_config + specops pool entry for 1P_SDColony01_P\n");
	}
	if (version < 75) {
		// v73 seeded the 6 specops map_game_info rows with friendly_name_msg_ids
		// pulled from the "1P_CP*" labels in asm_data_set_msg_translations, but
		// those resolve to the internal map codenames rather than the in-world
		// facility names players actually see. Repoint each row to the proper
		// facility-name msg_id. CPMine04/CPMine05 use the closest existing rows
		// ("Titanium Processing Site" / "Alumina Mine 1138") because no exact
		// "Titanium Processing Plant" / "Alumina Mine" string ships in the
		// translations table.
		const char* kV75_specops_friendly_names =
			"UPDATE map_game_info SET friendly_name_msg_id = 36084 WHERE map_game_id = 1296;"  // 1P_CPFactory01_P -> Weapon Manufacturing Plant
			"UPDATE map_game_info SET friendly_name_msg_id = 39926 WHERE map_game_id = 1319;"  // 1P_CPLab03       -> Embryonic Agent Testing Lab
			"UPDATE map_game_info SET friendly_name_msg_id = 39064 WHERE map_game_id = 1322;"  // 1P_CPMine04_P    -> Titanium Processing Site
			"UPDATE map_game_info SET friendly_name_msg_id = 38243 WHERE map_game_id = 1323;"  // 1P_CPFactory05_P -> Waste Management Center
			"UPDATE map_game_info SET friendly_name_msg_id = 39062 WHERE map_game_id = 1327;"  // 1P_CPMine05_P    -> Alumina Mine 1138
			"UPDATE map_game_info SET friendly_name_msg_id = 40439 WHERE map_game_id = 1336;"; // 1P_CPLab05_P     -> Bio-Tech Testing Facility
		result = sqlite3_exec(db, kV75_specops_friendly_names, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v75 (specops friendly_name_msg_id retarget): %s\n", err); return; }

		Logger::Log("db", "v75: retargeted friendly_name_msg_id for 6 specops maps to facility-name strings\n");
	}

	if (version < 76) {
		// v76: unify cosmetics into ga_players_inventory.
		//
		// Cosmetics are item-only (ref_device_id = 0 in asm_data_set_items) —
		// they don't instantiate an ATgDevice actor; their state lives on the
		// pawn's r_CustomCharacterAssembly struct. We extend the existing
		// device pool with `item_id` (asm_data_set_items.id) and `stock_n`
		// (which copy — dyes get 5 copies so the same color can fill all 5
		// dye slots simultaneously) so cosmetics share the same inventory_id
		// space as devices. Dispatch in equip / marshal code keys off
		// `item_id > 0` vs `device_id > 0`.
		//
		// Partial unique index makes the cosmetic seed idempotent without
		// constraining device rows (which can legitimately repeat).
		const char* kV76 =
			"ALTER TABLE ga_players_inventory ADD COLUMN item_id INTEGER NOT NULL DEFAULT 0;"
			"ALTER TABLE ga_players_inventory ADD COLUMN stock_n INTEGER NOT NULL DEFAULT 0;"
			"CREATE UNIQUE INDEX IF NOT EXISTS idx_ga_players_inventory_cosmetic_uniq "
			"  ON ga_players_inventory(user_id, item_id, stock_n) WHERE item_id > 0;"
			"CREATE INDEX IF NOT EXISTS idx_ga_players_inventory_item "
			"  ON ga_players_inventory(user_id, item_id);";
		result = sqlite3_exec(db, kV76, nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed v76 (cosmetics columns): %s\n", err);
			sqlite3_free(err);
			return;
		}
		Logger::Log("db", "v76: ga_players_inventory now carries item_id + stock_n for cosmetics\n");
	}

	if (version < 77) {
		// v77: wipe v76's mis-seeded cosmetic rows.
		//
		// The v76 seed inserted asm_data_set_items.id (the row PK) into
		// ga_players_inventory.item_id, but the wire protocol + engine
		// natives (HeadFlairId / SetDyeItemId / etc.) expect
		// asm_data_set_items.item_id (the game-logical id). Effect: cosmetics
		// rendered as random unrelated items on the client (e.g. a Tier5 dye
		// whose .id=3500 marshaled as ITEM_ID=3500 → client looked up 3500
		// in its asm.dat and found "Medic Jetpack"). Worse, the count
		// mismatch caused IsValid to fail and ALL equip-screen widgets to
		// render blank.
		//
		// Cleanup wipes the cosmetic equips (so the joined inventory_ids
		// don't dangle when the inventory rows are deleted), then wipes the
		// cosmetic inventory rows. Next login re-runs SeedCosmetics with
		// the corrected SQL (uses i.item_id, not i.id).
		const char* kV77 =
			"DELETE FROM ga_character_devices "
			"  WHERE inventory_id IN (SELECT id FROM ga_players_inventory WHERE item_id > 0);"
			"DELETE FROM ga_players_inventory WHERE item_id > 0;";
		result = sqlite3_exec(db, kV77, nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed v77 (cosmetic cleanup): %s\n", err);
			sqlite3_free(err);
			return;
		}
		Logger::Log("db", "v77: cleared mis-seeded cosmetic rows; next login will reseed correctly\n");
	}

	if (version < 78) {
		// v78: relocate cosmetic suit/helmet rows off the gameplay device slots.
		//
		// Cosmetic suit (asm_data_set_items.item_subtype_value_id=1008) and
		// the gameplay suit (a device, slot_used_value_id=394) both target
		// engine ES6. UNIQUE(character_id, equipped_slot) on
		// ga_character_devices made one silently overwrite the other on every
		// equip Apply — most visibly, equipping a cosmetic suit wiped the
		// gameplay suit row, so on reconnect the spawn-time device load
		// (filtered by i.device_id > 0) found nothing for slot 6 and the
		// character respawned without their gameplay suit. Same story at ES12
		// for cosmetic helmet/flair (1006/1007) vs gameplay helmet.
		//
		// Fix at the storage layer: move cosmetic-suit rows to DB slot 22
		// (engine ES22 Unused) and cosmetic-helmet rows to DB slot 23
		// (engine ES23 Unused). The wire protocol still presents them at
		// their logical engine slot — the remap lives in CosmeticSlots.hpp.
		//
		// Two passes:
		//   1. Relocate any existing cosmetic-suit/helmet rows.
		//   2. For any character whose gameplay-suit slot 6 is now empty
		//      (because the cosmetic clobbered it before this migration ran)
		//      OR whose gameplay-helmet slot 12 is empty, pin a default
		//      gameplay device from inventory. Without this the user's
		//      gameplay suit/helmet stays gone until they manually re-equip.
		//
		// SQLite caveat: an UPDATE that moves a row to a slot already taken
		// by another row of the same character would hit UNIQUE — but slots
		// 22/23 were never used before v78, so no conflict.
		const char* kV78a =
			"UPDATE ga_character_devices SET equipped_slot = 22 "
			"WHERE equipped_slot = 6 AND inventory_id IN ("
			"  SELECT pi.id FROM ga_players_inventory pi "
			"  JOIN asm_data_set_items ai ON ai.item_id = pi.item_id "
			"  WHERE pi.item_id > 0 AND ai.item_subtype_value_id = 1008"
			");"
			"UPDATE ga_character_devices SET equipped_slot = 23 "
			"WHERE equipped_slot = 12 AND inventory_id IN ("
			"  SELECT pi.id FROM ga_players_inventory pi "
			"  JOIN asm_data_set_items ai ON ai.item_id = pi.item_id "
			"  WHERE pi.item_id > 0 AND ai.item_subtype_value_id IN (1006, 1007)"
			");";
		result = sqlite3_exec(db, kV78a, nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed v78a (cosmetic slot relocation): %s\n", err);
			sqlite3_free(err);
			return;
		}

		// Pin a gameplay suit / helmet at slot 6 / 12 for any character that
		// has equipped gear (at least one row) but is now missing those
		// gameplay slots. Mirrors the slot-14 pin pattern in
		// SeedInventoryFromLoadouts. The inventory lookup picks the first
		// available device row whose allowed_slots matches the target SVID
		// (202 for ES6 Suit, 500 for ES12 Helmet) for the character's
		// profile, preferring profile-specific over shared.
		const char* kV78b =
			"INSERT OR IGNORE INTO ga_character_devices (character_id, inventory_id, equipped_slot) "
			"SELECT c.id, "
			"       (SELECT i.id FROM ga_players_inventory i "
			"        WHERE i.user_id = c.user_id "
			"          AND (i.profile_id = 0 OR i.profile_id = c.profile_id) "
			"          AND i.device_id > 0 "
			"          AND i.allowed_slots = '202' "
			"        ORDER BY i.profile_id DESC LIMIT 1), "
			"       6 "
			"FROM ga_characters c "
			"WHERE EXISTS (SELECT 1 FROM ga_character_devices d WHERE d.character_id = c.id) "
			"  AND NOT EXISTS ("
			"    SELECT 1 FROM ga_character_devices d "
			"    WHERE d.character_id = c.id AND d.equipped_slot = 6"
			"  );"
			"INSERT OR IGNORE INTO ga_character_devices (character_id, inventory_id, equipped_slot) "
			"SELECT c.id, "
			"       (SELECT i.id FROM ga_players_inventory i "
			"        WHERE i.user_id = c.user_id "
			"          AND (i.profile_id = 0 OR i.profile_id = c.profile_id) "
			"          AND i.device_id > 0 "
			"          AND i.allowed_slots = '500' "
			"        ORDER BY i.profile_id DESC LIMIT 1), "
			"       12 "
			"FROM ga_characters c "
			"WHERE EXISTS (SELECT 1 FROM ga_character_devices d WHERE d.character_id = c.id) "
			"  AND NOT EXISTS ("
			"    SELECT 1 FROM ga_character_devices d "
			"    WHERE d.character_id = c.id AND d.equipped_slot = 12"
			"  );";
		result = sqlite3_exec(db, kV78b, nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed v78b (gameplay suit/helmet repin): %s\n", err);
			sqlite3_free(err);
			return;
		}
		Logger::Log("db", "v78: relocated cosmetic suit/helmet to DB slots 22/23 + repinned missing gameplay suit/helmet\n");
	}

	if (version < 79) {
		// v79: Loadout profiles. Adds item_profile_id (1..5) partition key to
		// ga_character_devices + ga_character_skills, plus current_item_profile_id
		// on ga_characters for active-slot persistence. Existing rows land in
		// slot 1; slots 2-5 are absence of rows. See
		// docs/superpowers/specs/2026-05-30-loadout-profiles-design.md for the
		// full design.
		//
		// Idempotent guard: only runs if current_item_profile_id is missing from
		// ga_characters. Tracks 3 sub-steps individually so each tolerates a
		// retry safely.
		bool needs = true;
		{
			sqlite3_stmt* st = nullptr;
			if (sqlite3_prepare_v2(db,
					"SELECT 1 FROM pragma_table_info('ga_characters') "
					"WHERE name = 'current_item_profile_id'",
					-1, &st, nullptr) == SQLITE_OK) {
				if (sqlite3_step(st) == SQLITE_ROW) needs = false;
				sqlite3_finalize(st);
			}
		}
		if (needs) {
			const char* sqls[] = {
				"ALTER TABLE ga_characters ADD COLUMN current_item_profile_id INTEGER NOT NULL DEFAULT 1",

				"CREATE TABLE ga_character_devices_v2 ("
				" id              INTEGER PRIMARY KEY AUTOINCREMENT,"
				" character_id    INTEGER NOT NULL REFERENCES ga_characters(id),"
				" item_profile_id INTEGER NOT NULL,"
				" inventory_id    INTEGER NOT NULL REFERENCES ga_players_inventory(id),"
				" equipped_slot   INTEGER NOT NULL,"
				" UNIQUE(character_id, item_profile_id, equipped_slot)"
				")",
				"INSERT INTO ga_character_devices_v2 (character_id, item_profile_id, inventory_id, equipped_slot)"
				" SELECT character_id, 1, inventory_id, equipped_slot FROM ga_character_devices",
				"DROP TABLE ga_character_devices",
				"ALTER TABLE ga_character_devices_v2 RENAME TO ga_character_devices",
				"CREATE INDEX idx_ga_character_devices_char ON ga_character_devices(character_id)",

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
				"CREATE INDEX idx_ga_character_skills_char ON ga_character_skills(character_id)",
			};
			for (const char* sql : sqls) {
				if (sqlite3_exec(db, sql, nullptr, nullptr, &err) != SQLITE_OK) {
					Logger::Log("db", "v79 step failed: %s -- SQL: %s\n",
						err ? err : "(no msg)", sql);
					if (err) { sqlite3_free(err); err = nullptr; }
				}
			}
			Logger::Log("db", "v79: loadout profiles partition key added\n");
		}
	}

	if (version < 80) {
		// v80: deduplicate ga_character_devices. A historical
		// cosmetic-apply path lacked INSERT OR REPLACE and accumulated
		// thousands of duplicate rows for the same (character_id,
		// item_profile_id, equipped_slot) on long-running DBs (51k+
		// observed). The v79 UNIQUE constraint did NOT fix this
		// retroactively because the v79 INSERT-from-old-table step ran
		// the migration even when individual statements aborted on
		// constraint violation, leaving the new table with whatever
		// rows landed first — but later runtime INSERTs from the old
		// cosmetic path STILL violated the constraint silently if
		// they used a conflict resolution that bypasses UNIQUE
		// (e.g. an unconstrained sqlite_master view from a partial
		// rename).
		//
		// Cleanup keeps the row with the smallest `id` for each
		// unique slot and drops the rest. The smallest-id row is the
		// oldest insert, which mirrors what the UNIQUE constraint
		// would have preserved had it been enforced from the start.
		// Same treatment for ga_character_skills for symmetry —
		// hasn't been observed to dupe but the same code shape
		// applies.
		const char* dedupSqls[] = {
			"DELETE FROM ga_character_devices WHERE id NOT IN ("
			"  SELECT MIN(id) FROM ga_character_devices "
			"  GROUP BY character_id, item_profile_id, equipped_slot)",
			"DELETE FROM ga_character_skills WHERE id NOT IN ("
			"  SELECT MIN(id) FROM ga_character_skills "
			"  GROUP BY character_id, item_profile_id, skill_group_id, skill_id)",
		};
		int totalDevicesBefore = 0, totalSkillsBefore = 0;
		{
			sqlite3_stmt* st = nullptr;
			if (sqlite3_prepare_v2(db,
					"SELECT COUNT(*) FROM ga_character_devices", -1, &st, nullptr) == SQLITE_OK) {
				if (sqlite3_step(st) == SQLITE_ROW) totalDevicesBefore = sqlite3_column_int(st, 0);
				sqlite3_finalize(st);
			}
			if (sqlite3_prepare_v2(db,
					"SELECT COUNT(*) FROM ga_character_skills", -1, &st, nullptr) == SQLITE_OK) {
				if (sqlite3_step(st) == SQLITE_ROW) totalSkillsBefore = sqlite3_column_int(st, 0);
				sqlite3_finalize(st);
			}
		}
		for (const char* sql : dedupSqls) {
			if (sqlite3_exec(db, sql, nullptr, nullptr, &err) != SQLITE_OK) {
				Logger::Log("db", "v80 dedup step failed: %s -- SQL: %s\n",
					err ? err : "(no msg)", sql);
				if (err) { sqlite3_free(err); err = nullptr; }
			}
		}
		int totalDevicesAfter = 0, totalSkillsAfter = 0;
		{
			sqlite3_stmt* st = nullptr;
			if (sqlite3_prepare_v2(db,
					"SELECT COUNT(*) FROM ga_character_devices", -1, &st, nullptr) == SQLITE_OK) {
				if (sqlite3_step(st) == SQLITE_ROW) totalDevicesAfter = sqlite3_column_int(st, 0);
				sqlite3_finalize(st);
			}
			if (sqlite3_prepare_v2(db,
					"SELECT COUNT(*) FROM ga_character_skills", -1, &st, nullptr) == SQLITE_OK) {
				if (sqlite3_step(st) == SQLITE_ROW) totalSkillsAfter = sqlite3_column_int(st, 0);
				sqlite3_finalize(st);
			}
		}
		Logger::Log("db",
			"v80: dedup ga_character_devices %d -> %d (removed %d), ga_character_skills %d -> %d (removed %d)\n",
			totalDevicesBefore, totalDevicesAfter, totalDevicesBefore - totalDevicesAfter,
			totalSkillsBefore, totalSkillsAfter, totalSkillsBefore - totalSkillsAfter);
	}

	if (version < 81) {
		// v81: persist the three appearance fields the character-create UI
		// collects (CharacterInfoStruct.nHairAsmId / nSkinMatParamId /
		// nEyeMatParamId in TgDataInterface.uc). Previously thrown away by
		// ADD_PLAYER_CHARACTER; ship-back was hardcoded to HAIR_ASM_ID=0x85D
		// in send_character_list_response, which is incompatible with most
		// non-default heads and crashes the client's pawn-asm-attach pipeline
		// (FUN_109d1860, ~0x109d1f5b GPF through a 0xCCCCCCCC pointer).
		//
		// DEFAULT 0 is the deliberate safe fallback: hair_asm_id=0 means
		// "no hair" in the engine, which is the only value guaranteed not
		// to crash regardless of head/gender. Existing characters get 0
		// and render bald until they next re-enter the head menu.
		//
		// ALTERs are idempotent — swallow duplicate-column error on re-boot.
		const char* alters[] = {
			"ALTER TABLE ga_characters ADD COLUMN hair_asm_id        INTEGER NOT NULL DEFAULT 0;",
			"ALTER TABLE ga_characters ADD COLUMN skin_mat_param_id  INTEGER NOT NULL DEFAULT 0;",
			"ALTER TABLE ga_characters ADD COLUMN eye_mat_param_id   INTEGER NOT NULL DEFAULT 0;",
		};
		for (const char* sql : alters) {
			if (sqlite3_exec(db, sql, nullptr, nullptr, &err) != SQLITE_OK) {
				if (err) { sqlite3_free(err); err = nullptr; }
			}
		}
		Logger::Log("db", "v81: added ga_characters.{hair_asm_id, skin_mat_param_id, eye_mat_param_id}\n");
	}

	if (version < 82) {
		// v82: backfill hair_asm_id=0 → 403 (PC_CHARBUILD_Bald).
		// SUPERSEDED by v83 — asm 403 is `asm_mesh_type_value_id=596`,
		// the *character-builder UI* hair category, NOT the in-game
		// pawn hair category (type 850). The in-game pawn-asm-attach
		// pipeline asks the asset manager specifically for a type-850
		// hair; asm 403 isn't reachable from that lookup path, so the
		// asset manager still returns garbage and the client still
		// crashes at 0x109d1f5b. Left in for migration ordering.
		result = sqlite3_exec(db,
			"UPDATE ga_characters SET hair_asm_id = 403 WHERE hair_asm_id = 0;",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			if (err) { sqlite3_free(err); err = nullptr; }
		}
	}

	if (version < 83) {
		// v83: backfill hair_asm_id to a proven-loadable type-850 hair.
		//
		// 1974 = "NewHair15" (asm_mesh_type_value_id=850, mesh_res_id=4844)
		// is the same asm SpawnBotPawn sets on every bot's
		// r_CustomCharacterAssembly.HairMeshId — confirmed to load at
		// gameplay time without crashing.
		//
		// Replaces BOTH the original 0 sentinel and the misguided v82
		// fallback (403 / PC_CHARBUILD_Bald — wrong mesh-type category;
		// it's only loaded in the character-builder map and isn't
		// reachable from the in-game pawn-attach path).
		//
		// Idempotent — re-running finds no rows to update.
		result = sqlite3_exec(db,
			"UPDATE ga_characters SET hair_asm_id = 1974 WHERE hair_asm_id IN (0, 403);",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "v83 hair backfill failed: %s\n", err ? err : "(no msg)");
			if (err) { sqlite3_free(err); err = nullptr; }
		} else {
			Logger::Log("db", "v83: backfilled %d ga_characters rows hair_asm_id (0|403) -> 1974 (NewHair15, type-850)\n",
				sqlite3_changes(db));
		}
	}

	if (version < 84) {
		// v84: wire up TgDeviceVolume placements on 1P_CPFactory01_P.
		//
		// The map binary has 10 placed TgDeviceVolume actors but their
		// EditConst fields (s_n_device_id / s_n_team_number / s_n_task_force)
		// aren't carried through the .umap → SQL import — they land here as 0,
		// which `TgDeviceVolume::setupDevice` treats as "no device wired"
		// (bails before allocating a fire mode). Override per placement via
		// `map_object_config`, which is what `MapObjectConfig::Init` reads
		// at PostBeginPlay time. Identified via the map_object_id values on
		// `map_tg_device_volume` rows for this map.
		//
		// 8990 (Dropship/T1)  -> 2801 Invulnerable (Friend and Self), team/tf 1
		// 12259/12260 (T2 Transfer), 12317/12646/12696/12697 (objective hazards)
		//                       -> 7029 Magma Burn (Fire-typed, All target), team/tf 2
		// 12754                -> 5448 KillEffect / Grinder (instant-kill Enemy), team/tf 2
		//
		// One-shot insert — the version gate keeps this from re-running on
		// existing DBs at v84+. Fresh DBs already include the rows via the
		// asm capture, so the insert is harmless there too.
		const char* kInsert =
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES"
			"  ('1P_CPFactory01_P',  8990, 's_n_task_force',  '1',    NULL, NULL, 1),"
			"  ('1P_CPFactory01_P',  8990, 's_n_team_number', '1',    NULL, NULL, 1),"
			"  ('1P_CPFactory01_P',  8990, 's_n_device_id',   '2801', NULL, NULL, 1),"
			"  ('1P_CPFactory01_P', 12259, 's_n_device_id',   '7029', NULL, NULL, 1),"
			"  ('1P_CPFactory01_P', 12259, 's_n_team_number', '2',    NULL, NULL, 1),"
			"  ('1P_CPFactory01_P', 12259, 's_n_task_force',  '2',    NULL, NULL, 1),"
			"  ('1P_CPFactory01_P', 12260, 's_n_device_id',   '7029', NULL, NULL, 1),"
			"  ('1P_CPFactory01_P', 12260, 's_n_team_number', '2',    NULL, NULL, 1),"
			"  ('1P_CPFactory01_P', 12260, 's_n_task_force',  '2',    NULL, NULL, 1),"
			"  ('1P_CPFactory01_P', 12317, 's_n_device_id',   '7029', NULL, NULL, 1),"
			"  ('1P_CPFactory01_P', 12317, 's_n_team_number', '2',    NULL, NULL, 1),"
			"  ('1P_CPFactory01_P', 12317, 's_n_task_force',  '2',    NULL, NULL, 1),"
			"  ('1P_CPFactory01_P', 12646, 's_n_device_id',   '7029', NULL, NULL, 1),"
			"  ('1P_CPFactory01_P', 12646, 's_n_team_number', '2',    NULL, NULL, 1),"
			"  ('1P_CPFactory01_P', 12646, 's_n_task_force',  '2',    NULL, NULL, 1),"
			"  ('1P_CPFactory01_P', 12696, 's_n_device_id',   '7029', NULL, NULL, 1),"
			"  ('1P_CPFactory01_P', 12696, 's_n_team_number', '2',    NULL, NULL, 1),"
			"  ('1P_CPFactory01_P', 12696, 's_n_task_force',  '2',    NULL, NULL, 1),"
			"  ('1P_CPFactory01_P', 12697, 's_n_device_id',   '7029', NULL, NULL, 1),"
			"  ('1P_CPFactory01_P', 12697, 's_n_team_number', '2',    NULL, NULL, 1),"
			"  ('1P_CPFactory01_P', 12697, 's_n_task_force',  '2',    NULL, NULL, 1),"
			"  ('1P_CPFactory01_P', 12754, 's_n_device_id',   '5448', NULL, NULL, 1),"
			"  ('1P_CPFactory01_P', 12754, 's_n_team_number', '2',    NULL, NULL, 1),"
			"  ('1P_CPFactory01_P', 12754, 's_n_task_force',  '2',    NULL, NULL, 1);";
		result = sqlite3_exec(db, kInsert, nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "v84 device-volume config insert failed: %s\n", err ? err : "(no msg)");
			if (err) { sqlite3_free(err); err = nullptr; }
		} else {
			Logger::Log("db", "v84: seeded %d map_object_config rows for 1P_CPFactory01_P TgDeviceVolume placements\n",
				sqlite3_changes(db));
		}
	}

	if (version < 85) {
		// v85: wire up TgDeviceVolume placements on 1P_CPFactory05_P.
		//
		// Same shape as v84 (CPFactory01) — overrides s_n_device_id /
		// s_n_team_number / s_n_task_force per placement so
		// MapObjectConfig drives setupDevice on map load. Hazard names
		// resolved via asm_data_set_items.name_msg_id (see
		// device_volumes.md):
		//
		//  8990                       -> 2801 Invulnerable / dev sandbox, team/tf 1
		//                                 (dropship area, friendly bubble)
		//  12569, 12570               -> 2238 Crusher, team/tf 2
		//  12562, 12563               -> 5448 Grinder, team/tf 2
		//  12554, 12564..12567        -> 5449 Ventilation Fan, team/tf 2
		//
		// 12569's s_n_team_number was sent as empty string — normalized
		// to '2' to match every other 12xxx placement (the empty form
		// would parse to 0 in MapObjectConfig::GetInt and break team
		// resolution).
		//
		// One-shot insert — version gate keeps this from re-running.
		const char* kInsert =
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES"
			"  ('1P_CPFactory05_P',  8990, 's_n_task_force',  '1',    NULL, NULL, 1),"
			"  ('1P_CPFactory05_P',  8990, 's_n_team_number', '1',    NULL, NULL, 1),"
			"  ('1P_CPFactory05_P',  8990, 's_n_device_id',   '2801', NULL, NULL, 1),"
			"  ('1P_CPFactory05_P', 12554, 's_n_device_id',   '5449', NULL, NULL, 1),"
			"  ('1P_CPFactory05_P', 12554, 's_n_team_number', '2',    NULL, NULL, 1),"
			"  ('1P_CPFactory05_P', 12554, 's_n_task_force',  '2',    NULL, NULL, 1),"
			"  ('1P_CPFactory05_P', 12562, 's_n_device_id',   '5448', NULL, NULL, 1),"
			"  ('1P_CPFactory05_P', 12562, 's_n_team_number', '2',    NULL, NULL, 1),"
			"  ('1P_CPFactory05_P', 12562, 's_n_task_force',  '2',    NULL, NULL, 1),"
			"  ('1P_CPFactory05_P', 12563, 's_n_device_id',   '5448', NULL, NULL, 1),"
			"  ('1P_CPFactory05_P', 12563, 's_n_team_number', '2',    NULL, NULL, 1),"
			"  ('1P_CPFactory05_P', 12563, 's_n_task_force',  '2',    NULL, NULL, 1),"
			"  ('1P_CPFactory05_P', 12564, 's_n_device_id',   '5449', NULL, NULL, 1),"
			"  ('1P_CPFactory05_P', 12564, 's_n_team_number', '2',    NULL, NULL, 1),"
			"  ('1P_CPFactory05_P', 12564, 's_n_task_force',  '2',    NULL, NULL, 1),"
			"  ('1P_CPFactory05_P', 12565, 's_n_device_id',   '5449', NULL, NULL, 1),"
			"  ('1P_CPFactory05_P', 12565, 's_n_team_number', '2',    NULL, NULL, 1),"
			"  ('1P_CPFactory05_P', 12565, 's_n_task_force',  '2',    NULL, NULL, 1),"
			"  ('1P_CPFactory05_P', 12566, 's_n_device_id',   '5449', NULL, NULL, 1),"
			"  ('1P_CPFactory05_P', 12566, 's_n_team_number', '2',    NULL, NULL, 1),"
			"  ('1P_CPFactory05_P', 12566, 's_n_task_force',  '2',    NULL, NULL, 1),"
			"  ('1P_CPFactory05_P', 12567, 's_n_device_id',   '5449', NULL, NULL, 1),"
			"  ('1P_CPFactory05_P', 12567, 's_n_team_number', '2',    NULL, NULL, 1),"
			"  ('1P_CPFactory05_P', 12567, 's_n_task_force',  '2',    NULL, NULL, 1),"
			"  ('1P_CPFactory05_P', 12569, 's_n_device_id',   '2238', NULL, NULL, 1),"
			"  ('1P_CPFactory05_P', 12569, 's_n_team_number', '2',    NULL, NULL, 1),"
			"  ('1P_CPFactory05_P', 12569, 's_n_task_force',  '2',    NULL, NULL, 1),"
			"  ('1P_CPFactory05_P', 12570, 's_n_device_id',   '2238', NULL, NULL, 1),"
			"  ('1P_CPFactory05_P', 12570, 's_n_team_number', '2',    NULL, NULL, 1),"
			"  ('1P_CPFactory05_P', 12570, 's_n_task_force',  '2',    NULL, NULL, 1);";
		result = sqlite3_exec(db, kInsert, nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "v85 device-volume config insert failed: %s\n", err ? err : "(no msg)");
			if (err) { sqlite3_free(err); err = nullptr; }
		} else {
			Logger::Log("db", "v85: seeded %d map_object_config rows for 1P_CPFactory05_P TgDeviceVolume placements\n",
				sqlite3_changes(db));
		}
	}

	if (version < 86) {
		// v86: wire up TgDeviceVolume placements on 1P_CPLab05_P.
		//
		//  8990   -> 2801 Invulnerable / dev sandbox, team/tf 1 (dropship)
		//  12720  -> 5299 Poison Gas (Biological-gated), team/tf 2
		//  12721  -> 4976 Toxic Waste, team/tf 2
		//
		// One-shot insert — version gate keeps this from re-running.
		const char* kInsert =
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES"
			"  ('1P_CPLab05_P',  8990, 's_n_task_force',  '1',    NULL, NULL, 1),"
			"  ('1P_CPLab05_P',  8990, 's_n_team_number', '1',    NULL, NULL, 1),"
			"  ('1P_CPLab05_P',  8990, 's_n_device_id',   '2801', NULL, NULL, 1),"
			"  ('1P_CPLab05_P', 12720, 's_n_device_id',   '5299', NULL, NULL, 1),"
			"  ('1P_CPLab05_P', 12720, 's_n_team_number', '2',    NULL, NULL, 1),"
			"  ('1P_CPLab05_P', 12720, 's_n_task_force',  '2',    NULL, NULL, 1),"
			"  ('1P_CPLab05_P', 12721, 's_n_device_id',   '4976', NULL, NULL, 1),"
			"  ('1P_CPLab05_P', 12721, 's_n_team_number', '2',    NULL, NULL, 1),"
			"  ('1P_CPLab05_P', 12721, 's_n_task_force',  '2',    NULL, NULL, 1);";
		result = sqlite3_exec(db, kInsert, nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "v86 device-volume config insert failed: %s\n", err ? err : "(no msg)");
			if (err) { sqlite3_free(err); err = nullptr; }
		} else {
			Logger::Log("db", "v86: seeded %d map_object_config rows for 1P_CPLab05_P TgDeviceVolume placements\n",
				sqlite3_changes(db));
		}
	}

	if (version < 87) {
		// v87: wire up TgDeviceVolume placements on 1P_CPMine04_P.
		//
		//  12513  -> 2801 Invulnerable / dev sandbox, team/tf 1
		//            (dropship; map_object_id is 12513 here, not 8990 as
		//            on the CPFactory / CPLab maps)
		//  12453  -> 7029 Lava (Fire-typed Magma Burn), team/tf 2
		//
		// One-shot insert — version gate keeps this from re-running.
		const char* kInsert =
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES"
			"  ('1P_CPMine04_P', 12453, 's_n_device_id',   '7029', NULL, NULL, 1),"
			"  ('1P_CPMine04_P', 12453, 's_n_team_number', '2',    NULL, NULL, 1),"
			"  ('1P_CPMine04_P', 12453, 's_n_task_force',  '2',    NULL, NULL, 1),"
			"  ('1P_CPMine04_P', 12513, 's_n_device_id',   '2801', NULL, NULL, 1),"
			"  ('1P_CPMine04_P', 12513, 's_n_team_number', '1',    NULL, NULL, 1),"
			"  ('1P_CPMine04_P', 12513, 's_n_task_force',  '1',    NULL, NULL, 1);";
		result = sqlite3_exec(db, kInsert, nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "v87 device-volume config insert failed: %s\n", err ? err : "(no msg)");
			if (err) { sqlite3_free(err); err = nullptr; }
		} else {
			Logger::Log("db", "v87: seeded %d map_object_config rows for 1P_CPMine04_P TgDeviceVolume placements\n",
				sqlite3_changes(db));
		}
	}

	if (version < 88) {
		// v88: wire up TgDeviceVolume placements on 1P_CPMine05_P.
		//
		//  12513  -> 2801 Invulnerable / dev sandbox, team/tf 1
		//            (dropship; map_object_id 12513 like CPMine04)
		//  12572  -> 7029 Lava (Fire-typed Magma Burn), team/tf 2
		//
		// One-shot insert — version gate keeps this from re-running.
		const char* kInsert =
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES"
			"  ('1P_CPMine05_P', 12513, 's_n_device_id',   '2801', NULL, NULL, 1),"
			"  ('1P_CPMine05_P', 12513, 's_n_team_number', '1',    NULL, NULL, 1),"
			"  ('1P_CPMine05_P', 12513, 's_n_task_force',  '1',    NULL, NULL, 1),"
			"  ('1P_CPMine05_P', 12572, 's_n_device_id',   '7029', NULL, NULL, 1),"
			"  ('1P_CPMine05_P', 12572, 's_n_team_number', '2',    NULL, NULL, 1),"
			"  ('1P_CPMine05_P', 12572, 's_n_task_force',  '2',    NULL, NULL, 1);";
		result = sqlite3_exec(db, kInsert, nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "v88 device-volume config insert failed: %s\n", err ? err : "(no msg)");
			if (err) { sqlite3_free(err); err = nullptr; }
		} else {
			Logger::Log("db", "v88: seeded %d map_object_config rows for 1P_CPMine05_P TgDeviceVolume placements\n",
				sqlite3_changes(db));
		}
	}

	if (version < 89) {
		// v89: wire up TgDeviceVolume placements on 1P_CPLab03.
		//
		// Note: map_name has no `_P` suffix on this one (unlike all prior
		// migrations in this series). Match exactly what was imported.
		//
		//  12482  -> 2801 Invulnerable / dev sandbox, team/tf 1 (dropship)
		//  12483  -> 5450 High Voltage (KillEffect Enemy), team/tf 2
		//
		// One-shot insert — version gate keeps this from re-running.
		const char* kInsert =
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES"
			"  ('1P_CPLab03', 12482, 's_n_device_id',   '2801', NULL, NULL, 1),"
			"  ('1P_CPLab03', 12482, 's_n_team_number', '1',    NULL, NULL, 1),"
			"  ('1P_CPLab03', 12482, 's_n_task_force',  '1',    NULL, NULL, 1),"
			"  ('1P_CPLab03', 12483, 's_n_device_id',   '5450', NULL, NULL, 1),"
			"  ('1P_CPLab03', 12483, 's_n_team_number', '2',    NULL, NULL, 1),"
			"  ('1P_CPLab03', 12483, 's_n_task_force',  '2',    NULL, NULL, 1);";
		result = sqlite3_exec(db, kInsert, nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "v89 device-volume config insert failed: %s\n", err ? err : "(no msg)");
			if (err) { sqlite3_free(err); err = nullptr; }
		} else {
			Logger::Log("db", "v89: seeded %d map_object_config rows for 1P_CPLab03 TgDeviceVolume placements\n",
				sqlite3_changes(db));
		}
	}

	if (version < 90) {
		// v90: introduce `mod_data_set_bot_spawn_tables` — same schema as the
		// read-only `asm_data_set_bot_spawn_tables`, but writable for our own
		// custom spawn-table authoring. TgBotFactory__LoadObjectConfig reads
		// the UNION of both tables, so every row inserted here participates in
		// the difficulty cascade alongside the asm_* rows.
		//
		// Convention: asm_* prefix = verbatim from the game files, treat as
		// read-only; mod_* prefix = our additions.
		result = sqlite3_exec(db,
			"CREATE TABLE IF NOT EXISTS mod_data_set_bot_spawn_tables ( "
			"  id INTEGER PRIMARY KEY AUTOINCREMENT, "
			"  bot_spawn_table_id INTEGER, "
			"  difficulty_value_id INTEGER, "
			"  player_profile_id INTEGER, "
			"  spawn_group INTEGER, "
			"  enemy_bot_id INTEGER, "
			"  bot_count INTEGER, "
			"  spawn_chance FLOAT, "
			"  team_size INTEGER, "
			"  multiple_class_flag INTEGER, "
			"  bot_balance_multiplier FLOAT, "
			"  spawn_group_min INTEGER, "
			"  spawn_group_max INTEGER, "
			"  spawn_group_respawn_sec INTEGER "
			");",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "v90 create mod_data_set_bot_spawn_tables failed: %s\n", err ? err : "(no msg)");
			if (err) { sqlite3_free(err); err = nullptr; }
		}

		// Seed custom spawn tables at difficulty 1467 (Novice, lowest tier on
		// valid_value group 116). The cascade walks tiers ≤ primary by
		// sort_order descending, so a row authored at the lowest tier is the
		// very last fallback — proves the multi-tier cascade is doing real
		// work at any current difficulty. High ids (>=10000) keep custom
		// tables visually distinct from the 1–252 range used by the original
		// asm_ data.
		//   10000 — 4x Sector Worker (1305), 100% chance
		//   10001 — 1x Elite Helot (1321),  100% chance
		result = sqlite3_exec(db,
			"INSERT INTO mod_data_set_bot_spawn_tables "
			"  (bot_spawn_table_id, difficulty_value_id, player_profile_id, spawn_group, "
			"   enemy_bot_id, bot_count, spawn_chance, team_size, multiple_class_flag, "
			"   bot_balance_multiplier, spawn_group_min, spawn_group_max, spawn_group_respawn_sec) "
			"VALUES "
			"  (10000, 1467, 0, 1, 1305, 4, 1.0, 0, 0, 1.0, 0, 0, 0), "
			"  (10001, 1467, 0, 1, 1321, 1, 1.0, 0, 0, 1.0, 0, 0, 0);",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "v90 seed mod spawn tables failed: %s\n", err ? err : "(no msg)");
			if (err) { sqlite3_free(err); err = nullptr; }
		} else {
			Logger::Log("db", "v90: seeded mod_data_set_bot_spawn_tables ids 10000 (4x Sector Worker) + 10001 (1x Elite Helot) @ Novice\n");
		}
	}

	if (version < 91) {
		// v91: wire up 1P_CPLab01_P — TgDeviceVolume placements, factory
		// task_force / team_number / spawn_table assignments, plus two
		// bindings to the custom mod_ spawn tables seeded in v90
		// (12745/12746/12751/12752 → 10000 = 4x Sector Worker;
		//  12761 → 10001 = 1x Elite Helot). Map added to specops pool below.
		const char* kV91_cplab01 =
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES"
			"  ('1P_CPLab01_P', 12749, 'm_n_task_force', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 11531, 'm_n_task_force', '1', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12762, 's_n_task_force', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12762, 's_n_team_number', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12762, 's_n_device_id', '2238', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12089, 's_n_task_force', '1', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12089, 's_n_team_number', '1', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12089, 's_n_device_id', '2801', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 11514, 's_n_task_force', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 11514, 's_n_team_number', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 11515, 's_n_team_number', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 11515, 's_n_task_force', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 11518, 's_n_team_number', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 11518, 's_n_task_force', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 11519, 's_n_task_force', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 11519, 's_n_team_number', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 11520, 's_n_task_force', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 11520, 's_n_team_number', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 11521, 's_n_task_force', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 11521, 's_n_team_number', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 11530, 's_n_task_force', '1', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 11530, 's_n_team_number', '1', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 11633, 's_n_task_force', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 11633, 's_n_team_number', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12326, 's_n_task_force', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12326, 's_n_team_number', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12327, 's_n_team_number', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12327, 's_n_task_force', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12325, 's_n_task_force', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12325, 's_n_team_number', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12332, 's_n_task_force', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12332, 's_n_team_number', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12334, 's_n_task_force', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12334, 's_n_team_number', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12335, 's_n_task_force', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12335, 's_n_team_number', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12336, 's_n_task_force', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12336, 's_n_team_number', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12337, 's_n_task_force', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12337, 's_n_team_number', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12338, 's_n_team_number', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12338, 's_n_task_force', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12339, 's_n_task_force', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12339, 's_n_team_number', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12743, 's_n_task_force', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12743, 's_n_team_number', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12745, 's_n_task_force', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12745, 's_n_team_number', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12746, 's_n_task_force', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12746, 's_n_team_number', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12748, 's_n_task_force', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12748, 's_n_team_number', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12286, 's_n_task_force', '1', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12286, 's_n_team_number', '1', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12751, 's_n_task_force', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12751, 's_n_team_number', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12752, 's_n_task_force', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12752, 's_n_team_number', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12755, 's_n_task_force', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12755, 's_n_team_number', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12756, 's_n_task_force', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12756, 's_n_team_number', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12757, 's_n_task_force', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12757, 's_n_team_number', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12761, 's_n_task_force', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12761, 's_n_team_number', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12758, 's_n_task_force', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12758, 's_n_team_number', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12759, 's_n_task_force', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12759, 's_n_team_number', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12760, 's_n_task_force', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12760, 's_n_team_number', '2', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 11514, 'n_spawn_table_id', '29', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 11514, 'n_default_spawn_table_id', '29', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 11515, 'n_default_spawn_table_id', '28', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 11515, 'n_spawn_table_id', '28', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 11518, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 11518, 'n_spawn_table_id', '33', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 11519, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 11519, 'n_spawn_table_id', '33', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 11520, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 11520, 'n_spawn_table_id', '33', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 11521, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 11521, 'n_spawn_table_id', '33', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 11633, 'n_spawn_table_id', '34', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 11633, 'n_default_spawn_table_id', '34', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12326, 'n_default_spawn_table_id', '28', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12326, 'n_spawn_table_id', '28', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12327, 'n_default_spawn_table_id', '28', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12327, 'n_spawn_table_id', '28', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12325, 'n_spawn_table_id', '34', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12325, 'n_default_spawn_table_id', '34', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12332, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12332, 'n_spawn_table_id', '33', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12334, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12334, 'n_spawn_table_id', '33', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12335, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12335, 'n_spawn_table_id', '33', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12336, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12336, 'n_spawn_table_id', '33', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12337, 'n_spawn_table_id', '59', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12337, 'n_default_spawn_table_id', '59', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12338, 'n_default_spawn_table_id', '59', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12338, 'n_spawn_table_id', '59', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12339, 'n_default_spawn_table_id', '59', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12339, 'n_spawn_table_id', '59', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12743, 'n_spawn_table_id', '28', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12743, 'n_default_spawn_table_id', '28', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12755, 'n_spawn_table_id', '33', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12755, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12756, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12756, 'n_spawn_table_id', '33', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12757, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12757, 'n_spawn_table_id', '33', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12758, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12758, 'n_spawn_table_id', '33', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12759, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12759, 'n_spawn_table_id', '33', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12760, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12760, 'n_spawn_table_id', '33', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12745, 'n_spawn_table_id', '10000', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12745, 'n_default_spawn_table_id', '10000', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12746, 'n_default_spawn_table_id', '10000', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12746, 'n_spawn_table_id', '10000', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12751, 'n_default_spawn_table_id', '10000', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12751, 'n_spawn_table_id', '10000', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12752, 'n_default_spawn_table_id', '10000', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12752, 'n_spawn_table_id', '10000', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12761, 'n_spawn_table_id', '10001', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12761, 'n_default_spawn_table_id', '10001', NULL, NULL, 1);";
		result = sqlite3_exec(db, kV91_cplab01, nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "v91 map_object_config insert failed: %s\n", err ? err : "(no msg)");
			if (err) { sqlite3_free(err); err = nullptr; }
		}

		// Add map to specops pool (map_pool_id=1 — see v65 for context).
		const char* kV91_specops_pool =
			"INSERT OR IGNORE INTO ga_map_pool_entries (map_pool_id, map_name, game_mode) VALUES"
			" (1, '1P_CPLab01_P', 'TgGame.TgGame_Mission');";
		result = sqlite3_exec(db, kV91_specops_pool, nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "v91 specops pool insert failed: %s\n", err ? err : "(no msg)");
			if (err) { sqlite3_free(err); err = nullptr; }
		} else {
			Logger::Log("db", "v91: seeded map_object_config + specops pool entry for 1P_CPLab01_P\n");
		}
	}

	if (version < 92) {
		// v92: register 1P_CPLab01_P in map_game_info so it shows up in the
		// mission selection UI. Companion to v91 (which wired up factories
		// + added the map to the specops pool).
		//   entry_background_image_res_id = 6516
		//     -> asm_data_set_resources row "HUD_MissionLoads.PvE_CP.1P_CPLab01_P",
		//        matching the 6511/6515/6518/6520/6524/6525 family used by the
		//        other 1P_CP* PvE-CP missions.
		const char* kV92_mapgameinfo =
			"INSERT OR IGNORE INTO map_game_info "
			"  (map_game_id, map_name, game_class, gameplay_type_value_id, friendly_name_msg_id, entry_background_image_res_id) "
			"VALUES (1302, '1P_CPLab01_P', 'TgGame.TgGame_Mission', 1553, 40683, 6516);";
		result = sqlite3_exec(db, kV92_mapgameinfo, nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "v92 map_game_info insert failed: %s\n", err ? err : "(no msg)");
			if (err) { sqlite3_free(err); err = nullptr; }
		} else {
			Logger::Log("db", "v92: registered 1P_CPLab01_P in map_game_info (map_game_id=1302, bg=6516)\n");
		}
	}

	if (version < 93) {
		// v93: disable s_b_auto_spawn on two 1P_CPLab01_P map objects
		// (11530, 12286) — both are TgMissionObjective_Bot placements that
		// must wait for kismet rather than auto-spawning on PostBeginPlay.
		const char* kV93_cplab01_autospawn =
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES"
			"  ('1P_CPLab01_P', 11530, 's_b_auto_spawn', '1', NULL, NULL, 1),"
			"  ('1P_CPLab01_P', 12286, 's_b_auto_spawn', '0', NULL, NULL, 1);";
		result = sqlite3_exec(db, kV93_cplab01_autospawn, nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "v93 1P_CPLab01_P autospawn insert failed: %s\n", err ? err : "(no msg)");
			if (err) { sqlite3_free(err); err = nullptr; }
		} else {
			Logger::Log("db", "v93: disabled s_b_auto_spawn on 1P_CPLab01_P objects 11530, 12286\n");
		}
	}

	if (version < 94) {
		// v94: priority + auto-spawn tweaks across six PvE-CP maps. Bumps
		// objective-related map objects to priority 1 so they advance out of
		// the boot phase, and disables auto-spawn on a few bot factories that
		// should wait for kismet rather than spawning at PostBeginPlay.
		const char* kV94_cplab03 =
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES"
			"  ('1P_CPLab03', 12541, 's_b_auto_spawn', '0', NULL, NULL, 1),"
			"  ('1P_CPLab03', 12480, 'm_n_priority',   '1', NULL, NULL, 1),"
			"  ('1P_CPLab03', 12541, 'm_n_priority',   '1', NULL, NULL, 1);";
		result = sqlite3_exec(db, kV94_cplab03, nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "v94 cplab03 insert failed: %s\n", err ? err : "(no msg)");
			if (err) { sqlite3_free(err); err = nullptr; }
		}

		const char* kV94_cpmine05 =
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES"
			"  ('1P_CPMine05_P', 12512, 'm_n_priority',   '1', NULL, NULL, 1),"
			"  ('1P_CPMine05_P', 12581, 'm_n_priority',   '1', NULL, NULL, 1),"
			"  ('1P_CPMine05_P', 12581, 's_b_auto_spawn', '0', NULL, NULL, 1);";
		result = sqlite3_exec(db, kV94_cpmine05, nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "v94 cpmine05 insert failed: %s\n", err ? err : "(no msg)");
			if (err) { sqlite3_free(err); err = nullptr; }
		}

		const char* kV94_cpfactory05 =
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES"
			"  ('1P_CPFactory05_P', 12286, 's_b_auto_spawn', '0', NULL, NULL, 1),"
			"  ('1P_CPFactory05_P', 12286, 'm_n_priority',   '1', NULL, NULL, 1),"
			"  ('1P_CPFactory05_P', 11716, 'm_n_priority',   '1', NULL, NULL, 1);";
		result = sqlite3_exec(db, kV94_cpfactory05, nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "v94 cpfactory05 insert failed: %s\n", err ? err : "(no msg)");
			if (err) { sqlite3_free(err); err = nullptr; }
		}

		const char* kV94_cpmine04 =
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES"
			"  ('1P_CPMine04_P', 12512, 'm_n_priority',   '1', NULL, NULL, 1),"
			"  ('1P_CPMine04_P', 12542, 'm_n_priority',   '1', NULL, NULL, 1),"
			"  ('1P_CPMine04_P', 12542, 's_b_auto_spawn', '0', NULL, NULL, 1);";
		result = sqlite3_exec(db, kV94_cpmine04, nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "v94 cpmine04 insert failed: %s\n", err ? err : "(no msg)");
			if (err) { sqlite3_free(err); err = nullptr; }
		}

		const char* kV94_cplab05 =
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES"
			"  ('1P_CPLab05_P', 11716, 'm_n_priority',   '1', NULL, NULL, 1),"
			"  ('1P_CPLab05_P', 12286, 'm_n_priority',   '1', NULL, NULL, 1),"
			"  ('1P_CPLab05_P', 12286, 's_n_team_number','1', NULL, NULL, 1),"
			"  ('1P_CPLab05_P', 12286, 's_n_task_force', '1', NULL, NULL, 1),"
			"  ('1P_CPLab05_P', 11716, 's_n_task_force', '1', NULL, NULL, 1),"
			"  ('1P_CPLab05_P', 11716, 's_n_team_number','1', NULL, NULL, 1),"
			"  ('1P_CPLab05_P', 12286, 's_b_auto_spawn', '0', NULL, NULL, 1);";
		result = sqlite3_exec(db, kV94_cplab05, nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "v94 cplab05 insert failed: %s\n", err ? err : "(no msg)");
			if (err) { sqlite3_free(err); err = nullptr; }
		}

		const char* kV94_cpfactory01 =
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES"
			"  ('1P_CPFactory01_P', 11716, 'm_n_priority',   '1', NULL, NULL, 1),"
			"  ('1P_CPFactory01_P', 12286, 'm_n_priority',   '1', NULL, NULL, 1),"
			"  ('1P_CPFactory01_P', 12286, 's_b_auto_spawn', '0', NULL, NULL, 1);";
		result = sqlite3_exec(db, kV94_cpfactory01, nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "v94 cpfactory01 insert failed: %s\n", err ? err : "(no msg)");
			if (err) { sqlite3_free(err); err = nullptr; }
		} else {
			Logger::Log("db", "v94: priority + auto-spawn tweaks applied across 6 PvE-CP maps\n");
		}
	}

	if (version < 95) {
		// v95: per-map mission time + PvP flag on map_game_info. Replaces the
		// hardcoded `15 * 60` mission time and the class-driven r_bIsPVP fallout
		// in LoadGameConfig / InitGameRepInfo with a DB-driven override that
		// lets two maps of the same TgGame class differ. Concretely: TgGame_Mission
		// covers both PvP merc Breach (3P_Beachhead_P, gameplay_type 1544) and
		// PvE specops (1P_CPFactory01_P, gameplay_type 1553) — class-based
		// resolution can't distinguish them.
		//
		// Defaults match what the C++ used to hardcode: 900s (15 min) and 0 (PvE).
		// Both consumers fall back to class-based defaults when a map is absent
		// from map_game_info, so unseeded maps keep their old behavior.
		const char* kV95_schema =
			"ALTER TABLE map_game_info ADD COLUMN mission_time_secs INTEGER NOT NULL DEFAULT 900;"
			"ALTER TABLE map_game_info ADD COLUMN is_pvp            INTEGER NOT NULL DEFAULT 0;";
		result = sqlite3_exec(db, kV95_schema, nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "v95 schema failed: %s\n", err ? err : "(no msg)");
			if (err) { sqlite3_free(err); err = nullptr; }
		}

		// gameplay_type_value_id taxonomy (asm_data_set_valid_values group 171):
		//   1543-1548 = PVP- {Any,Breach,Control,Aquisition,Payload,Scramble} Merc
		//   1569-1572 = PVP- {Arena, Arena 4v4, Arena 10v10, Warzone}
		// Everything else (PVE-* / bare PVP test / AVA / NULL) stays is_pvp=0.
		const char* kV95_seed =
			"UPDATE map_game_info SET is_pvp = 1 "
			"WHERE gameplay_type_value_id IN (1543,1544,1545,1546,1547,1548,1569,1570,1571,1572);";
		result = sqlite3_exec(db, kV95_seed, nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "v95 is_pvp seed failed: %s\n", err ? err : "(no msg)");
			if (err) { sqlite3_free(err); err = nullptr; }
		} else {
			Logger::Log("db", "v95: added mission_time_secs+is_pvp to map_game_info; flagged PvP merc+arena maps\n");
		}
	}

	if (version < 96) {
		// v96: shorten mission time to 7 minutes for 14 PvP maps (6 Breach,
		// 1 Arena/4v4, 5 Payload, 2 Climate Control). Default stays 15 minutes
		// for everything else.
		const char* kV96_seed =
			"UPDATE map_game_info SET mission_time_secs = 420 "
			"WHERE map_name IN ("
			"  'Climate_Control_P',"
			"  '3P_Climate_Control3_P',"
			"  '3P_Beachhead3_P',"
			"  'Ice_GorgeA01_v2',"
			"  '3P_VolcanoAssault_P',"
			"  '3P_Him_Arena_P',"
			"  'MissileComplex_4v4_P',"
			"  '3P_Beachhead_P',"
			"  '3P_Beachhead2_P',"
			"  'Push_IceFloe_P',"
			"  'push_Ravine_P',"
			"  'Push_IceFloe3_P',"
			"  'Push_Toxicity',"
			"  'Push_Dust_P'"
			");";
		result = sqlite3_exec(db, kV96_seed, nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "v96 7-minute mission-time seed failed: %s\n", err ? err : "(no msg)");
			if (err) { sqlite3_free(err); err = nullptr; }
		} else {
			Logger::Log("db", "v96: set mission_time_secs=420 on 14 short-format PvP maps\n");
		}
	}

	if (version < 97) {
		// v97: per-map overtime config on map_game_info. Replaces the hardcoded
		// `m_fGameOvertimeTime = 4*60; m_bAllowOvertime = 1` in LoadGameConfig /
		// InitGameRepInfo with DB-driven values.
		//
		// Defaults are 0/false ("no overtime") so that any seeded map omitted
		// from the explicit lists below ends up without overtime — matches the
		// rule "everything else no overtime allowed". Unseeded maps (no row in
		// map_game_info) still fall back to the legacy hardcoded behavior
		// (4 min / allowed) inside the C++ consumers, preserving the prior
		// instruction to keep fallback semantics intact.
		const char* kV97_schema =
			"ALTER TABLE map_game_info ADD COLUMN overtime_secs  INTEGER NOT NULL DEFAULT 0;"
			"ALTER TABLE map_game_info ADD COLUMN allow_overtime INTEGER NOT NULL DEFAULT 0;";
		result = sqlite3_exec(db, kV97_schema, nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "v97 schema failed: %s\n", err ? err : "(no msg)");
			if (err) { sqlite3_free(err); err = nullptr; }
		}

		// (1) Same 14 maps that v96 set to 7-min missions also get 3-min
		//     overtime + overtime allowed.
		const char* kV97_listed =
			"UPDATE map_game_info SET overtime_secs = 180, allow_overtime = 1 "
			"WHERE map_name IN ("
			"  'Climate_Control_P',"
			"  '3P_Climate_Control3_P',"
			"  '3P_Beachhead3_P',"
			"  'Ice_GorgeA01_v2',"
			"  '3P_VolcanoAssault_P',"
			"  '3P_Him_Arena_P',"
			"  'MissileComplex_4v4_P',"
			"  '3P_Beachhead_P',"
			"  '3P_Beachhead2_P',"
			"  'Push_IceFloe_P',"
			"  'push_Ravine_P',"
			"  'Push_IceFloe3_P',"
			"  'Push_Toxicity',"
			"  'Push_Dust_P'"
			");";
		result = sqlite3_exec(db, kV97_listed, nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "v97 listed-maps overtime seed failed: %s\n", err ? err : "(no msg)");
			if (err) { sqlite3_free(err); err = nullptr; }
		}

		// (2) All PointRotation maps (Scramble + 4v4 Arena rotation) also get
		//     3-min overtime + allowed. No overlap with the 14-list — the
		//     listed maps are all Mission/Escort. Driven off game_class so any
		//     future PointRotation seed inherits the rule without revisiting
		//     this migration.
		const char* kV97_pointrotation =
			"UPDATE map_game_info SET overtime_secs = 180, allow_overtime = 1 "
			"WHERE game_class = 'TgGame.TgGame_PointRotation';";
		result = sqlite3_exec(db, kV97_pointrotation, nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "v97 pointrotation overtime seed failed: %s\n", err ? err : "(no msg)");
			if (err) { sqlite3_free(err); err = nullptr; }
		} else {
			Logger::Log("db", "v97: added overtime_secs+allow_overtime; enabled 3-min overtime on 14 listed + all PointRotation maps\n");
		}
	}

	if (version < 98) {
		// map_planner output for 1P_CPFactory02_P (2026-06-02). Same bundled
		// shape as v68/v70/v72/v74: bot-factory + objective task_force/team_number
		// pairs, spawn_table_id / default_spawn_table_id per factory, the
		// specops pool entry, and the map_game_info row.
		const char* kV98_cpfactory02 =
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES"
			" ('1P_CPFactory02_P', 12340, 's_n_team_number',          '1',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 12340, 's_n_task_force',           '1',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 12322, 'm_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 11717, 'm_n_task_force',           '1',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 8990,  's_n_team_number',          '1',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 8990,  's_n_task_force',           '1',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 12319, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 12319, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 12320, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 12320, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 12321, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 12321, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 12428, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 12428, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 12427, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 12427, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 11670, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 11670, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 11671, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 11671, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 11672, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 11672, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 11673, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 11673, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 11674, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 11674, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 11675, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 11675, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 11676, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 11676, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 11677, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 11677, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 11678, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 11678, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 11679, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 11679, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 11692, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 11692, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 11806, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 11806, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 11808, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 11808, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 11895, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 11895, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 11918, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 11918, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 12104, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 12104, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 12105, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 12105, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 12109, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 12109, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 12110, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 12110, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 12258, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 12258, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 12318, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 12318, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 12324, 's_n_task_force',           '2',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 12324, 's_n_team_number',          '2',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 11716, 'm_n_priority',             '1',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 12340, 'm_n_priority',             '1',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 12340, 's_b_auto_spawn',           '0',  NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 11670, 'n_spawn_table_id',         '28', NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 11670, 'n_default_spawn_table_id', '28', NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 11671, 'n_default_spawn_table_id', '28', NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 11671, 'n_spawn_table_id',         '28', NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 11672, 'n_spawn_table_id',         '46', NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 11672, 'n_default_spawn_table_id', '46', NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 11673, 'n_spawn_table_id',         '28', NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 11673, 'n_default_spawn_table_id', '28', NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 11674, 'n_spawn_table_id',         '33', NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 11674, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 11675, 'n_spawn_table_id',         '33', NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 11675, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 11676, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 11676, 'n_spawn_table_id',         '33', NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 11677, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 11677, 'n_spawn_table_id',         '33', NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 11678, 'n_spawn_table_id',         '33', NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 11678, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 11679, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 11679, 'n_spawn_table_id',         '33', NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 11692, 'n_default_spawn_table_id', '34', NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 11692, 'n_spawn_table_id',         '34', NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 11806, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 11806, 'n_spawn_table_id',         '33', NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 11808, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 11808, 'n_spawn_table_id',         '33', NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 11895, 'n_default_spawn_table_id', '59', NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 11895, 'n_spawn_table_id',         '59', NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 11918, 'n_default_spawn_table_id', '59', NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 11918, 'n_spawn_table_id',         '59', NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 12104, 'n_default_spawn_table_id', '29', NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 12104, 'n_spawn_table_id',         '29', NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 12105, 'n_default_spawn_table_id', '34', NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 12105, 'n_spawn_table_id',         '34', NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 12109, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 12109, 'n_spawn_table_id',         '33', NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 12110, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 12110, 'n_spawn_table_id',         '33', NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 12258, 'n_default_spawn_table_id', '59', NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 12258, 'n_spawn_table_id',         '59', NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 12318, 'n_default_spawn_table_id', '28', NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 12318, 'n_spawn_table_id',         '28', NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 12324, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 12324, 'n_spawn_table_id',         '33', NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 8990,  's_n_device_id',            '2801', NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 12427, 's_n_device_id',            '2238', NULL, NULL, 1),"
			" ('1P_CPFactory02_P', 12428, 's_n_device_id',            '2238', NULL, NULL, 1);";
		result = sqlite3_exec(db, kV98_cpfactory02, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v98 (map_object_config cpfactory02): %s\n", err); return; }

		// Add the map to the specops pool (map_pool_id=1 — see v65 for context).
		const char* kV98_specops_pool =
			"INSERT OR IGNORE INTO ga_map_pool_entries (map_pool_id, map_name, game_mode) VALUES"
			" (1, '1P_CPFactory02_P', 'TgGame.TgGame_Mission');";
		result = sqlite3_exec(db, kV98_specops_pool, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v98 (specops pool): %s\n", err); return; }

		// map_game_info row so the loading screen + friendly name resolve.
		// friendly_name_msg_id 36545 = "Android Assembly Plant"
		// entry_background_image_res_id 6512 = HUD_MissionLoads.PvE_CP.1P_CPFactory02_P
		const char* kV98_map_game_info =
			"INSERT OR IGNORE INTO map_game_info (map_game_id, map_name, game_class, gameplay_type_value_id, friendly_name_msg_id, entry_background_image_res_id) VALUES"
			" (1300, '1P_CPFactory02_P', 'TgGame.TgGame_Mission', 1553, 36545, 6512);";
		result = sqlite3_exec(db, kV98_map_game_info, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v98 (map_game_info): %s\n", err); return; }

		Logger::Log("db", "v98: seeded map_object_config + specops pool + map_game_info for 1P_CPFactory02_P\n");
	}
	if (version < 99) {
		const char* kV99_1v1_queue =
			"UPDATE ga_queues SET queue_id = 7 "
			"WHERE queue_id = (SELECT MIN(queue_id) FROM ga_queues WHERE name = '1v1') "
			"AND NOT EXISTS (SELECT 1 FROM ga_queues WHERE queue_id = 7);"
			"DELETE FROM ga_queues WHERE name = '1v1' AND queue_id <> 7;"
			"INSERT OR IGNORE INTO ga_queues (queue_id, name, taskforce_policy, continue_in_queue, enabled, queue_type_value_id, status_msg_id, name_msg_id, desc_msg_id, icon_id, max_players_per_side, min_players_per_team, max_players_per_team, level_min, level_max, tab, map_x, map_y, map_active_flag, map_icon_texture_res_id, video_res_id, location_value_id, double_agent_flag, sys_site_id, sort_order, bonus_queue_flag, difficulty_value_id, access_flags, active_flag, locked_flag, map_pool_id, min_players_to_pop, max_players_per_instance, pop_delay_seconds) VALUES "
			"(7, '1v1', 'balanced_pvp', 0, 1, 1421, 0, 22671, 22671, 532, 1, 1, 1, 5, 50, 443, 6, 0, 1, 5126, 0, 1477, 1, 0, 1, 0, 0, 0, 1, 0, 2, 2, 2, 0);"
			"UPDATE ga_queues SET name = '1v1', taskforce_policy = 'balanced_pvp', continue_in_queue = 0, enabled = 1, queue_type_value_id = 1421, status_msg_id = 0, name_msg_id = 22671, desc_msg_id = 22671, icon_id = 532, max_players_per_side = 1, min_players_per_team = 1, max_players_per_team = 1, level_min = 5, level_max = 50, tab = 443, map_x = 6, map_y = 0, map_active_flag = 1, map_icon_texture_res_id = 5126, video_res_id = 0, location_value_id = 1477, double_agent_flag = 1, sys_site_id = 0, sort_order = 1, bonus_queue_flag = 0, difficulty_value_id = 0, access_flags = 0, active_flag = 1, locked_flag = 0, map_pool_id = 2, min_players_to_pop = 2, max_players_per_instance = 2, pop_delay_seconds = 0 WHERE queue_id = 7 AND name = '1v1';";
		result = sqlite3_exec(db, kV99_1v1_queue, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v99 (1v1 queue): %s\n", err); return; }

		Logger::Log("db", "v99: seeded 1v1 queue\n");
	}
	if (version < 100) {
		const char* kV100_vr_loading_screen =
			"UPDATE map_game_info "
			"SET entry_background_image_res_id = 4985 "
			"WHERE map_game_id = 100005 "
			"   OR map_name IN ('Dome3_VR_Arena_P', 'Dome3_VR_Arena_P_reserved');";
		result = sqlite3_exec(db, kV100_vr_loading_screen, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v100 (VR loading screen): %s\n", err); return; }

		Logger::Log("db", "v100: retargeted Virtual Arena loading screen\n");
	}
	if (version < 101) {
		// v101: arm the VR home-map heal pad via map_object_config instead of
		// the hardcoded fallback in TgDeviceVolume::setupDevice. The map ships
		// the TgDeviceVolume actor (m_nMapObjectId=11041) with editconst
		// s_nDeviceId=0, so the pad never armed. 2064 is the Medical Station
		// pulse (instant heal, target_fx_id=432). Idempotent: INSERT only if
		// no matching row exists yet.
		const char* kV101_vr_heal_pad =
			"INSERT INTO map_object_config "
			"  (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) "
			"SELECT 'Dome3_VR_Arena_P', 11041, 's_n_device_id', '2064', NULL, NULL, 1 "
			"WHERE NOT EXISTS ("
			"  SELECT 1 FROM map_object_config "
			"  WHERE map_name = 'Dome3_VR_Arena_P' "
			"    AND map_object_id = 11041 "
			"    AND column_name = 's_n_device_id'"
			");";
		result = sqlite3_exec(db, kV101_vr_heal_pad, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v101 (VR heal pad): %s\n", err); return; }

		Logger::Log("db", "v101: seeded VR heal pad device override (Dome3_VR_Arena_P/11041)\n");
	}
	if (version < 102) {
		// v102: switch the VR heal pad device from 2064 (Medical Station, +154
		// instant heal with morale side-effects) to 5134 (ER-2 Heal — single-
		// mode device, flat +100 instant heal, target_type=213 Friend-and-Self,
		// no FX, no HUD icon). 5134's first mode is exactly the +100 effect, so
		// TgDeviceVolume::setupDevice's m_FireMode[0] pick lands on it cleanly.
		const char* kV102_vr_heal_pad =
			"UPDATE map_object_config "
			"SET value = '5134' "
			"WHERE map_name = 'Dome3_VR_Arena_P' "
			"  AND map_object_id = 11041 "
			"  AND column_name = 's_n_device_id';";
		result = sqlite3_exec(db, kV102_vr_heal_pad, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v102 (VR heal pad retarget): %s\n", err); return; }

		Logger::Log("db", "v102: retargeted VR heal pad to device 5134 (ER-2 Heal, +100 flat)\n");
	}
	if (version < 103) {
		// v103: PvE spawn-table + map-object fixes.
		//   * Mobile-grinder spawn table 10002 (difficulty 1467 = Easy?, bot 1428)
		//     for the Alumina Mine grinder objective points.
		//   * Waste Management Center (1P_CPFactory05_P) defender-spawn instakill fix.
		//   * Alumina Mine (1P_CPMine05_P) grinder objective points point at 10002.
		//   * Weapon Manufacturing Plant (1P_CPFactory01_P) spawn-instakill + lava fix.
		//   * Sector 20 (1P_CPLab01_P) beacon priority fix.
		//   * Android Assembly Plant (1P_CPFactory02_P) beacon team/task-force fix.
		// INSERTs guarded with NOT EXISTS on the natural key so the migration is
		// idempotent across re-runs (matches the v101 pattern). UPDATEs by id are
		// inherently idempotent.

		const char* kV103_grinder_spawn_table =
			"INSERT INTO mod_data_set_bot_spawn_tables "
			"  (bot_spawn_table_id, difficulty_value_id, player_profile_id, "
			"   spawn_group, enemy_bot_id, bot_count, "
			"   spawn_chance, team_size, multiple_class_flag, "
			"   bot_balance_multiplier, spawn_group_min, spawn_group_max, "
			"   spawn_group_respawn_sec) "
			"SELECT 10002, 1467, 0, 1, 1428, 1, 1.0, 0, 0, 1.0, 0, 0, 0 "
			"WHERE NOT EXISTS ("
			"  SELECT 1 FROM mod_data_set_bot_spawn_tables "
			"  WHERE bot_spawn_table_id = 10002 "
			"    AND difficulty_value_id = 1467 "
			"    AND player_profile_id = 0 "
			"    AND spawn_group = 1 "
			"    AND enemy_bot_id = 1428"
			");";
		result = sqlite3_exec(db, kV103_grinder_spawn_table, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v103 (grinder spawn table): %s\n", err); return; }

		const char* kV103_map_object_config_updates =
			// Waste Management Center defender-spawn instakill fix.
			"UPDATE map_object_config SET map_name = '1P_CPFactory05_P', value = '2801', variant_group = NULL, variant_id = NULL, weight = 1 WHERE id = 2117;"
			// Alumina Mine grinder spawn points -> spawn table 10002.
			"UPDATE map_object_config SET map_name = '1P_CPMine05_P', value = '10002', variant_group = NULL, variant_id = NULL, weight = 1 WHERE id = 1745;"
			"UPDATE map_object_config SET map_name = '1P_CPMine05_P', value = '10002', variant_group = NULL, variant_id = NULL, weight = 1 WHERE id = 1744;"
			"UPDATE map_object_config SET map_name = '1P_CPMine05_P', value = '10002', variant_group = NULL, variant_id = NULL, weight = 1 WHERE id = 1746;"
			"UPDATE map_object_config SET map_name = '1P_CPMine05_P', value = '10002', variant_group = NULL, variant_id = NULL, weight = 1 WHERE id = 1747;"
			// Weapon Manufacturing Plant spawn-instakill + lava fix.
			"UPDATE map_object_config SET map_name = '1P_CPFactory01_P', value = '4662', variant_group = NULL, variant_id = NULL, weight = 1 WHERE id = 2093;"
			"UPDATE map_object_config SET map_name = '1P_CPFactory01_P', value = '4662', variant_group = NULL, variant_id = NULL, weight = 1 WHERE id = 2096;"
			"UPDATE map_object_config SET map_name = '1P_CPFactory01_P', value = '2801', variant_group = NULL, variant_id = NULL, weight = 1 WHERE id = 2099;"
			"UPDATE map_object_config SET map_name = '1P_CPFactory01_P', value = '4662', variant_group = NULL, variant_id = NULL, weight = 1 WHERE id = 2102;"
			"UPDATE map_object_config SET map_name = '1P_CPFactory01_P', value = '4662', variant_group = NULL, variant_id = NULL, weight = 1 WHERE id = 2105;"
			"UPDATE map_object_config SET map_name = '1P_CPFactory01_P', value = '4662', variant_group = NULL, variant_id = NULL, weight = 1 WHERE id = 2108;";
		result = sqlite3_exec(db, kV103_map_object_config_updates, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v103 (map_object_config updates): %s\n", err); return; }

		// Sector 20 beacon priority fix.
		const char* kV103_sector20_beacon =
			"INSERT INTO map_object_config "
			"  (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) "
			"SELECT '1P_CPLab01_P', 12286, 'm_n_priority', '1', NULL, NULL, 1 "
			"WHERE NOT EXISTS ("
			"  SELECT 1 FROM map_object_config "
			"  WHERE map_name = '1P_CPLab01_P' "
			"    AND map_object_id = 12286 "
			"    AND column_name = 'm_n_priority'"
			");";
		result = sqlite3_exec(db, kV103_sector20_beacon, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v103 (sector20 beacon): %s\n", err); return; }

		// Android Assembly Plant beacon team/task-force fix.
		const char* kV103_android_beacon =
			"INSERT INTO map_object_config "
			"  (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) "
			"SELECT '1P_CPFactory02_P', 11716, 's_n_team_number', '1', NULL, NULL, 1 "
			"WHERE NOT EXISTS ("
			"  SELECT 1 FROM map_object_config "
			"  WHERE map_name = '1P_CPFactory02_P' "
			"    AND map_object_id = 11716 "
			"    AND column_name = 's_n_team_number'"
			");"
			"INSERT INTO map_object_config "
			"  (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) "
			"SELECT '1P_CPFactory02_P', 11716, 's_n_task_force', '1', NULL, NULL, 1 "
			"WHERE NOT EXISTS ("
			"  SELECT 1 FROM map_object_config "
			"  WHERE map_name = '1P_CPFactory02_P' "
			"    AND map_object_id = 11716 "
			"    AND column_name = 's_n_task_force'"
			");";
		result = sqlite3_exec(db, kV103_android_beacon, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v103 (android beacon): %s\n", err); return; }

		Logger::Log("db", "v103: applied PvE spawn-table + map-object config fixes\n");
	}
	if (version < 104) {
		// v104: 1P_CPLab05_P beacon task-force assignments.
		// Two map objects need m_n_task_force overrides (11717=TF1, 12772=TF2).
		// Idempotent via NOT EXISTS on (map_name, map_object_id, column_name).
		const char* kV104_lab05_beacons =
			"INSERT INTO map_object_config "
			"  (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) "
			"SELECT '1P_CPLab05_P', 11717, 'm_n_task_force', '1', NULL, NULL, 1 "
			"WHERE NOT EXISTS ("
			"  SELECT 1 FROM map_object_config "
			"  WHERE map_name = '1P_CPLab05_P' "
			"    AND map_object_id = 11717 "
			"    AND column_name = 'm_n_task_force'"
			");"
			"INSERT INTO map_object_config "
			"  (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) "
			"SELECT '1P_CPLab05_P', 12772, 'm_n_task_force', '2', NULL, NULL, 1 "
			"WHERE NOT EXISTS ("
			"  SELECT 1 FROM map_object_config "
			"  WHERE map_name = '1P_CPLab05_P' "
			"    AND map_object_id = 12772 "
			"    AND column_name = 'm_n_task_force'"
			");";
		result = sqlite3_exec(db, kV104_lab05_beacons, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v104 (lab05 beacons): %s\n", err); return; }

		Logger::Log("db", "v104: assigned 1P_CPLab05_P beacons to task forces (11717=TF1, 12772=TF2)\n");
	}
	if (version < 105) {
		// v105: 1P_CPLab03 map-object fixes.
		// Two existing rows retargeted to value '59' (UPDATE by id — idempotent),
		// plus a new spawn-table override on map object 12485 (NOT EXISTS guard).
		const char* kV105_lab03_updates =
			"UPDATE map_object_config SET map_name = '1P_CPLab03', value = '59', variant_group = NULL, variant_id = NULL, weight = 1 WHERE id = 1955;"
			"UPDATE map_object_config SET map_name = '1P_CPLab03', value = '59', variant_group = NULL, variant_id = NULL, weight = 1 WHERE id = 1956;";
		result = sqlite3_exec(db, kV105_lab03_updates, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v105 (lab03 updates): %s\n", err); return; }

		const char* kV105_lab03_spawn_table =
			"INSERT INTO map_object_config "
			"  (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) "
			"SELECT '1P_CPLab03', 12485, 's_n_spawn_table_id', '41', NULL, NULL, 1 "
			"WHERE NOT EXISTS ("
			"  SELECT 1 FROM map_object_config "
			"  WHERE map_name = '1P_CPLab03' "
			"    AND map_object_id = 12485 "
			"    AND column_name = 's_n_spawn_table_id'"
			");";
		result = sqlite3_exec(db, kV105_lab03_spawn_table, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v105 (lab03 spawn table): %s\n", err); return; }

		Logger::Log("db", "v105: applied 1P_CPLab03 map-object fixes\n");
	}

	if (version < 106) {
		// v106: seed 1P_CPLab02_P ("Remote Operations Control Center") as the
		// next specops map. Same bundled shape as v68/v70/v72/v74:
		// map_object_config (factory + objective task_force/team_number pairs,
		// spawn_table_id / default_spawn_table_id per factory), specops pool
		// entry, and a map_game_info row so the loading screen + friendly
		// name resolve. friendly_name_msg_id 38898 = "Remote Operations
		// Control Center" (asm_data_set_msg_translations). Background
		// res_id 6517 = HUD_MissionLoads.PvE_CP.1P_CPLab02_P.
		const char* kV106_lab02_map_objects =
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES "
			"  ('1P_CPLab02_P', 12227, 'm_n_task_force',           '1',  NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12190, 'm_n_priority',             '1',  NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12190, 's_n_task_force',           '1',  NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12190, 's_n_team_number',          '1',  NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12286, 's_b_auto_spawn',           '0',  NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12286, 's_n_team_number',          '1',  NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12286, 's_n_task_force',           '1',  NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12286, 'm_n_priority',             '1',  NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12390, 's_n_task_force',           '2',  NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12390, 'n_spawn_table_id',         '29', NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12390, 'n_default_spawn_table_id', '29', NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12432, 's_n_task_force',           '2',  NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12432, 'n_spawn_table_id',         '28', NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12432, 'n_default_spawn_table_id', '28', NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12421, 's_n_task_force',           '2',  NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12421, 'n_spawn_table_id',         '28', NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12421, 'n_default_spawn_table_id', '28', NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12422, 's_n_task_force',           '2',  NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12422, 'n_spawn_table_id',         '28', NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12422, 'n_default_spawn_table_id', '28', NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12423, 'n_default_spawn_table_id', '28', NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12423, 'n_spawn_table_id',         '28', NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12423, 's_n_task_force',           '2',  NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12419, 'n_default_spawn_table_id', '46', NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12419, 'n_spawn_table_id',         '46', NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12419, 's_n_task_force',           '2',  NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12413, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12413, 'n_spawn_table_id',         '33', NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12413, 's_n_task_force',           '2',  NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12414, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12414, 'n_spawn_table_id',         '33', NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12414, 's_n_task_force',           '2',  NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12420, 'n_default_spawn_table_id', '34', NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12420, 'n_spawn_table_id',         '34', NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12420, 's_n_task_force',           '2',  NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12418, 'n_default_spawn_table_id', '59', NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12418, 'n_spawn_table_id',         '59', NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12418, 's_n_task_force',           '2',  NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12415, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12415, 'n_spawn_table_id',         '33', NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12415, 's_n_task_force',           '2',  NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12416, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12416, 'n_spawn_table_id',         '33', NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12416, 's_n_task_force',           '2',  NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12417, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12417, 'n_spawn_table_id',         '33', NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12417, 's_n_task_force',           '2',  NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12444, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12444, 'n_spawn_table_id',         '33', NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12444, 's_n_task_force',           '2',  NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12448, 'n_default_spawn_table_id', '34', NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12448, 'n_spawn_table_id',         '34', NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12448, 's_n_task_force',           '2',  NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12445, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12445, 'n_spawn_table_id',         '33', NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12445, 's_n_task_force',           '2',  NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12446, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12446, 'n_spawn_table_id',         '33', NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12446, 's_n_task_force',           '2',  NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12447, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12447, 'n_spawn_table_id',         '33', NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12447, 's_n_task_force',           '2',  NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12442, 'm_n_task_force',           '2',  NULL, NULL, 1),"
			"  ('1P_CPLab02_P', 12424, 's_n_spawn_table_id',       '41', NULL, NULL, 1);";
		result = sqlite3_exec(db, kV106_lab02_map_objects, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v106 (map_object_config lab02): %s\n", err); return; }

		// Add the map to the specops pool (map_pool_id=1 — see v65 for context).
		const char* kV106_specops_pool =
			"INSERT OR IGNORE INTO ga_map_pool_entries (map_pool_id, map_name, game_mode) VALUES"
			" (1, '1P_CPLab02_P', 'TgGame.TgGame_Mission');";
		result = sqlite3_exec(db, kV106_specops_pool, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v106 (specops pool): %s\n", err); return; }

		// Seed map_game_info so the loading screen + friendly name resolve.
		// map_game_id 1313 reserved for 1P_CPLab02_P. friendly_name_msg_id
		// 38898 = "Remote Operations Control Center". Background res_id 6517
		// = HUD_MissionLoads.PvE_CP.1P_CPLab02_P. Other fields match the
		// shape established in v73.
		const char* kV106_lab02_map_game_info =
			"INSERT OR REPLACE INTO map_game_info (map_game_id, map_name, game_class, gameplay_type_value_id, friendly_name_msg_id, entry_background_image_res_id) VALUES "
			"(1313, '1P_CPLab02_P', 'TgGame.TgGame_Mission', 1553, 38898, 6517);";
		result = sqlite3_exec(db, kV106_lab02_map_game_info, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v106 (map_game_info lab02): %s\n", err); return; }

		Logger::Log("db", "v106: seeded 1P_CPLab02_P map-object config + specops pool entry + map_game_info\n");
	}

	if (version < 107) {
		// v107: seed 1P_CPMine02_P ("Mineral Extraction Site") as the next
		// specops map. Same bundled shape as v106 (1P_CPLab02_P):
		// map_object_config (factory + objective task_force/team_number pairs,
		// spawn_table_id / default_spawn_table_id per factory, one s_n_device_id
		// override), specops pool entry, and a map_game_info row so the loading
		// screen + friendly name resolve. friendly_name_msg_id 37318 =
		// "Mineral Extraction Site A-31" (follows the 1P_CPMine02 map-slug
		// entry 37315 in the friendly-name family). Background res_id 6522 =
		// HUD_MissionLoads.PvE_CP.1P_CPMine02_P.
		const char* kV107_mine02_map_objects =
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES "
			"  ('1P_CPMine02_P', 12227, 'm_n_task_force',           '1',  NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12425, 'm_n_task_force',           '2',  NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12190, 'm_n_priority',             '1',  NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12190, 's_n_task_force',           '1',  NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12190, 's_n_team_number',          '1',  NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12340, 's_n_task_force',           '1',  NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12340, 's_n_team_number',          '1',  NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12340, 's_b_auto_spawn',           '0',  NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12197, 's_n_team_number',          '2',  NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12197, 's_n_task_force',           '2',  NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12197, 'n_spawn_table_id',         '29', NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12197, 'n_default_spawn_table_id', '29', NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12198, 'n_default_spawn_table_id', '40', NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12198, 'n_spawn_table_id',         '40', NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12198, 's_n_task_force',           '2',  NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12198, 's_n_team_number',          '2',  NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12199, 'n_spawn_table_id',         '33', NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12199, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12199, 's_n_task_force',           '2',  NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12199, 's_n_team_number',          '2',  NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12200, 'n_spawn_table_id',         '33', NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12200, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12200, 's_n_task_force',           '2',  NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12200, 's_n_team_number',          '2',  NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12364, 's_n_team_number',          '2',  NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12364, 's_n_task_force',           '2',  NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12364, 'n_spawn_table_id',         '28', NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12364, 'n_default_spawn_table_id', '28', NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12365, 'n_default_spawn_table_id', '28', NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12365, 'n_spawn_table_id',         '28', NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12365, 's_n_task_force',           '2',  NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12365, 's_n_team_number',          '2',  NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12366, 'n_default_spawn_table_id', '58', NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12366, 'n_spawn_table_id',         '58', NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12366, 's_n_task_force',           '2',  NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12366, 's_n_team_number',          '2',  NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12367, 's_n_task_force',           '2',  NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12367, 's_n_team_number',          '2',  NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12367, 'n_spawn_table_id',         '28', NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12367, 'n_default_spawn_table_id', '28', NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12363, 'n_default_spawn_table_id', '34', NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12363, 'n_spawn_table_id',         '34', NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12363, 's_n_task_force',           '2',  NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12363, 's_n_team_number',          '2',  NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12380, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12380, 'n_spawn_table_id',         '33', NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12380, 's_n_task_force',           '2',  NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12380, 's_n_team_number',          '2',  NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12381, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12381, 'n_spawn_table_id',         '33', NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12381, 's_n_task_force',           '2',  NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12381, 's_n_team_number',          '2',  NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12382, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12382, 'n_spawn_table_id',         '33', NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12382, 's_n_task_force',           '2',  NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12382, 's_n_team_number',          '2',  NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12383, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12383, 'n_spawn_table_id',         '33', NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12383, 's_n_task_force',           '2',  NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12383, 's_n_team_number',          '2',  NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12384, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12384, 'n_spawn_table_id',         '33', NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12384, 's_n_task_force',           '2',  NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12384, 's_n_team_number',          '2',  NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12385, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12385, 'n_spawn_table_id',         '33', NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12385, 's_n_task_force',           '2',  NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12385, 's_n_team_number',          '2',  NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12386, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12386, 'n_spawn_table_id',         '33', NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12386, 's_n_task_force',           '2',  NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12386, 's_n_team_number',          '2',  NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12387, 'n_default_spawn_table_id', '33', NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12387, 'n_spawn_table_id',         '33', NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12387, 's_n_task_force',           '2',  NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12387, 's_n_team_number',          '2',  NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12388, 'n_default_spawn_table_id', '59', NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12388, 'n_spawn_table_id',         '59', NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12388, 's_n_task_force',           '2',  NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 12388, 's_n_team_number',          '2',  NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 13386, 'n_spawn_table_id',         '46', NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 13386, 'n_default_spawn_table_id', '46', NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 13386, 's_n_task_force',           '2',  NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 13386, 's_n_team_number',          '2',  NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 8990,  's_n_team_number',          '1',  NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 8990,  's_n_task_force',           '1',  NULL, NULL, 1),"
			"  ('1P_CPMine02_P', 8990,  's_n_device_id',            '2801', NULL, NULL, 1);";
		result = sqlite3_exec(db, kV107_mine02_map_objects, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v107 (map_object_config mine02): %s\n", err); return; }

		// Add the map to the specops pool (map_pool_id=1 — see v65 for context).
		const char* kV107_specops_pool =
			"INSERT OR IGNORE INTO ga_map_pool_entries (map_pool_id, map_name, game_mode) VALUES"
			" (1, '1P_CPMine02_P', 'TgGame.TgGame_Mission');";
		result = sqlite3_exec(db, kV107_specops_pool, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v107 (specops pool): %s\n", err); return; }

		// Seed map_game_info so the loading screen + friendly name resolve.
		// map_game_id 1312 reserved for 1P_CPMine02_P. friendly_name_msg_id
		// 37318 = "Mineral Extraction Site A-31". Background res_id 6522 =
		// HUD_MissionLoads.PvE_CP.1P_CPMine02_P.
		const char* kV107_mine02_map_game_info =
			"INSERT OR REPLACE INTO map_game_info (map_game_id, map_name, game_class, gameplay_type_value_id, friendly_name_msg_id, entry_background_image_res_id) VALUES "
			"(1312, '1P_CPMine02_P', 'TgGame.TgGame_Mission', 1553, 37318, 6522);";
		result = sqlite3_exec(db, kV107_mine02_map_game_info, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v107 (map_game_info mine02): %s\n", err); return; }

		Logger::Log("db", "v107: seeded 1P_CPMine02_P map-object config + specops pool entry + map_game_info\n");
	}

	if (version < 108) {
		// v108: seed 1P_CPMine01_P ("Unobtanium Mine LV-426") as the next
		// specops map. Same bundled shape as v107 (1P_CPMine02_P):
		// map_object_config (factory + objective task_force/team_number pairs,
		// spawn_table_id / default_spawn_table_id per factory, two s_n_device_id
		// overrides), specops pool entry, and a map_game_info row so the loading
		// screen + friendly name resolve. friendly_name_msg_id 36032 =
		// "Unobtanium Mine LV-426". Background res_id 6521 =
		// HUD_MissionLoads.PvE_CP.1P_CPMine01_P.
		const char* kV108_mine01_map_objects =
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES "
			"  ('1P_CPMine01_P', 12227, 'm_n_task_force',           '1',    NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12450, 'm_n_task_force',           '2',    NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 8990,  's_n_device_id',            '2801', NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 8990,  's_n_team_number',          '1',    NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 8990,  's_n_task_force',           '1',    NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12457, 's_n_device_id',            '4662', NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12457, 's_n_team_number',          '2',    NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12457, 's_n_task_force',           '2',    NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12456, 's_n_team_number',          '1',    NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12456, 's_n_task_force',           '1',    NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12456, 'm_n_priority',             '1',    NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12456, 's_b_auto_spawn',           '0',    NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12455, 's_n_team_number',          '1',    NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12455, 's_n_task_force',           '1',    NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12455, 'm_n_priority',             '1',    NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12455, 's_b_auto_spawn',           '0',    NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12526, 'n_priority',               '1',    NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12195, 's_n_team_number',          '2',    NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12195, 's_n_task_force',           '2',    NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12195, 'n_default_spawn_table_id', '28',   NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12195, 'n_spawn_table_id',         '28',   NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12196, 'n_default_spawn_table_id', '28',   NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12196, 'n_spawn_table_id',         '28',   NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12196, 's_n_task_force',           '2',    NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12196, 's_n_team_number',          '2',    NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12197, 'n_default_spawn_table_id', '29',   NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12197, 'n_spawn_table_id',         '29',   NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12197, 's_n_task_force',           '2',    NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12197, 's_n_team_number',          '2',    NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12198, 'n_default_spawn_table_id', '40',   NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12198, 'n_spawn_table_id',         '40',   NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12198, 's_n_task_force',           '2',    NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12198, 's_n_team_number',          '2',    NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12205, 'n_default_spawn_table_id', '28',   NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12205, 'n_spawn_table_id',         '28',   NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12205, 's_n_task_force',           '2',    NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12205, 's_n_team_number',          '2',    NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12204, 'n_default_spawn_table_id', '28',   NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12204, 'n_spawn_table_id',         '28',   NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12204, 's_n_task_force',           '2',    NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12204, 's_n_team_number',          '2',    NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12225, 'n_spawn_table_id',         '59',   NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12225, 'n_default_spawn_table_id', '59',   NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12225, 's_n_task_force',           '2',    NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12225, 's_n_team_number',          '2',    NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12226, 'n_default_spawn_table_id', '59',   NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12226, 'n_spawn_table_id',         '59',   NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12226, 's_n_task_force',           '2',    NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12226, 's_n_team_number',          '2',    NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12199, 's_n_team_number',          '2',    NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12199, 's_n_task_force',           '2',    NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12199, 'n_spawn_table_id',         '33',   NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12199, 'n_default_spawn_table_id', '33',   NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12200, 'n_default_spawn_table_id', '33',   NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12200, 'n_spawn_table_id',         '33',   NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12200, 's_n_task_force',           '2',    NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12200, 's_n_team_number',          '2',    NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12201, 'n_default_spawn_table_id', '33',   NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12201, 'n_spawn_table_id',         '33',   NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12201, 's_n_task_force',           '2',    NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12201, 's_n_team_number',          '2',    NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12202, 's_n_team_number',          '2',    NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12202, 's_n_task_force',           '2',    NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12202, 'n_default_spawn_table_id', '33',   NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12202, 'n_spawn_table_id',         '33',   NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12203, 'n_default_spawn_table_id', '33',   NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12203, 'n_spawn_table_id',         '33',   NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12203, 's_n_task_force',           '2',    NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12203, 's_n_team_number',          '2',    NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12221, 's_n_task_force',           '2',    NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12221, 's_n_team_number',          '2',    NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12221, 'n_spawn_table_id',         '33',   NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12221, 'n_default_spawn_table_id', '33',   NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12222, 'n_default_spawn_table_id', '40',   NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12222, 'n_spawn_table_id',         '40',   NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12222, 's_n_task_force',           '2',    NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12222, 's_n_team_number',          '2',    NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12223, 'n_spawn_table_id',         '33',   NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12223, 's_n_task_force',           '2',    NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12223, 's_n_team_number',          '2',    NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12224, 'n_spawn_table_id',         '33',   NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12224, 'n_default_spawn_table_id', '33',   NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12224, 's_n_task_force',           '2',    NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12224, 's_n_team_number',          '2',    NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12526, 'n_default_spawn_table_id', '46',   NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12526, 'n_spawn_table_id',         '46',   NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12526, 's_n_task_force',           '2',    NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12526, 's_n_team_number',          '2',    NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12223, 'n_default_spawn_table_id', '33',   NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12222, 'b_spawn_on_alarm',         '0',    NULL, NULL, 1),"
			"  ('1P_CPMine01_P', 12450, 'm_n_priority',             '1',    NULL, NULL, 1);";
		result = sqlite3_exec(db, kV108_mine01_map_objects, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v108 (map_object_config mine01): %s\n", err); return; }

		// Add the map to the specops pool (map_pool_id=1 — see v65 for context).
		const char* kV108_specops_pool =
			"INSERT OR IGNORE INTO ga_map_pool_entries (map_pool_id, map_name, game_mode) VALUES"
			" (1, '1P_CPMine01_P', 'TgGame.TgGame_Mission');";
		result = sqlite3_exec(db, kV108_specops_pool, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v108 (specops pool): %s\n", err); return; }

		// Seed map_game_info so the loading screen + friendly name resolve.
		// map_game_id 1293 reserved for 1P_CPMine01_P (production_map_game_list).
		// friendly_name_msg_id 36032 = "Unobtanium Mine LV-426". Background
		// res_id 6521 = HUD_MissionLoads.PvE_CP.1P_CPMine01_P.
		const char* kV108_mine01_map_game_info =
			"INSERT OR REPLACE INTO map_game_info (map_game_id, map_name, game_class, gameplay_type_value_id, friendly_name_msg_id, entry_background_image_res_id) VALUES "
			"(1293, '1P_CPMine01_P', 'TgGame.TgGame_Mission', 1553, 36032, 6521);";
		result = sqlite3_exec(db, kV108_mine01_map_game_info, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v108 (map_game_info mine01): %s\n", err); return; }

		Logger::Log("db", "v108: seeded 1P_CPMine01_P map-object config + specops pool entry + map_game_info\n");
	}

	if (version < 109) {
		// v109: playtest fixes to the v108 1P_CPMine01_P seed. Keyed on the
		// natural (map_object_id, column_name) tuple rather than absolute row
		// id so it resolves on any DB regardless of autoincrement sequence.
		//   - object 12222: drop b_spawn_on_alarm; set spawn tables 40 -> 33.
		//   - object 12456: s_b_auto_spawn 0 -> 1.
		const char* kV109_mine01_fixes =
			"DELETE FROM map_object_config "
			"  WHERE map_name='1P_CPMine01_P' AND map_object_id=12222 AND column_name='b_spawn_on_alarm';"
			"UPDATE map_object_config SET value='33', variant_group=NULL, variant_id=NULL, weight=1 "
			"  WHERE map_name='1P_CPMine01_P' AND map_object_id=12222 AND column_name='n_default_spawn_table_id';"
			"UPDATE map_object_config SET value='33', variant_group=NULL, variant_id=NULL, weight=1 "
			"  WHERE map_name='1P_CPMine01_P' AND map_object_id=12222 AND column_name='n_spawn_table_id';"
			"UPDATE map_object_config SET value='1', variant_group=NULL, variant_id=NULL, weight=1 "
			"  WHERE map_name='1P_CPMine01_P' AND map_object_id=12456 AND column_name='s_b_auto_spawn';";
		result = sqlite3_exec(db, kV109_mine01_fixes, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v109 (mine01 fixes): %s\n", err); return; }

		Logger::Log("db", "v109: applied 1P_CPMine01_P playtest fixes (12222 spawn tables/alarm, 12456 auto_spawn)\n");
	}

	if (version < 110) {
		// v110: seed 1P_CPLab04_P ("Advanced Weaponry Research Lab") as the next
		// specops map. Same bundled shape as v107/v108: map_object_config
		// (factory + objective task_force/team_number pairs, spawn_table_id /
		// default_spawn_table_id per factory, two s_n_device_id overrides),
		// specops pool entry, and a map_game_info row. friendly_name_msg_id
		// 39168 = "Advanced Weaponry Research Lab". Background res_id 6519 =
		// HUD_MissionLoads.PvE_CP.1P_CPLab04_P. map_game_id 1328 reserved via
		// production_map_game_list. The 12588/12609/12610/12611/12612 factories
		// reference three custom bot_spawn_table_ids (10003/10004/10005) seeded
		// into mod_data_set_bot_spawn_tables below.
		const char* kV110_lab04_map_objects =
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES "
			"  ('1P_CPLab04_P', 12616, 'm_n_task_force',           '2',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 11717, 'm_n_task_force',           '1',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 8990,  's_n_device_id',            '2801',  NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 8990,  's_n_task_force',           '1',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 8990,  's_n_team_number',          '1',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12590, 's_n_device_id',            '5079',  NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12590, 's_n_task_force',           '2',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12590, 's_n_team_number',          '2',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12541, 's_n_task_force',           '1',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12541, 's_n_team_number',          '1',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12541, 's_b_auto_spawn',           '0',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12541, 'm_n_priority',             '1',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 11716, 's_n_task_force',           '1',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 11716, 's_n_team_number',          '1',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 11716, 'm_n_priority',             '1',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12651, 's_n_task_force',           '2',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12651, 's_n_team_number',          '2',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12639, 's_n_team_number',          '2',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12639, 's_n_task_force',           '2',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12638, 's_n_team_number',          '2',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12638, 's_n_task_force',           '2',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12637, 's_n_task_force',           '2',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12637, 's_n_team_number',          '2',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12636, 's_n_task_force',           '2',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12636, 's_n_team_number',          '2',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12635, 's_n_task_force',           '2',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12635, 's_n_team_number',          '2',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12634, 's_n_task_force',           '2',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12634, 's_n_team_number',          '2',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12633, 's_n_task_force',           '2',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12633, 's_n_team_number',          '2',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12632, 's_n_task_force',           '2',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12632, 's_n_team_number',          '2',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12631, 's_n_task_force',           '2',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12631, 's_n_team_number',          '2',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12630, 's_n_task_force',           '2',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12630, 's_n_team_number',          '2',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12629, 's_n_task_force',           '2',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12629, 's_n_team_number',          '2',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12642, 's_n_task_force',           '2',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12642, 's_n_team_number',          '2',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12641, 's_n_task_force',           '2',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12641, 's_n_team_number',          '2',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12640, 's_n_task_force',           '2',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12640, 's_n_team_number',          '2',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12615, 's_n_task_force',           '2',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12615, 's_n_team_number',          '2',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12613, 's_n_task_force',           '2',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12613, 's_n_team_number',          '2',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12610, 's_n_task_force',           '2',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12610, 's_n_team_number',          '2',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12611, 's_n_task_force',           '2',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12611, 's_n_team_number',          '2',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12612, 's_n_task_force',           '2',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12612, 's_n_team_number',          '2',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12609, 's_n_task_force',           '2',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12609, 's_n_team_number',          '2',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12608, 's_n_task_force',           '2',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12608, 's_n_team_number',          '2',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12607, 's_n_task_force',           '2',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12607, 's_n_team_number',          '2',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12606, 's_n_task_force',           '2',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12606, 's_n_team_number',          '2',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12592, 's_n_task_force',           '2',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12592, 's_n_team_number',          '2',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12591, 's_n_task_force',           '2',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12591, 's_n_team_number',          '2',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12588, 's_n_task_force',           '2',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12588, 's_n_team_number',          '2',     NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12606, 'n_spawn_table_id',         '29',    NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12606, 'n_default_spawn_table_id', '29',    NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12607, 'n_default_spawn_table_id', '28',    NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12607, 'n_spawn_table_id',         '28',    NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12608, 'n_default_spawn_table_id', '28',    NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12608, 'n_spawn_table_id',         '28',    NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12613, 'n_spawn_table_id',         '28',    NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12613, 'n_default_spawn_table_id', '28',    NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12615, 'n_default_spawn_table_id', '28',    NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12615, 'n_spawn_table_id',         '28',    NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12640, 'n_default_spawn_table_id', '46',    NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12640, 'n_spawn_table_id',         '46',    NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12641, 'n_spawn_table_id',         '34',    NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12641, 'n_default_spawn_table_id', '34',    NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12642, 'n_spawn_table_id',         '34',    NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12642, 'n_default_spawn_table_id', '34',    NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12629, 'n_default_spawn_table_id', '33',    NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12629, 'n_spawn_table_id',         '33',    NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12630, 'n_default_spawn_table_id', '33',    NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12630, 'n_spawn_table_id',         '33',    NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12631, 'n_default_spawn_table_id', '33',    NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12631, 'n_spawn_table_id',         '33',    NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12632, 'n_default_spawn_table_id', '33',    NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12632, 'n_spawn_table_id',         '33',    NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12633, 'n_spawn_table_id',         '33',    NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12633, 'n_default_spawn_table_id', '33',    NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12634, 'n_default_spawn_table_id', '33',    NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12634, 'n_spawn_table_id',         '33',    NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12635, 'n_default_spawn_table_id', '33',    NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12635, 'n_spawn_table_id',         '33',    NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12636, 'n_default_spawn_table_id', '33',    NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12636, 'n_spawn_table_id',         '33',    NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12637, 'n_default_spawn_table_id', '33',    NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12637, 'n_spawn_table_id',         '33',    NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12638, 'n_default_spawn_table_id', '33',    NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12638, 'n_spawn_table_id',         '33',    NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12639, 'n_default_spawn_table_id', '59',    NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12639, 'n_spawn_table_id',         '59',    NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12651, 'n_spawn_table_id',         '34',    NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12651, 'n_default_spawn_table_id', '34',    NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12588, 'n_spawn_table_id',         '10003', NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12588, 'n_default_spawn_table_id', '10003', NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12609, 'n_default_spawn_table_id', '10004', NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12609, 'n_spawn_table_id',         '10004', NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12612, 'n_default_spawn_table_id', '10005', NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12612, 'n_spawn_table_id',         '10005', NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12611, 'n_spawn_table_id',         '10005', NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12611, 'n_default_spawn_table_id', '10005', NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12610, 'n_default_spawn_table_id', '10005', NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12610, 'n_spawn_table_id',         '10005', NULL, NULL, 1);";
		result = sqlite3_exec(db, kV110_lab04_map_objects, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v110 (map_object_config lab04): %s\n", err); return; }

		// Three custom bot spawn tables referenced by the Lab04 firing-range
		// factories (12588/12609/12610/12611/12612). Same column layout as the
		// v90 seed; difficulty 1467, 1 bot, 100% spawn chance.
		//   10003 — 1x bot 1439
		//   10004 — 1x bot 1445
		//   10005 — 1x bot 1438
		const char* kV110_bot_spawn_tables =
			"INSERT INTO mod_data_set_bot_spawn_tables "
			"  (bot_spawn_table_id, difficulty_value_id, player_profile_id, spawn_group, "
			"   enemy_bot_id, bot_count, spawn_chance, team_size, multiple_class_flag, "
			"   bot_balance_multiplier, spawn_group_min, spawn_group_max, spawn_group_respawn_sec) "
			"VALUES "
			"  (10003, 1467, 0, 1, 1439, 1, 1.0, 0, 0, 1.0, 0, 0, 0), "
			"  (10004, 1467, 0, 1, 1445, 1, 1.0, 0, 0, 1.0, 0, 0, 0), "
			"  (10005, 1467, 0, 1, 1438, 1, 1.0, 0, 0, 1.0, 0, 0, 0);";
		result = sqlite3_exec(db, kV110_bot_spawn_tables, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v110 (bot_spawn_tables 10003-10005): %s\n", err); return; }

		// Add the map to the specops pool (map_pool_id=1 — see v65 for context).
		const char* kV110_specops_pool =
			"INSERT OR IGNORE INTO ga_map_pool_entries (map_pool_id, map_name, game_mode) VALUES"
			" (1, '1P_CPLab04_P', 'TgGame.TgGame_Mission');";
		result = sqlite3_exec(db, kV110_specops_pool, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v110 (specops pool): %s\n", err); return; }

		// Seed map_game_info so the loading screen + friendly name resolve.
		const char* kV110_lab04_map_game_info =
			"INSERT OR REPLACE INTO map_game_info (map_game_id, map_name, game_class, gameplay_type_value_id, friendly_name_msg_id, entry_background_image_res_id) VALUES "
			"(1328, '1P_CPLab04_P', 'TgGame.TgGame_Mission', 1553, 39168, 6519);";
		result = sqlite3_exec(db, kV110_lab04_map_game_info, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v110 (map_game_info lab04): %s\n", err); return; }

		Logger::Log("db", "v110: seeded 1P_CPLab04_P map-object config + bot spawn tables 10003-10005 + specops pool entry + map_game_info\n");
	}

	if (version < 111) {
		// v111: face the 1P_CPLab04_P firing-range turret factories 12588 and
		// 12609 east. SpawnNextBot defaults a spawn to its TgBotStart nav
		// point's yaw; those nav points are all yaw 0 (north), so without an
		// override every emplacement faces north. `spawn_rotation_yaw` is the
		// new per-factory override SpawnNextBot reads (UE3 yaw units, 0..65535).
		// 16384 = +90 degrees (east, if north is +X). NOTE: if these come out
		// facing WEST in playtest, flip to 49152 (-90 degrees).
		const char* kV111_lab04_turret_rotation =
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES "
			"  ('1P_CPLab04_P', 12588, 'spawn_rotation_yaw', '16384', NULL, NULL, 1),"
			"  ('1P_CPLab04_P', 12609, 'spawn_rotation_yaw', '16384', NULL, NULL, 1);";
		result = sqlite3_exec(db, kV111_lab04_turret_rotation, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v111 (lab04 turret rotation): %s\n", err); return; }

		Logger::Log("db", "v111: set spawn_rotation_yaw=16384 (east) for 1P_CPLab04_P factories 12588, 12609\n");
	}
	if (version < 112) {
		// v112: b_respawn=0 for every configured MISSION bot factory. The map
		// packages bake the class default (bRespawn=true), which is correct
		// for open-world trickle/defense factories but wrong for missions —
		// with the factory rewrite, the intact BotDied schedules an immediate
		// replacement for every killed mob (mission tables have
		// spawn_group_respawn_sec=0), making trash and alarm waves endless.
		// Source the factory list from map_object_config itself (rows with an
		// n_spawn_table_id config ARE the configured bot factories; map_*
		// dumps only exist for a few maps). map_object_id is NOT unique
		// across maps — match (map_object_id, map_name) pairs NULL-safely.
		// Raid_DomeCityDefense_P is the only non-mission configured map and
		// keeps the default (1).
		const char* kV112_mission_no_respawn =
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) "
			"SELECT DISTINCT s.map_name, s.map_object_id, 'b_respawn', '0', NULL, NULL, 1 "
			"FROM map_object_config s "
			"WHERE s.column_name = 'n_spawn_table_id' "
			"  AND (s.map_name IS NULL OR s.map_name <> 'Raid_DomeCityDefense_P') "
			"  AND NOT EXISTS (SELECT 1 FROM map_object_config e "
			"                  WHERE e.map_object_id = s.map_object_id "
			"                    AND e.column_name = 'b_respawn' "
			"                    AND e.map_name IS s.map_name);";
		result = sqlite3_exec(db, kV112_mission_no_respawn, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v112 (mission b_respawn): %s\n", err); return; }

		Logger::Log("db", "v112: b_respawn=0 for all configured mission bot factories\n");
	}
	if (version < 113) {
		// v113: defense-map factories ship DORMANT. With map defaults
		// (bAutoSpawn=1, nPriority=0), UC PostBeginPlay auto-spawns all 55
		// Raid factories' full rosters at map load (~hundreds of enemies),
		// and b_respawn=1 makes every kill schedule an immediate replacement.
		// Defense waves are driven solely by TgGame_Defense TickWaveNodes,
		// which wakes a factory (bAutoSpawn=1) and rolls a fresh wave roster
		// when its previous wave finished spawning. Same sourcing rules as
		// v112 (configured factories = n_spawn_table_id rows; NULL-safe
		// (map_object_id, map_name) pairs).
		const char* kV113_defense_dormant =
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) "
			"SELECT DISTINCT s.map_name, s.map_object_id, c.column_name, '0', NULL, NULL, 1 "
			"FROM map_object_config s "
			"JOIN (SELECT 'b_auto_spawn' AS column_name UNION ALL SELECT 'b_respawn') c "
			"WHERE s.column_name = 'n_spawn_table_id' "
			"  AND s.map_name = 'Raid_DomeCityDefense_P' "
			"  AND NOT EXISTS (SELECT 1 FROM map_object_config e "
			"                  WHERE e.map_object_id = s.map_object_id "
			"                    AND e.column_name = c.column_name "
			"                    AND e.map_name IS s.map_name);";
		result = sqlite3_exec(db, kV113_defense_dormant, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v113 (defense dormant factories): %s\n", err); return; }

		Logger::Log("db", "v113: b_auto_spawn=0 + b_respawn=0 for Raid_DomeCityDefense_P factories\n");
	}
	if (version < 114) {
		// v114: add the custom "Super Agent" PvE queue. Same kind as the security
		// tiers (queue_type 1021, pinned_1, own_match, specops map pool) — a clone
		// of `umax` with its own name/desc strings and difficulty_value_id 10000
		// (which drives SuperAgent::IsActive on the game side). marshal_difficulty
		// _value_id=1471 so the client renders it as Ultra-Max (it has no label
		// for the custom 10000 and won't list the queue otherwise). Slot it
		// between Ultra-Max (sort_order 4) and the Double Agent queues, shifting
		// those down one. Also bump Ultra-Max's location_value_id to 1483.
		// Explicit values = idempotent.
		const char* kV114_queue_fixups =
			"UPDATE ga_queues SET sort_order = 6 WHERE name = 'double_agent_high';"
			"UPDATE ga_queues SET sort_order = 7 WHERE name = 'double_agent_max';"
			"UPDATE ga_queues SET sort_order = 8 WHERE name = 'double_agent_umax';"
			"UPDATE ga_queues SET location_value_id = 1483 WHERE name = 'umax';";
		result = sqlite3_exec(db, kV114_queue_fixups, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v114 (queue fixups): %s\n", err); return; }

		const char* kV114_super_agent_queue =
			"INSERT OR IGNORE INTO ga_queues "
			"(queue_id, name, taskforce_policy, continue_in_queue, enabled, queue_type_value_id, "
			" status_msg_id, name_msg_id, desc_msg_id, icon_id, max_players_per_side, "
			" min_players_per_team, max_players_per_team, level_min, level_max, tab, map_x, map_y, "
			" map_active_flag, map_icon_texture_res_id, video_res_id, location_value_id, "
			" double_agent_flag, sys_site_id, sort_order, bonus_queue_flag, difficulty_value_id, "
			" access_flags, active_flag, locked_flag, map_pool_id, min_players_to_pop, "
			" max_players_per_instance, pop_delay_seconds, pop_delay_policy, instant_pop_when_full, "
			" marshal_difficulty_value_id, requires_pvp_verification, team_policy, team_side_policy, "
			" max_team_size) VALUES "
			"(11, 'super_agent', 'pinned_1', 0, 1, 1021, 0, 55411, 55410, 537, 30, 1, 30, 5, 200, "
			" 443, 6.0, 0.0, 1, 5126, 0, 1477, 1, 0, 5, 0, 10000, 0, 1, 0, 1, 1, 0, 0.0, "
			" 'halve_on_join', 1, 1471, 0, 'own_match', 'required', 0);";
		result = sqlite3_exec(db, kV114_super_agent_queue, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v114 (super agent queue): %s\n", err); return; }

		Logger::Log("db", "v114: added Super Agent queue (difficulty 10000, marshal 1471, specops pool, sort_order 5) + umax location 1483\n");
	}

	if (version < 115) {
		// v115: CTR_Recursive_P spawns everyone in one room regardless of task
		// force. Assign the two spawn-room objects to opposing task forces so
		// attackers (TF1) and defenders (TF2) spawn separately, matching the
		// CTR/DualCTF layout used by the released CTR_DuelStrike maps.
		const char* kV115_recursive_spawn_task_forces =
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES"
			"  ('CTR_Recursive_P', 13880, 'm_n_task_force', '1', NULL, NULL, 1),"
			"  ('CTR_Recursive_P', 13881, 'm_n_task_force', '2', NULL, NULL, 1);";
		result = sqlite3_exec(db, kV115_recursive_spawn_task_forces, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v115 (recursive spawn task forces): %s\n", err); return; }

		Logger::Log("db", "v115: assigned CTR_Recursive_P spawn rooms 13880=TF1, 13881=TF2\n");
	}

	if (version < 116) {
		// v116: CTR_DuelStrike3_P and CTR_DuelStrike_P had their friendly names
		// crossed — DuelStrike3 showed "Kimerial Point" (msg 36824) and
		// DuelStrike_P showed "Hart Station" (msg 35942). Swap so DuelStrike3 =
		// Hart Station. The msg strings live in read-only asm_data_set_msg_
		// translations; only the map_game_info references move. Both maps already
		// share loading screen 5146 (loading_dualstrike_a), so there is no screen
		// to swap. Explicit values = idempotent.
		const char* kV116_duelstrike_name_swap =
			"UPDATE map_game_info SET friendly_name_msg_id = 35942 WHERE map_name = 'CTR_DuelStrike3_P';"  // Hart Station
			"UPDATE map_game_info SET friendly_name_msg_id = 36824 WHERE map_name = 'CTR_DuelStrike_P';";   // Kimerial Point
		result = sqlite3_exec(db, kV116_duelstrike_name_swap, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v116 (duelstrike name swap): %s\n", err); return; }

		Logger::Log("db", "v116: swapped names — CTR_DuelStrike3_P=Hart Station(35942), CTR_DuelStrike_P=Kimerial Point(36824)\n");
	}

	if (version < 117) {
		// v117: register the 4 CTR maps as a custom Point-Rotation PvP mode.
		// New map_game_info rows with fabricated ids (100006-100009) leave the
		// originals' client-recognized ids intact; the DLL resolves the right row
		// via LookupByNameAndGameMode(map_name, game_class). Cloned from the Rot_
		// PointRotation maps (gameplay_type 1548, 900s/180s/pvp). friendly_name +
		// bg keep each map's identity (post-v116 DuelStrike name swap). NOTE: the
		// actual PointRotation timer is set by TgGame_Arena's LoadGameConfig hook,
		// not these columns — they match the other Rot_ rows for consistency.
		const char* kV117_mapgameinfo =
			"INSERT OR IGNORE INTO map_game_info "
			"(map_game_id, map_name, game_class, gameplay_type_value_id, friendly_name_msg_id, "
			" entry_background_image_res_id, mission_time_secs, is_pvp, overtime_secs, allow_overtime) VALUES "
			"  (100006, 'CTR_Recursive_P',   'TgGame.TgGame_PointRotation', 1548, 65824, 6064, 900, 1, 180, 1),"
			"  (100007, 'CTR_DuelStrike_P',  'TgGame.TgGame_PointRotation', 1548, 36824, 5146, 900, 1, 180, 1),"  // Kimerial Point
			"  (100008, 'CTR_DuelStrike2_P', 'TgGame.TgGame_PointRotation', 1548, 34477, 5713, 900, 1, 180, 1),"
			"  (100009, 'CTR_DuelStrike3_P', 'TgGame.TgGame_PointRotation', 1548, 35942, 5146, 900, 1, 180, 1);"; // Hart Station
		result = sqlite3_exec(db, kV117_mapgameinfo, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v117 (cp map_game_info): %s\n", err); return; }

		// Add to the pvp ("merc") pool (map_pool_id=2), matching the Rot_
		// PointRotation entries (weight 10, enabled, min_players 9, no max).
		const char* kV117_pool =
			"INSERT OR IGNORE INTO ga_map_pool_entries "
			"(map_pool_id, map_name, game_mode, weight, enabled, min_players, max_players) VALUES "
			"  (2, 'CTR_Recursive_P',   'TgGame.TgGame_PointRotation', 10, 1, 9, NULL),"
			"  (2, 'CTR_DuelStrike_P',  'TgGame.TgGame_PointRotation', 10, 1, 9, NULL),"
			"  (2, 'CTR_DuelStrike2_P', 'TgGame.TgGame_PointRotation', 10, 1, 9, NULL),"
			"  (2, 'CTR_DuelStrike3_P', 'TgGame.TgGame_PointRotation', 10, 1, 9, NULL);";
		result = sqlite3_exec(db, kV117_pool, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v117 (cp pool entries): %s\n", err); return; }

		Logger::Log("db", "v117: registered 4 CTR maps as custom PointRotation (ids 100006-100009) + added to pvp pool\n");
	}

	if (version < 118) {
		// v118: assign opposing task forces to the two spawn-room objects on the
		// three CTR_DuelStrike maps (10394=TF2, 10395=TF1) so attackers and
		// defenders spawn separately — the same fix v115 applied to
		// CTR_Recursive_P's 13880/13881. Needed now that these maps run the custom
		// two-sided PointRotation PvP mode (v117).
		const char* kV118_duelstrike_spawn_task_forces =
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES"
			"  ('CTR_DuelStrike_P',  10394, 'm_n_task_force', '2', NULL, NULL, 1),"
			"  ('CTR_DuelStrike_P',  10395, 'm_n_task_force', '1', NULL, NULL, 1),"
			"  ('CTR_DuelStrike2_P', 10394, 'm_n_task_force', '2', NULL, NULL, 1),"
			"  ('CTR_DuelStrike2_P', 10395, 'm_n_task_force', '1', NULL, NULL, 1),"
			"  ('CTR_DuelStrike3_P', 10394, 'm_n_task_force', '2', NULL, NULL, 1),"
			"  ('CTR_DuelStrike3_P', 10395, 'm_n_task_force', '1', NULL, NULL, 1);";
		result = sqlite3_exec(db, kV118_duelstrike_spawn_task_forces, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v118 (duelstrike spawn task forces): %s\n", err); return; }

		Logger::Log("db", "v118: assigned CTR_DuelStrike{,2,3}_P spawn rooms 10394=TF2, 10395=TF1\n");
	}

	if (version < 119) {
		// v119: Raid_DomeCityDefense_P spawn-table / objective-priority fixes.
		// Runtime factory + objective config is read from map_object_config (the
		// map_tg_* tables are dump-only). The boss has no rows yet (INSERT); the
		// factory rows already exist with the wrong values (UPDATE).

		// (a) Take the boss objective (13460) out of the priority unlock sequence
		//     so PostBeginPlay's UnlockObjective(1) + TgGame_Defense's per-objective
		//     UnlockObjective() loop skip it (both bail on nPriority<=0). The boss
		//     then spawns only when the kismet "ACTIVATE BOSS" node fires. Relies
		//     on the n_priority read added to TgMissionObjective::LoadObjectConfig.
		const char* kV119_boss_priority =
			"INSERT INTO map_object_config "
			"(map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES"
			"  ('Raid_DomeCityDefense_P', 13460, 'n_priority', '0', NULL, NULL, 1);";
		result = sqlite3_exec(db, kV119_boss_priority, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v119 (boss priority): %s\n", err); return; }

		// (b) The Wave-3 juggernaut DefenseWaveSpawner's only factory is 13809
		//     ("Dummy Bot Factory"), but it was configured with spawn table 102
		//     (Sand Spider) — so wave 3 produced spiders, not juggernauts. Point it
		//     at the juggernaut table (149). Update active + default table ids.
		const char* kV119_jugg =
			"UPDATE map_object_config SET value='149' "
			"WHERE map_name='Raid_DomeCityDefense_P' AND map_object_id=13809 "
			"  AND column_name IN ('n_spawn_table_id','n_default_spawn_table_id');";
		result = sqlite3_exec(db, kV119_jugg, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v119 (13809 juggernaut table): %s\n", err); return; }

		// (c) The end-of-wave "spider swarm" factories (13802-13806, 13810) were on
		//     spawn table 102 (Sand Spider) but are meant to spawn Hunter Spiders
		//     (table 86). Update active + default table ids.
		const char* kV119_spiders =
			"UPDATE map_object_config SET value='86' "
			"WHERE map_name='Raid_DomeCityDefense_P' "
			"  AND map_object_id IN (13802,13803,13804,13805,13806,13810) "
			"  AND column_name IN ('n_spawn_table_id','n_default_spawn_table_id');";
		result = sqlite3_exec(db, kV119_spiders, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v119 (spider swarm table): %s\n", err); return; }

		Logger::Log("db", "v119: DomeCityDefense boss n_priority=0; 13809 spawn table=149 (juggernaut); spider swarm spawn table=86 (Hunter Spider)\n");
	}

	if (version < 120) {
		// v120: round-2 juggernaut. Split out from v119 (already shipped). The
		// round-2 juggernaut is factory 13700 — toggle-driven (SeqAct_Toggle fires
		// 270s after MissionStarted, one-shot guarded), which also fires the
		// "Juggernaut_Spawned" remote event / announcer line. It was configured with
		// spawn table 102 (Sand Spider), so the cue played while a sand spider
		// spawned in place of the juggernaut. Point it at the juggernaut table (149);
		// rows already exist (102) → UPDATE active + default ids.
		const char* kV120_round2_jugg =
			"UPDATE map_object_config SET value='149' "
			"WHERE map_name='Raid_DomeCityDefense_P' AND map_object_id=13700 "
			"  AND column_name IN ('n_spawn_table_id','n_default_spawn_table_id');";
		result = sqlite3_exec(db, kV120_round2_jugg, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v120 (round-2 juggernaut table 13700): %s\n", err); return; }

		Logger::Log("db", "v120: DomeCityDefense round-2 juggernaut 13700 spawn table=149\n");
	}

	if (version < 121) {
		// v121: Raid_DomeCityDefense_P deployable factories. These have no baked
		// per-factory deployable id (selection-list resolution lived in the stripped
		// native), so drive them from map_object_config — read in
		// TgActorFactory::LoadObjectConfig, spawned by TgDeployableFactory::SpawnObject.
		//   13728 -> deployable 217 (DEF_DismantlerLanding, the boss-landing FX), TF1.
		//   13791/13792/13793/13794/13800 -> deployable 218 (DEF_DomeEMP, end-of-wave
		//     EMP), TF2.
		// All are kismet-toggle driven, so s_b_auto_spawn=0 suppresses the
		// PostBeginPlay auto-spawn (the toggle's "Turn On" is the real trigger).
		// No existing rows for these map_object_ids → INSERT.
		const char* kV121_deployables =
			"INSERT INTO map_object_config "
			"(map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES"
			"  ('Raid_DomeCityDefense_P', 13728, 's_n_selected_object_id', '217', NULL, NULL, 1),"
			"  ('Raid_DomeCityDefense_P', 13728, 's_n_task_force',         '1',   NULL, NULL, 1),"
			"  ('Raid_DomeCityDefense_P', 13728, 's_n_team_number',        '1',   NULL, NULL, 1),"
			"  ('Raid_DomeCityDefense_P', 13728, 's_b_auto_spawn',         '0',   NULL, NULL, 1),"
			"  ('Raid_DomeCityDefense_P', 13791, 's_n_selected_object_id', '218', NULL, NULL, 1),"
			"  ('Raid_DomeCityDefense_P', 13791, 's_n_task_force',         '2',   NULL, NULL, 1),"
			"  ('Raid_DomeCityDefense_P', 13791, 's_n_team_number',        '2',   NULL, NULL, 1),"
			"  ('Raid_DomeCityDefense_P', 13791, 's_b_auto_spawn',         '0',   NULL, NULL, 1),"
			"  ('Raid_DomeCityDefense_P', 13792, 's_n_selected_object_id', '218', NULL, NULL, 1),"
			"  ('Raid_DomeCityDefense_P', 13792, 's_n_task_force',         '2',   NULL, NULL, 1),"
			"  ('Raid_DomeCityDefense_P', 13792, 's_n_team_number',        '2',   NULL, NULL, 1),"
			"  ('Raid_DomeCityDefense_P', 13792, 's_b_auto_spawn',         '0',   NULL, NULL, 1),"
			"  ('Raid_DomeCityDefense_P', 13793, 's_n_selected_object_id', '218', NULL, NULL, 1),"
			"  ('Raid_DomeCityDefense_P', 13793, 's_n_task_force',         '2',   NULL, NULL, 1),"
			"  ('Raid_DomeCityDefense_P', 13793, 's_n_team_number',        '2',   NULL, NULL, 1),"
			"  ('Raid_DomeCityDefense_P', 13793, 's_b_auto_spawn',         '0',   NULL, NULL, 1),"
			"  ('Raid_DomeCityDefense_P', 13794, 's_n_selected_object_id', '218', NULL, NULL, 1),"
			"  ('Raid_DomeCityDefense_P', 13794, 's_n_task_force',         '2',   NULL, NULL, 1),"
			"  ('Raid_DomeCityDefense_P', 13794, 's_n_team_number',        '2',   NULL, NULL, 1),"
			"  ('Raid_DomeCityDefense_P', 13794, 's_b_auto_spawn',         '0',   NULL, NULL, 1),"
			"  ('Raid_DomeCityDefense_P', 13800, 's_n_selected_object_id', '218', NULL, NULL, 1),"
			"  ('Raid_DomeCityDefense_P', 13800, 's_n_task_force',         '2',   NULL, NULL, 1),"
			"  ('Raid_DomeCityDefense_P', 13800, 's_n_team_number',        '2',   NULL, NULL, 1),"
			"  ('Raid_DomeCityDefense_P', 13800, 's_b_auto_spawn',         '0',   NULL, NULL, 1);";
		result = sqlite3_exec(db, kV121_deployables, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v121 (deployable factory config): %s\n", err); return; }

		Logger::Log("db", "v121: DomeCityDefense deployable factories — 13728=217/TF1, 13791-13794/13800=218/TF2, auto_spawn=0\n");
	}

	if (version < 122) {
		// v122: Raid_DomeCityDefense_P mission time -> 0 so TgGame_Defense uses
		// its PER-ROUND timer. TgGame_Defense.RoundInProgress.BeginState only runs
		// SetMissionTime(roundTime)+ChangeTimerState(6)+MissionTimerStart() when
		// `m_fGameMissionTime == 0`; with a non-zero overall mission time the
		// per-round countdown state (6) never replicates, and the client's Ava
		// satellite-countdown VO director (Bancroft_Start/HalfwayPoint/30s/10s)
		// — which keys on that round countdown — stays silent. Our LoadGameConfig
		// reads mission_time_secs from this row (and defaults to 900 otherwise),
		// which is what regressed her VO. 0 restores the round timer; round
		// progression itself is unaffected (the 'RoundTimer' SetTimer runs
		// regardless). overtime already 0/disabled for this row.
		const char* kV122 =
			"UPDATE map_game_info SET mission_time_secs = 0 "
			"WHERE map_name = 'Raid_DomeCityDefense_P';";
		result = sqlite3_exec(db, kV122, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v122 (dome mission_time=0): %s\n", err); return; }
		Logger::Log("db", "v122: Raid_DomeCityDefense_P mission_time_secs=0 (enable per-round timer -> Ava satellite VO)\n");
	}

	if (version < 123) {
		// v123: Raid_DomeCityDefense_P bot factories — swap spawn table 102 -> 99.
		// Every factory on this map already carries an explicit n_spawn_table_id /
		// n_default_spawn_table_id override in map_object_config (which is the only
		// source the DLL reads — TgBotFactory::LoadObjectConfig →
		// ApplyFactoryFieldOverrides; map_tg_bot_factory is a write-only dump).
		// Exactly five are at 102 (13635/13659/13665/13694/13709); the rest are
		// already pointed at other tables (86/87/99/147/148/149/166), so a plain
		// value swap of the 102 rows is all that's needed — no inserts.
		result = sqlite3_exec(db,
			"UPDATE map_object_config SET value = '99' "
			"WHERE map_name = 'Raid_DomeCityDefense_P' "
			"  AND column_name IN ('n_spawn_table_id', 'n_default_spawn_table_id') "
			"  AND value = '102';", nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v123 (spawn table 102->99): %s\n", err); return; }

		Logger::Log("db", "v123: DomeCityDefense bot factory spawn table 102 -> 99\n");
	}

	if (version < 124) {
		// v124: revert v119(b) — factory 13809 back to its original table 102
		// (android trash; the v119 comment mislabeled it). 13809 is the wave-3
		// DefenseWaveSpawner's only Target and its role is the between-pulse
		// trash trickle; the wave-3 juggernauts come from the node's Activated
		// kismet chain instead (SeqCond_Increment capped at 2 -> SeqAct_Toggle
		// on factory 13653, table 149): exactly two per wave 3 — pulse 1 at
		// round start, pulse 2 at +90s, pulse 3 blocked by the counter.
		// Pointing 13809 at 149 made the node pulse AND the toggle chain each
		// spawn a juggernaut roster simultaneously (the "two juggernauts at
		// once" bug).
		result = sqlite3_exec(db,
			"UPDATE map_object_config SET value = '102' "
			"WHERE map_name = 'Raid_DomeCityDefense_P' AND map_object_id = 13809 "
			"  AND column_name IN ('n_spawn_table_id', 'n_default_spawn_table_id');",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v124 (13809 back to table 102): %s\n", err); return; }

		Logger::Log("db", "v124: DomeCityDefense wave-3 node dummy factory 13809 spawn table 149 -> 102\n");
	}

	if (version < 125) {
		// v125: Canyon_Defense00 — second defense raid ("Sonoran Raid" pool).
		// Factory archetypes come straight from the kismet dump's SeqVar comments
		// (kismet_Canyon_Defense00.json, "Minion/Spider/Guardian - waveN - facN"),
		// resolved to map_object_ids via the vars' mapObjectId field. Spawn
		// tables reuse the dome-proven per-archetype rosters:
		//   99 = Colony minion trash mix (Drone/Soldier/Wasp/Ant/Sand Spider)
		//   86 = Hunter Spider x2      87 = Colony Guardian x1
		// All factories ship dormant (b_auto_spawn=0 — the DefenseWaveSpawner
		// pulse is the only trigger) and one-roster (b_respawn=0), task force 1
		// (Colony side) — the same shape the dome factory config uses.
		//
		// (a) map_game_info so the map can launch as TgGame_Defense.
		//     54108 = "Canyon Defense", 6635 = HUD_MissionLoads.Defense.DEF_CanyonDefense.
		//     mission_time_secs MUST be 0 (defaults to 900): TgGame_Defense's
		//     RoundInProgress.BeginState only arms the per-round countdown
		//     (SetMissionTime + ChangeTimerState(6)) when m_fGameMissionTime==0 —
		//     see the v122 dome fix.
		result = sqlite3_exec(db,
			"INSERT OR REPLACE INTO map_game_info "
			"(map_game_id, map_name, game_class, gameplay_type_value_id, "
			" friendly_name_msg_id, entry_background_image_res_id, "
			" mission_time_secs, is_pvp, overtime_secs, allow_overtime) VALUES "
			"(100010, 'Canyon_Defense00', 'TgGame.TgGame_Defense', 1550, 54108, 6635, 0, 0, 0, 0);",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v125 (canyon map_game_info): %s\n", err); return; }

		// (b) Bot factory spawn tables. Minion gates -> 99, Spider drops -> 86,
		//     Guardian escorts -> 87 (both active + default table ids).
		const char* kV125_tables =
			"INSERT INTO map_object_config "
			"(map_name, map_object_id, column_name, value, variant_group, variant_id, weight) "
			"SELECT 'Canyon_Defense00', mid, col, tbl, NULL, NULL, 1 "
			"FROM (SELECT 'n_spawn_table_id' AS col UNION ALL SELECT 'n_default_spawn_table_id'), ("
			"  SELECT 13280 AS mid, '99' AS tbl UNION ALL SELECT 13287, '99' UNION ALL"
			"  SELECT 13289, '99' UNION ALL SELECT 13291, '99' UNION ALL"
			"  SELECT 13292, '99' UNION ALL SELECT 13293, '99' UNION ALL"
			"  SELECT 13294, '99' UNION ALL SELECT 13296, '99' UNION ALL"
			"  SELECT 13461, '99' UNION ALL SELECT 13476, '99' UNION ALL"
			"  SELECT 13299, '86' UNION ALL SELECT 13300, '86' UNION ALL"
			"  SELECT 13307, '86' UNION ALL SELECT 13551, '86' UNION ALL"
			"  SELECT 13552, '86' UNION ALL"
			"  SELECT 13297, '87' UNION ALL SELECT 13298, '87' UNION ALL SELECT 13308, '87');";
		result = sqlite3_exec(db, kV125_tables, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v125 (canyon spawn tables): %s\n", err); return; }

		// All 18 factories: dormant + one-roster + Colony side (task force /
		// team 1), matching the dome factory config shape.
		const char* kV125_flags =
			"INSERT INTO map_object_config "
			"(map_name, map_object_id, column_name, value, variant_group, variant_id, weight) "
			"SELECT 'Canyon_Defense00', mid, col, val, NULL, NULL, 1 "
			"FROM (SELECT 'b_auto_spawn' AS col, '0' AS val UNION ALL"
			"      SELECT 'b_respawn', '0' UNION ALL"
			"      SELECT 's_n_task_force', '1' UNION ALL"
			"      SELECT 's_n_team_number', '1'), ("
			"  SELECT 13280 AS mid UNION ALL SELECT 13287 UNION ALL SELECT 13289 UNION ALL"
			"  SELECT 13291 UNION ALL SELECT 13292 UNION ALL SELECT 13293 UNION ALL"
			"  SELECT 13294 UNION ALL SELECT 13296 UNION ALL SELECT 13297 UNION ALL"
			"  SELECT 13298 UNION ALL SELECT 13299 UNION ALL SELECT 13300 UNION ALL"
			"  SELECT 13307 UNION ALL SELECT 13308 UNION ALL SELECT 13461 UNION ALL"
			"  SELECT 13476 UNION ALL SELECT 13551 UNION ALL SELECT 13552);";
		result = sqlite3_exec(db, kV125_flags, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v125 (canyon factory flags): %s\n", err); return; }

		// (c) Boss objective 13459 (Dune Commander, bot 1486, s_b_auto_spawn=0
		//     baked) out of the priority unlock sequence — n_priority=0 so both
		//     unlock paths skip it and only the kismet chain (first Boss Wave
		//     Minions pulse -> CompareBool -> ActivateObjective) spawns it.
		//     Mirrors the dome boss fix (v119a, objective 13460). The defended
		//     NPC 13282 (Backup Generator, tf=2, auto-spawn) needs no config —
		//     its baked values are already correct.
		result = sqlite3_exec(db,
			"INSERT INTO map_object_config "
			"(map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES"
			"  ('Canyon_Defense00', 13459, 'n_priority', '0', NULL, NULL, 1);",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v125 (canyon boss priority): %s\n", err); return; }

		Logger::Log("db", "v125: Canyon_Defense00 defense raid config — map_game_info 100010, "
			"18 factories (minion=99 spider=86 guardian=87, dormant), boss 13459 n_priority=0\n");
	}

	if (version < 126) {
		// v126: Canyon_Defense00 minion gates were ALL on table 99 (the heavy
		// 21-24 bot UMax mix) — every node pulse dumped a worst-case roster
		// (~100 heavy bots/min in wave 1, worse at the faster wave-2/3
		// frequencies) and the server choked. Dome's wave-1 shape — the one
		// that plays right — is mostly cheap swarm gates with a few heavy
		// anchors (5x148 ticks + 3x99). Mirror that ratio:
		//   99  (heavy mix)   keep: 13280, 13289, 13291 (present in all 4 waves)
		//   148 (ant + ticks) ->    13287, 13292, 13293, 13294, 13296
		//   147 (wasps)       ->    13461, 13476 (wave 2-4 flavor gates)
		// Spider (86) / guardian (87) support gates unchanged. NOTE: 147/148
		// have rosters only at 1259/1471 — empty at MedSec (known trap).
		result = sqlite3_exec(db,
			"UPDATE map_object_config SET value = '148' "
			"WHERE map_name = 'Canyon_Defense00' "
			"  AND map_object_id IN (13287, 13292, 13293, 13294, 13296) "
			"  AND column_name IN ('n_spawn_table_id', 'n_default_spawn_table_id');",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v126 (canyon tick gates): %s\n", err); return; }

		result = sqlite3_exec(db,
			"UPDATE map_object_config SET value = '147' "
			"WHERE map_name = 'Canyon_Defense00' "
			"  AND map_object_id IN (13461, 13476) "
			"  AND column_name IN ('n_spawn_table_id', 'n_default_spawn_table_id');",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v126 (canyon wasp gates): %s\n", err); return; }

		Logger::Log("db", "v126: Canyon_Defense00 minion gates rebalanced — "
			"3x99 heavy anchors, 5x148 ticks, 2x147 wasps\n");
	}

	if (version < 127) {
		// v127: Moving_Target00 — third defense raid (Sonoran Raid pool). Same
		// template as Canyon_Defense00 (waves 1-4 minion+support nodes, identical
		// kismet frequencies, Dune Commander boss), except the defended NPC is
		// the "Mobile Power Unit" (bot 1505, rides the mission-long battery
		// matinee). Factory archetypes from kismet_Moving_Target00.json SeqVar
		// comments/mapObjectIds; minion tables use the canyon-playtested ratio
		// (2 heavy 99 gates per wave + 148 ticks / 147 wasps).
		//
		// (a) map_game_info. 59353 = "Pentarch Reactor Facility" (the map's
		//     retail display name; 55200 "Moving Target" is the internal name),
		//     6646 = DEF_MovingTarget. mission_time_secs=0 -> per-round timer
		//     (v122 lesson).
		result = sqlite3_exec(db,
			"INSERT OR REPLACE INTO map_game_info "
			"(map_game_id, map_name, game_class, gameplay_type_value_id, "
			" friendly_name_msg_id, entry_background_image_res_id, "
			" mission_time_secs, is_pvp, overtime_secs, allow_overtime) VALUES "
			"(100011, 'Moving_Target00', 'TgGame.TgGame_Defense', 1550, 59353, 6646, 0, 0, 0, 0);",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v127 (moving target map_game_info): %s\n", err); return; }

		// (b) Spawn tables for the 28 kismet-referenced factories (13374/13378
		//     are unreferenced by any wave node — flags only, no table).
		//     Per-wave minion mix = 2x99 + light swarm, avg ~16 bots/pulse:
		//       w1: 99{13393,13357} 148{13359,13360,13363,13380}
		//       w2: 99{13395,13368} 148{13365,13377,13375} 147{13382}
		//       w3/w4: 99{13370,13368} 148{13375,13550} 147{13373,13382}
		const char* kV127_tables =
			"INSERT INTO map_object_config "
			"(map_name, map_object_id, column_name, value, variant_group, variant_id, weight) "
			"SELECT 'Moving_Target00', mid, col, tbl, NULL, NULL, 1 "
			"FROM (SELECT 'n_spawn_table_id' AS col UNION ALL SELECT 'n_default_spawn_table_id'), ("
			"  SELECT 13357 AS mid, '99' AS tbl UNION ALL SELECT 13368, '99' UNION ALL"
			"  SELECT 13370, '99' UNION ALL SELECT 13393, '99' UNION ALL SELECT 13395, '99' UNION ALL"
			"  SELECT 13359, '148' UNION ALL SELECT 13360, '148' UNION ALL"
			"  SELECT 13363, '148' UNION ALL SELECT 13365, '148' UNION ALL"
			"  SELECT 13375, '148' UNION ALL SELECT 13377, '148' UNION ALL"
			"  SELECT 13380, '148' UNION ALL SELECT 13550, '148' UNION ALL"
			"  SELECT 13373, '147' UNION ALL SELECT 13382, '147' UNION ALL"
			"  SELECT 13361, '86' UNION ALL SELECT 13362, '86' UNION ALL"
			"  SELECT 13364, '86' UNION ALL SELECT 13366, '86' UNION ALL"
			"  SELECT 13369, '86' UNION ALL SELECT 13371, '86' UNION ALL"
			"  SELECT 13376, '86' UNION ALL SELECT 13549, '86' UNION ALL"
			"  SELECT 13367, '87' UNION ALL SELECT 13372, '87' UNION ALL"
			"  SELECT 13379, '87' UNION ALL SELECT 13381, '87' UNION ALL SELECT 13383, '87');";
		result = sqlite3_exec(db, kV127_tables, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v127 (moving target spawn tables): %s\n", err); return; }

		// All 30 factories: dormant + one-roster + Colony side.
		const char* kV127_flags =
			"INSERT INTO map_object_config "
			"(map_name, map_object_id, column_name, value, variant_group, variant_id, weight) "
			"SELECT 'Moving_Target00', mid, col, val, NULL, NULL, 1 "
			"FROM (SELECT 'b_auto_spawn' AS col, '0' AS val UNION ALL"
			"      SELECT 'b_respawn', '0' UNION ALL"
			"      SELECT 's_n_task_force', '1' UNION ALL"
			"      SELECT 's_n_team_number', '1'), ("
			"  SELECT 13357 AS mid UNION ALL SELECT 13359 UNION ALL SELECT 13360 UNION ALL"
			"  SELECT 13361 UNION ALL SELECT 13362 UNION ALL SELECT 13363 UNION ALL"
			"  SELECT 13364 UNION ALL SELECT 13365 UNION ALL SELECT 13366 UNION ALL"
			"  SELECT 13367 UNION ALL SELECT 13368 UNION ALL SELECT 13369 UNION ALL"
			"  SELECT 13370 UNION ALL SELECT 13371 UNION ALL SELECT 13372 UNION ALL"
			"  SELECT 13373 UNION ALL SELECT 13374 UNION ALL SELECT 13375 UNION ALL"
			"  SELECT 13376 UNION ALL SELECT 13377 UNION ALL SELECT 13378 UNION ALL"
			"  SELECT 13379 UNION ALL SELECT 13380 UNION ALL SELECT 13381 UNION ALL"
			"  SELECT 13382 UNION ALL SELECT 13383 UNION ALL SELECT 13393 UNION ALL"
			"  SELECT 13395 UNION ALL SELECT 13549 UNION ALL SELECT 13550);";
		result = sqlite3_exec(db, kV127_flags, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v127 (moving target factory flags): %s\n", err); return; }

		// (c) Boss objective (Dune Commander, bot 1486, s_b_auto_spawn=0) has
		//     map_object_id **0** on this map — the designers never assigned
		//     one. It is the ONLY mid-0 object on the map, and MapObjectConfig
		//     keys overrides by plain int, so a 0-keyed n_priority row lands
		//     exactly on it: out of the unlock sequence, kismet-only activation
		//     (first Boss Wave Minions pulse), same as dome 13460 / canyon 13459.
		//     The defended NPC 13332 (Mobile Power Unit) needs no config.
		result = sqlite3_exec(db,
			"INSERT INTO map_object_config "
			"(map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES"
			"  ('Moving_Target00', 0, 'n_priority', '0', NULL, NULL, 1);",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v127 (moving target boss priority): %s\n", err); return; }

		Logger::Log("db", "v127: Moving_Target00 defense raid config — map_game_info 100011, "
			"30 factories (5x99/8x148/2x147 minions, 8x86 spiders, 5x87 guardians, dormant), "
			"boss (mid 0) n_priority=0\n");
	}

	if (version < 128) {
		// v128: Moving_Target00 display name — the v127 seed shipped 55200
		// ("Moving Target", the internal map name); the retail display name is
		// 59353 "Pentarch Reactor Facility". Repair DBs already at v127.
		result = sqlite3_exec(db,
			"UPDATE map_game_info SET friendly_name_msg_id = 59353 "
			"WHERE map_name = 'Moving_Target00' AND friendly_name_msg_id = 55200;",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v128 (moving target display name): %s\n", err); return; }

		Logger::Log("db", "v128: Moving_Target00 friendly name -> 59353 'Pentarch Reactor Facility'\n");
	}

	if (version < 129) {
		// v129: Oasis_Checkpoint + Raid_Halloween_Oasis_P — Sonoran Raid maps
		// 3 and 4. The Halloween map is a verbatim reskin: both kismets are
		// identical clones (same wave nodes, same frequencies, same
		// map_object_ids for every factory/objective), so both maps share one
		// config set. Only the baked boss/NPC bots differ: Dune Commander
		// (1486) + Backup Generator (1645) vs Doom Commander (1595) + Cage of
		// Souls (1597) — those come from the maps' baked s_n_bot_id, no config
		// needed. No orphan factories; all 12 are kismet-referenced.
		//
		// (a) map_game_info. 39114 "Oasis Checkpoint" / 6638 DEF_OasisCheckpoint;
		//     62626 "Oasis Checkpoint... of DOOM" / 7175 DEF_HalloweenOasis.
		result = sqlite3_exec(db,
			"INSERT OR REPLACE INTO map_game_info "
			"(map_game_id, map_name, game_class, gameplay_type_value_id, "
			" friendly_name_msg_id, entry_background_image_res_id, "
			" mission_time_secs, is_pvp, overtime_secs, allow_overtime) VALUES "
			"(100012, 'Oasis_Checkpoint',       'TgGame.TgGame_Defense', 1550, 39114, 6638, 0, 0, 0, 0),"
			"(100013, 'Raid_Halloween_Oasis_P', 'TgGame.TgGame_Defense', 1550, 62626, 7175, 0, 0, 0, 0);",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v129 (oasis map_game_info): %s\n", err); return; }

		// (b) Spawn tables, canyon-playtested per-wave ratio (2x99 heavy
		//     anchors + 148 ticks / 147 wasps, avg ~16 bots/pulse):
		//       minions: 99{12597,12598} 148{12593,12595,12596,13540} 147{12594}
		//       spiders: 86{12602,12603}  guardians: 87{12601,13284,13290}
		const char* kV129_tables =
			"INSERT INTO map_object_config "
			"(map_name, map_object_id, column_name, value, variant_group, variant_id, weight) "
			"SELECT map, mid, col, tbl, NULL, NULL, 1 "
			"FROM (SELECT 'Oasis_Checkpoint' AS map UNION ALL SELECT 'Raid_Halloween_Oasis_P'),"
			"     (SELECT 'n_spawn_table_id' AS col UNION ALL SELECT 'n_default_spawn_table_id'), ("
			"  SELECT 12597 AS mid, '99' AS tbl UNION ALL SELECT 12598, '99' UNION ALL"
			"  SELECT 12593, '148' UNION ALL SELECT 12595, '148' UNION ALL"
			"  SELECT 12596, '148' UNION ALL SELECT 13540, '148' UNION ALL"
			"  SELECT 12594, '147' UNION ALL"
			"  SELECT 12602, '86' UNION ALL SELECT 12603, '86' UNION ALL"
			"  SELECT 12601, '87' UNION ALL SELECT 13284, '87' UNION ALL SELECT 13290, '87');";
		result = sqlite3_exec(db, kV129_tables, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v129 (oasis spawn tables): %s\n", err); return; }

		// All 12 factories on both maps: dormant + one-roster + Colony side.
		const char* kV129_flags =
			"INSERT INTO map_object_config "
			"(map_name, map_object_id, column_name, value, variant_group, variant_id, weight) "
			"SELECT map, mid, col, val, NULL, NULL, 1 "
			"FROM (SELECT 'Oasis_Checkpoint' AS map UNION ALL SELECT 'Raid_Halloween_Oasis_P'),"
			"     (SELECT 'b_auto_spawn' AS col, '0' AS val UNION ALL"
			"      SELECT 'b_respawn', '0' UNION ALL"
			"      SELECT 's_n_task_force', '1' UNION ALL"
			"      SELECT 's_n_team_number', '1'), ("
			"  SELECT 12593 AS mid UNION ALL SELECT 12594 UNION ALL SELECT 12595 UNION ALL"
			"  SELECT 12596 UNION ALL SELECT 12597 UNION ALL SELECT 12598 UNION ALL"
			"  SELECT 12601 UNION ALL SELECT 12602 UNION ALL SELECT 12603 UNION ALL"
			"  SELECT 13284 UNION ALL SELECT 13290 UNION ALL SELECT 13540);";
		result = sqlite3_exec(db, kV129_flags, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v129 (oasis factory flags): %s\n", err); return; }

		// (c) Boss objective 13460 (same mid on both maps, and coincidentally
		//     the same mid as dome's boss — rows are scoped per map_name) out
		//     of the priority unlock sequence; kismet activates it on the
		//     first Boss Wave Minions pulse. Defended NPC 12599 (auto-spawn,
		//     tf=2 baked) needs no config.
		result = sqlite3_exec(db,
			"INSERT INTO map_object_config "
			"(map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES"
			"  ('Oasis_Checkpoint',       13460, 'n_priority', '0', NULL, NULL, 1),"
			"  ('Raid_Halloween_Oasis_P', 13460, 'n_priority', '0', NULL, NULL, 1);",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v129 (oasis boss priority): %s\n", err); return; }

		Logger::Log("db", "v129: Oasis_Checkpoint + Raid_Halloween_Oasis_P defense raid config — "
			"map_game_info 100012/100013, 12 shared factories per map "
			"(2x99/4x148/1x147 minions, 2x86 spiders, 3x87 guardians, dormant), boss 13460 n_priority=0\n");
	}

	if (version < 130) {
		// v130: map_planner output for 1P_CPMine03_P (2026-07-13) — same
		// bundled shape as v74: spawn_table_id / default_spawn_table_id pairs,
		// task_force/team_number pairs, objective device ids, plus the specops
		// pool entry and map_game_info row.
		const char* kV130_cpmine03 =
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES"
			" ('1P_CPMine03_P', 12198, 'n_default_spawn_table_id', '10002', NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12198, 'n_spawn_table_id',         '10002', NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12458, 'n_default_spawn_table_id', '10002', NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12458, 'n_spawn_table_id',         '10002', NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12478, 'n_default_spawn_table_id', '10002', NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12478, 'n_spawn_table_id',         '10002', NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12536, 'n_default_spawn_table_id', '10002', NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12536, 'n_spawn_table_id',         '10002', NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12197, 'n_spawn_table_id',         '29',    NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12197, 'n_default_spawn_table_id', '29',    NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12479, 'n_default_spawn_table_id', '28',    NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12479, 'n_spawn_table_id',         '28',    NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12506, 'n_default_spawn_table_id', '28',    NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12506, 'n_spawn_table_id',         '28',    NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12534, 'n_default_spawn_table_id', '28',    NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12534, 'n_spawn_table_id',         '28',    NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12535, 'n_default_spawn_table_id', '28',    NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12535, 'n_spawn_table_id',         '28',    NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12199, 'n_default_spawn_table_id', '33',    NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12199, 'n_spawn_table_id',         '33',    NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12200, 'n_default_spawn_table_id', '33',    NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12200, 'n_spawn_table_id',         '33',    NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12529, 'n_default_spawn_table_id', '33',    NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12529, 'n_spawn_table_id',         '33',    NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12530, 'n_default_spawn_table_id', '33',    NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12530, 'n_spawn_table_id',         '33',    NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12531, 'n_default_spawn_table_id', '33',    NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12531, 'n_spawn_table_id',         '33',    NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12528, 'n_default_spawn_table_id', '59',    NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12528, 'n_spawn_table_id',         '59',    NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12532, 'n_spawn_table_id',         '46',    NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12532, 'n_default_spawn_table_id', '46',    NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12533, 'n_spawn_table_id',         '40',    NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12533, 'n_default_spawn_table_id', '40',    NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12538, 's_n_device_id',            '5447',  NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12227, 'm_n_task_force',           '1',     NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12425, 'm_n_task_force',           '2',     NULL, NULL, 1),"
			" ('1P_CPMine03_P', 8990,  's_n_team_number',          '1',     NULL, NULL, 1),"
			" ('1P_CPMine03_P', 8990,  's_n_task_force',           '1',     NULL, NULL, 1),"
			" ('1P_CPMine03_P', 8990,  's_n_device_id',            '2801',  NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12540, 's_n_device_id',            '7037',  NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12540, 's_n_team_number',          '2',     NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12540, 's_n_task_force',           '2',     NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12650, 's_n_task_force',           '2',     NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12650, 's_n_team_number',          '2',     NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12650, 's_n_device_id',            '7037',  NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12649, 's_n_task_force',           '2',     NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12649, 's_n_team_number',          '2',     NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12649, 's_n_device_id',            '7037',  NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12538, 's_n_team_number',          '2',     NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12538, 's_n_task_force',           '2',     NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12537, 's_n_device_id',            '5447',  NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12537, 's_n_team_number',          '1',     NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12537, 's_n_task_force',           '1',     NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12648, 's_n_device_id',            '7037',  NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12648, 's_n_team_number',          '1',     NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12648, 's_n_task_force',           '1',     NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12647, 's_n_device_id',            '7037',  NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12647, 's_n_team_number',          '1',     NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12647, 's_n_task_force',           '1',     NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12190, 's_n_team_number',          '1',     NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12190, 's_n_task_force',           '1',     NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12197, 's_n_task_force',           '2',     NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12197, 's_n_team_number',          '2',     NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12198, 's_n_task_force',           '2',     NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12198, 's_n_team_number',          '2',     NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12199, 's_n_task_force',           '2',     NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12199, 's_n_team_number',          '2',     NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12200, 's_n_task_force',           '2',     NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12200, 's_n_team_number',          '2',     NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12340, 's_n_task_force',           '1',     NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12340, 's_n_team_number',          '1',     NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12458, 's_n_task_force',           '2',     NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12458, 's_n_team_number',          '2',     NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12478, 's_n_task_force',           '2',     NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12478, 's_n_team_number',          '2',     NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12479, 's_n_task_force',           '2',     NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12479, 's_n_team_number',          '2',     NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12506, 's_n_task_force',           '2',     NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12506, 's_n_team_number',          '2',     NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12534, 's_n_task_force',           '2',     NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12534, 's_n_team_number',          '2',     NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12535, 's_n_task_force',           '2',     NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12535, 's_n_team_number',          '2',     NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12536, 's_n_task_force',           '2',     NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12536, 's_n_team_number',          '2',     NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12533, 's_n_task_force',           '2',     NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12533, 's_n_team_number',          '2',     NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12529, 's_n_task_force',           '2',     NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12529, 's_n_team_number',          '2',     NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12530, 's_n_task_force',           '2',     NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12530, 's_n_team_number',          '2',     NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12531, 's_n_task_force',           '2',     NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12531, 's_n_team_number',          '2',     NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12532, 's_n_task_force',           '2',     NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12532, 's_n_team_number',          '2',     NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12528, 's_n_task_force',           '2',     NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12528, 's_n_team_number',          '2',     NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12340, 's_b_auto_spawn',           '0',     NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12190, 'm_n_priority',             '1',     NULL, NULL, 1);";
		result = sqlite3_exec(db, kV130_cpmine03, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v130 (map_object_config cpmine03): %s\n", err); return; }

		// Add the map to the specops pool (map_pool_id=1 — see v65 for context).
		const char* kV130_specops_pool =
			"INSERT OR IGNORE INTO ga_map_pool_entries (map_pool_id, map_name, game_mode) VALUES"
			" (1, '1P_CPMine03_P', 'TgGame.TgGame_Mission');";
		result = sqlite3_exec(db, kV130_specops_pool, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v130 (specops pool): %s\n", err); return; }

		// map_game_info, same shape as the other specops maps (v73/v75).
		// 37933 = "Uranium Mining Complex" (asm_data_set_msg_translations);
		// 6523 = HUD_MissionLoads.PvE_CP.1P_CPMine03_P (asm_data_set_resources).
		result = sqlite3_exec(db,
			"INSERT OR REPLACE INTO map_game_info (map_game_id, map_name, game_class, gameplay_type_value_id, friendly_name_msg_id, entry_background_image_res_id) VALUES "
			"(1316, '1P_CPMine03_P', 'TgGame.TgGame_Mission', 1553, 37933, 6523);",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v130 (map_game_info cpmine03): %s\n", err); return; }

		Logger::Log("db", "v130: seeded map_object_config + specops pool + map_game_info "
			"(1316, Uranium Mining Complex) for 1P_CPMine03_P\n");
	}

	if (version < 131) {
		// v131: 1P_CPMine03_P bakes b_respawn=1 on ALL its factories, unlike
		// retail maps (1P_CPLab05_P: all 25 factories b_respawn=0). On alarm
		// factories that combines with the alarm bots' data-driven stand-down
		// (action 1272 Despawn, "CANCEL ALARM-BOT STATUS") into an endless
		// spawn/despawn loop: stand-down despawn -> intact BotDied requeues a
		// replacement (bRespawn) -> respawn -> stand-down again. Bot 1320
		// "Elite Assassin" (behavior 658) has no time-since-spawned gate, so its
		// loop cycles sub-second — spawn FX with no visible pawn + client FPS
		// collapse. Override ALL 17 factories to retail shape (regular
		// factories get the same b_respawn=0 override on other maps too).
		result = sqlite3_exec(db,
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES"
			" ('1P_CPMine03_P', 12197, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12198, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12199, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12200, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12458, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12478, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12479, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12506, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12528, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12529, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12530, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12531, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12532, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12533, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12534, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12535, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_CPMine03_P', 12536, 'b_respawn', '0', NULL, NULL, 1);",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v131 (cpmine03 factory respawn): %s\n", err); return; }

		Logger::Log("db", "v131: disabled b_respawn on all 17 bot factories of 1P_CPMine03_P\n");
	}

	if (version < 132) {
		// v132: 1p_SDColony04_P ("Recursive Communications", retail map_game_id
		// 1469 per asm_data_set_production_map_game_list + the kismet MapGameId
		// compares). Recursive Colony node mission: 3 doors open by destroying
		// Control Panel bots (factory TgSeqEvent_BotDied -> door matinee), then
		// 3 inner panels + the Colony Eye objective bot (13332, baked spawn
		// table 210, needs no config).
		//
		// Factory roles from the kismet dump:
		//   panels (spawn table 173 = Control Panel 1621):
		//     13911 first door, 13910 cave door, 13909 arena door,
		//     13904/13905/13908 inner "Destroy All 3 Panels" trio
		//   kismet-toggled (start dormant, b_auto_spawn=0):
		//     13917/13920/13925  Turn On at 2+ players (GetPlayerCount)
		//     13921/13922/13923/14091/14283  Turn On at 3+ players
		//     13927  Turn On 60s after the boss-area trigger volume
		//   alarm responder: 14094 (b_spawn_on_alarm=1 baked)
		//   flying (VolumePathNode patrols): 13918, 13923 -> wasp table 147
		// Spawn tables are the retail Recursive Colony set (85/98..103/147/
		// 167/168 — same family 1P_SDColony01_P uses); all carry Ultra-Max
		// Security (1471) rows so the difficulty cascade serves every tier of
		// the desert_pve queues. b_respawn=0 everywhere (retail shape, v131
		// lesson). Bots on task force / team 2, players + beacons on 1.
		const char* kV132_sdcolony04 =
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES"
			// --- door + inner panels: 1x Control Panel each ---
			" ('1p_SDColony04_P', 13911, 'n_spawn_table_id',         '173', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13911, 'n_default_spawn_table_id', '173', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13910, 'n_spawn_table_id',         '173', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13910, 'n_default_spawn_table_id', '173', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13909, 'n_spawn_table_id',         '173', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13909, 'n_default_spawn_table_id', '173', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13904, 'n_spawn_table_id',         '173', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13904, 'n_default_spawn_table_id', '173', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13905, 'n_spawn_table_id',         '173', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13905, 'n_default_spawn_table_id', '173', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13908, 'n_spawn_table_id',         '173', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13908, 'n_default_spawn_table_id', '173', NULL, NULL, 1),"
			// --- entry / surface area ---
			" ('1p_SDColony04_P', 13916, 'n_spawn_table_id',         '85',  NULL, NULL, 1)," // drones/ants/soldiers
			" ('1p_SDColony04_P', 13916, 'n_default_spawn_table_id', '85',  NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13917, 'n_spawn_table_id',         '85',  NULL, NULL, 1)," // 2p+ extra
			" ('1p_SDColony04_P', 13917, 'n_default_spawn_table_id', '85',  NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13922, 'n_spawn_table_id',         '102', NULL, NULL, 1)," // 3p+ extra, drones/soldiers
			" ('1p_SDColony04_P', 13922, 'n_default_spawn_table_id', '102', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13918, 'n_spawn_table_id',         '147', NULL, NULL, 1)," // wasps (VolumePathNode)
			" ('1p_SDColony04_P', 13918, 'n_default_spawn_table_id', '147', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13923, 'n_spawn_table_id',         '147', NULL, NULL, 1)," // wasps, 3p+ extra
			" ('1p_SDColony04_P', 13923, 'n_default_spawn_table_id', '147', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13930, 'n_spawn_table_id',         '102', NULL, NULL, 1)," // spawn approach, light
			" ('1p_SDColony04_P', 13930, 'n_default_spawn_table_id', '102', NULL, NULL, 1),"
			// --- mid area (after first door) ---
			" ('1p_SDColony04_P', 13924, 'n_spawn_table_id',         '167', NULL, NULL, 1)," // standard mix
			" ('1p_SDColony04_P', 13924, 'n_default_spawn_table_id', '167', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13925, 'n_spawn_table_id',         '167', NULL, NULL, 1)," // 2p+ extra
			" ('1p_SDColony04_P', 13925, 'n_default_spawn_table_id', '167', NULL, NULL, 1),"
			// --- arena door cluster ---
			" ('1p_SDColony04_P', 13919, 'n_spawn_table_id',         '99',  NULL, NULL, 1)," // heavy mix + sand spiders
			" ('1p_SDColony04_P', 13919, 'n_default_spawn_table_id', '99',  NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13932, 'n_spawn_table_id',         '167', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13932, 'n_default_spawn_table_id', '167', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13920, 'n_spawn_table_id',         '167', NULL, NULL, 1)," // 2p+ extra
			" ('1p_SDColony04_P', 13920, 'n_default_spawn_table_id', '167', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13921, 'n_spawn_table_id',         '99',  NULL, NULL, 1)," // 3p+ extra
			" ('1p_SDColony04_P', 13921, 'n_default_spawn_table_id', '99',  NULL, NULL, 1),"
			// --- deep area / Colony Eye pit ---
			" ('1p_SDColony04_P', 13929, 'n_spawn_table_id',         '167', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13929, 'n_default_spawn_table_id', '167', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13927, 'n_spawn_table_id',         '100', NULL, NULL, 1)," // boss-trigger ambush: sentry/guardian/scorpion/spiders
			" ('1p_SDColony04_P', 13927, 'n_default_spawn_table_id', '100', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 14091, 'n_spawn_table_id',         '101', NULL, NULL, 1)," // 3p+ extra, repair/soldier/scorpion
			" ('1p_SDColony04_P', 14091, 'n_default_spawn_table_id', '101', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 14283, 'n_spawn_table_id',         '99',  NULL, NULL, 1)," // 3p+ extra
			" ('1p_SDColony04_P', 14283, 'n_default_spawn_table_id', '99',  NULL, NULL, 1),"
			" ('1p_SDColony04_P', 14094, 'n_spawn_table_id',         '168', NULL, NULL, 1)," // alarm responders: elites
			" ('1p_SDColony04_P', 14094, 'n_default_spawn_table_id', '168', NULL, NULL, 1),"
			// --- kismet-toggled factories start dormant ---
			" ('1p_SDColony04_P', 13917, 'b_auto_spawn', '0', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13920, 'b_auto_spawn', '0', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13921, 'b_auto_spawn', '0', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13922, 'b_auto_spawn', '0', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13923, 'b_auto_spawn', '0', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13925, 'b_auto_spawn', '0', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 14091, 'b_auto_spawn', '0', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 14283, 'b_auto_spawn', '0', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13927, 'b_auto_spawn', '0', NULL, NULL, 1),"
			// --- b_respawn=0 on all 23 factories (retail shape) ---
			" ('1p_SDColony04_P', 13904, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13905, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13908, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13909, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13910, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13911, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13916, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13917, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13918, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13919, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13920, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13921, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13922, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13923, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13924, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13925, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13927, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13929, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13930, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13932, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 14091, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 14094, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 14283, 'b_respawn', '0', NULL, NULL, 1),"
			// --- bots on task force / team 2 ---
			" ('1p_SDColony04_P', 13904, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13904, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13905, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13905, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13908, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13908, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13909, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13909, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13910, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13910, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13911, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13911, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13916, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13916, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13917, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13917, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13918, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13918, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13919, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13919, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13920, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13920, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13921, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13921, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13922, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13922, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13923, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13923, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13924, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13924, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13925, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13925, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13927, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13927, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13929, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13929, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13930, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13930, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13932, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13932, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 14091, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 14091, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 14094, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 14094, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 14283, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 14283, 's_n_team_number', '2', NULL, NULL, 1),"
			// --- beacons: 13912 spawn-room, 13913 checkpoint (kismet Turn On
			//     via the TgPlayerCountVolume 'Toggle Checkpoint' event) ---
			" ('1p_SDColony04_P', 13912, 'm_n_priority',    '1', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13912, 's_n_task_force',  '1', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13912, 's_n_team_number', '1', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13913, 'm_n_priority',    '1', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13913, 's_n_task_force',  '1', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13913, 's_n_team_number', '1', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13913, 's_b_auto_spawn',  '0', NULL, NULL, 1),"
			// --- player start ---
			" ('1p_SDColony04_P', 13903, 'm_n_task_force',  '1', NULL, NULL, 1);";
		result = sqlite3_exec(db, kV132_sdcolony04, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v132 (map_object_config sdcolony04): %s\n", err); return; }

		// map_game_info: 1469 is the retail production id for this map (kismet
		// compares MapGameId==1469 for the intro textbox; ==1460 is a doors-
		// pre-opened variant we deliberately do NOT use). 66986 = "Recursive
		// Communications"; 8022 = HUD_MissionLoads_1_5.1P_DN_Colony_04 (the
		// 1.5-era Desert North colony-node loading screen set).
		result = sqlite3_exec(db,
			"INSERT OR REPLACE INTO map_game_info (map_game_id, map_name, game_class, gameplay_type_value_id, friendly_name_msg_id, entry_background_image_res_id) VALUES "
			"(1469, '1p_SDColony04_P', 'TgGame.TgGame_Mission', 1553, 66986, 8022);",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v132 (map_game_info sdcolony04): %s\n", err); return; }

		Logger::Log("db", "v132: seeded map_object_config + map_game_info "
			"(1469, Recursive Communications) for 1p_SDColony04_P\n");
	}

	if (version < 133) {
		// v133: 1P_SDColony03_P ("Recursive Colony Nest", never released —
		// no retail production id, custom 100014). Kismet roles:
		//   13807 BotDied -> door matinee (Doors/door_2/Lock/Bracer/Light)
		//     -> single Control Panel (173), like the SDColony04 doors
		//   13853 BotDied -> SetBool -> stops the Crusher loop -> panel (173)
		//   player-count extras (Turn On at 2+: 13808/13851/13966/13967,
		//     3+ adds 13838/13970) -> b_auto_spawn=0 (dormant until
		//     TgSeqAct_GetPlayerCount gets wired, same as SDColony04)
		//   13852 b_spawn_on_alarm=1 baked (alarm responders)
		//   flying (VolumePathNode patrols): 13838/13840/13971 -> wasps 147
		//   objective 13854 bakes spawn table 104 = Colony Overseer (80k HP)
		//     -> no config needed
		// Roster follows the v32 map-planner pass (obj_bot_factories, legacy
		// dead table) ported to map_object_config, with 13807 168->173 (door
		// panel, not a guardian pack) and 13838/13840 167->147 (flying).
		const char* kV133_sdcolony03 =
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES"
			// --- door panel + crusher panel ---
			" ('1P_SDColony03_P', 13807, 'n_spawn_table_id',         '173', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13807, 'n_default_spawn_table_id', '173', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13853, 'n_spawn_table_id',         '173', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13853, 'n_default_spawn_table_id', '173', NULL, NULL, 1),"
			// --- standard mixes ---
			" ('1P_SDColony03_P', 13808, 'n_spawn_table_id',         '167', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13808, 'n_default_spawn_table_id', '167', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13836, 'n_spawn_table_id',         '167', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13836, 'n_default_spawn_table_id', '167', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13837, 'n_spawn_table_id',         '167', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13837, 'n_default_spawn_table_id', '167', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13841, 'n_spawn_table_id',         '167', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13841, 'n_default_spawn_table_id', '167', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13842, 'n_spawn_table_id',         '167', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13842, 'n_default_spawn_table_id', '167', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13850, 'n_spawn_table_id',         '167', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13850, 'n_default_spawn_table_id', '167', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13966, 'n_spawn_table_id',         '167', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13966, 'n_default_spawn_table_id', '167', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13967, 'n_spawn_table_id',         '167', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13967, 'n_default_spawn_table_id', '167', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13968, 'n_spawn_table_id',         '167', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13968, 'n_default_spawn_table_id', '167', NULL, NULL, 1),"
			// --- special rosters ---
			" ('1P_SDColony03_P', 13851, 'n_spawn_table_id',         '99',  NULL, NULL, 1)," // heavy mix + sand spiders
			" ('1P_SDColony03_P', 13851, 'n_default_spawn_table_id', '99',  NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13852, 'n_spawn_table_id',         '98',  NULL, NULL, 1)," // alarm: drones/ants/wasps
			" ('1P_SDColony03_P', 13852, 'n_default_spawn_table_id', '98',  NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13969, 'n_spawn_table_id',         '148', NULL, NULL, 1)," // ants/ticks
			" ('1P_SDColony03_P', 13969, 'n_default_spawn_table_id', '148', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13970, 'n_spawn_table_id',         '102', NULL, NULL, 1)," // drones/soldiers
			" ('1P_SDColony03_P', 13970, 'n_default_spawn_table_id', '102', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13838, 'n_spawn_table_id',         '147', NULL, NULL, 1)," // wasps (VolumePathNode)
			" ('1P_SDColony03_P', 13838, 'n_default_spawn_table_id', '147', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13840, 'n_spawn_table_id',         '147', NULL, NULL, 1)," // wasps (VolumePathNode)
			" ('1P_SDColony03_P', 13840, 'n_default_spawn_table_id', '147', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13971, 'n_spawn_table_id',         '147', NULL, NULL, 1)," // wasps (VolumePathNode)
			" ('1P_SDColony03_P', 13971, 'n_default_spawn_table_id', '147', NULL, NULL, 1),"
			// --- kismet-toggled player-count extras start dormant ---
			" ('1P_SDColony03_P', 13808, 'b_auto_spawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13838, 'b_auto_spawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13851, 'b_auto_spawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13966, 'b_auto_spawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13967, 'b_auto_spawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13970, 'b_auto_spawn', '0', NULL, NULL, 1),"
			// --- b_respawn=0 on all 18 factories (retail shape) ---
			" ('1P_SDColony03_P', 13807, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13808, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13836, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13837, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13838, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13840, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13841, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13842, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13850, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13851, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13852, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13853, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13966, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13967, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13968, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13969, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13970, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13971, 'b_respawn', '0', NULL, NULL, 1),"
			// --- bots on task force / team 2 ---
			" ('1P_SDColony03_P', 13807, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13807, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13808, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13808, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13836, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13836, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13837, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13837, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13838, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13838, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13840, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13840, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13841, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13841, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13842, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13842, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13850, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13850, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13851, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13851, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13852, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13852, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13853, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13853, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13966, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13966, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13967, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13967, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13968, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13968, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13969, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13969, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13970, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13970, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13971, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13971, 's_n_team_number', '2', NULL, NULL, 1),"
			// --- beacons: 12456 spawn-room, 13839 checkpoint (kismet
			//     PlayerCountHit Turn On, m_b_beacon_exit=1 baked) ---
			" ('1P_SDColony03_P', 12456, 'm_n_priority',    '1', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 12456, 's_n_task_force',  '1', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 12456, 's_n_team_number', '1', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13839, 'm_n_priority',    '1', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13839, 's_n_task_force',  '1', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13839, 's_n_team_number', '1', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13839, 's_b_auto_spawn',  '0', NULL, NULL, 1),"
			// --- player start ---
			" ('1P_SDColony03_P', 13285, 'm_n_task_force',  '1', NULL, NULL, 1),"
			// --- device volumes. 8990 = the standard spawn invulnerability
			//     pad (2801) every 1P map configures; 12552 is stacked on it
			//     at the spawn room -> same 2801 (redundant coverage beats a
			//     wrong-guess gameplay effect). 13835 = Crusher (2238) and
			//     13466 = High Voltage (5450), both named by kismet toggle
			//     comments; 13814 sits at the bottom of the boss-room shaft
			//     (z=-5764) -> 5450 kill volume. Enemy-targeting hazards on
			//     team 2, friend-only pads on team 1 (v130 CPMine03 shape).
			" ('1P_SDColony03_P', 8990,  's_n_device_id',   '2801', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 8990,  's_n_task_force',  '1',    NULL, NULL, 1),"
			" ('1P_SDColony03_P', 8990,  's_n_team_number', '1',    NULL, NULL, 1),"
			" ('1P_SDColony03_P', 12552, 's_n_device_id',   '2801', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 12552, 's_n_task_force',  '1',    NULL, NULL, 1),"
			" ('1P_SDColony03_P', 12552, 's_n_team_number', '1',    NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13835, 's_n_device_id',   '2238', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13835, 's_n_task_force',  '2',    NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13835, 's_n_team_number', '2',    NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13466, 's_n_device_id',   '5450', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13466, 's_n_task_force',  '2',    NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13466, 's_n_team_number', '2',    NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13814, 's_n_device_id',   '5450', NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13814, 's_n_task_force',  '2',    NULL, NULL, 1),"
			" ('1P_SDColony03_P', 13814, 's_n_team_number', '2',    NULL, NULL, 1),"
			// --- 1p_SDColony04_P device-volume backfill (missed in v132):
			//     13915 sits at the spawn area next to beacon 13912 -> the
			//     standard 2801 pad; 13972/13973 are stacked in the deep
			//     shaft below the Colony Eye pit -> 5450 kill volumes.
			" ('1p_SDColony04_P', 13915, 's_n_device_id',   '2801', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13915, 's_n_task_force',  '1',    NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13915, 's_n_team_number', '1',    NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13972, 's_n_device_id',   '5450', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13972, 's_n_task_force',  '2',    NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13972, 's_n_team_number', '2',    NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13973, 's_n_device_id',   '5450', NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13973, 's_n_task_force',  '2',    NULL, NULL, 1),"
			" ('1p_SDColony04_P', 13973, 's_n_team_number', '2',    NULL, NULL, 1);";
		result = sqlite3_exec(db, kV133_sdcolony03, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v133 (map_object_config sdcolony03): %s\n", err); return; }

		// map_game_info: never released, no retail production id -> custom
		// 100014. 68623 = "Recursive Colony Nest"; 7855 =
		// HUD_MissionLoads.PVE_DN.1P_Colony04 (user-picked from extracted
		// art; alternatives: 8021 _1_5.1P_DN_Colony_02, 6844
		// PVE_SD.SDColony_01, 8023/8024 _05/_06).
		result = sqlite3_exec(db,
			"INSERT OR REPLACE INTO map_game_info (map_game_id, map_name, game_class, gameplay_type_value_id, friendly_name_msg_id, entry_background_image_res_id) VALUES "
			"(100014, '1P_SDColony03_P', 'TgGame.TgGame_Mission', 1553, 68623, 7855);",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v133 (map_game_info sdcolony03): %s\n", err); return; }

		Logger::Log("db", "v133: seeded map_object_config + map_game_info "
			"(100014, Recursive Colony Nest) for 1P_SDColony03_P "
			"+ SDColony04 device-volume backfill\n");
	}

	if (version < 134) {
		// v134: 1P_SDColony06_P ("28 Nights Later", never released — kismet's
		// SeqVar_Int 1468 is labeled "Quest Version" and its MissionStarted ->
		// CompareInt(MapGameId==1468) -> Destroy path removes the 6 barrier
		// InterpActors of the "More Areas for Quest Version" frame, so 1468 is
		// used as the map_game_id to open the side rooms at mission start).
		// Spawn-room doors open via the MissionStarted matinee (leftDoor/
		// RightDoor); the player-count toggles on this map target NOTHING
		// (unwired) -> no dormant factories.
		//
		// Roster = the map's own dev-authored spawn-table family 179-188
		// (difficulty-1029-only; the cascade serves every desert_pve tier):
		// 182 Corrupted Tribesman x4 (melee horde), 183 Corrupted Bruiser,
		// 163 Tick Incubator, 188 Wasp Incubator,
		// 184 Colony Tick x3 + Ant. Objective 13288 bakes table 181 = Colony
		// Overlord (50k) but also bakes s_bAutoSpawn=FALSE (every other 1P
		// map bakes TRUE) -> override, else the boss never spawns and the
		// mission cannot end.
		const char* kV134_sdcolony06 =
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES"
			// --- Corrupted Tribesman packs (182) — patrol factories ---
			" ('1P_SDColony06_P', 14020, 'n_spawn_table_id',         '182', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14020, 'n_default_spawn_table_id', '182', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14021, 'n_spawn_table_id',         '182', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14021, 'n_default_spawn_table_id', '182', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14022, 'n_spawn_table_id',         '182', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14022, 'n_default_spawn_table_id', '182', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14025, 'n_spawn_table_id',         '182', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14025, 'n_default_spawn_table_id', '182', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14026, 'n_spawn_table_id',         '182', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14026, 'n_default_spawn_table_id', '182', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14027, 'n_spawn_table_id',         '182', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14027, 'n_default_spawn_table_id', '182', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14028, 'n_spawn_table_id',         '182', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14028, 'n_default_spawn_table_id', '182', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14031, 'n_spawn_table_id',         '182', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14031, 'n_default_spawn_table_id', '182', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14032, 'n_spawn_table_id',         '182', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14032, 'n_default_spawn_table_id', '182', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14034, 'n_spawn_table_id',         '182', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14034, 'n_default_spawn_table_id', '182', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14035, 'n_spawn_table_id',         '182', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14035, 'n_default_spawn_table_id', '182', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14038, 'n_spawn_table_id',         '182', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14038, 'n_default_spawn_table_id', '182', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14040, 'n_spawn_table_id',         '182', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14040, 'n_default_spawn_table_id', '182', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14041, 'n_spawn_table_id',         '182', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14041, 'n_default_spawn_table_id', '182', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14042, 'n_spawn_table_id',         '182', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14042, 'n_default_spawn_table_id', '182', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14198, 'n_spawn_table_id',         '182', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14198, 'n_default_spawn_table_id', '182', NULL, NULL, 1),"
			// --- Corrupted Bruisers (183) — one heavy per cluster.
			//     14018/14019 confirmed by user playtest 2026-07-16 (OG had
			//     bruisers near the player spawn, not incubators) ---
			" ('1P_SDColony06_P', 14018, 'n_spawn_table_id',         '183', NULL, NULL, 1)," // near spawn
			" ('1P_SDColony06_P', 14018, 'n_default_spawn_table_id', '183', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14019, 'n_spawn_table_id',         '183', NULL, NULL, 1)," // near spawn
			" ('1P_SDColony06_P', 14019, 'n_default_spawn_table_id', '183', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14029, 'n_spawn_table_id',         '183', NULL, NULL, 1)," // west triple w/ 14041/14042
			" ('1P_SDColony06_P', 14029, 'n_default_spawn_table_id', '183', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14030, 'n_spawn_table_id',         '183', NULL, NULL, 1)," // north pair w/ 14031
			" ('1P_SDColony06_P', 14030, 'n_default_spawn_table_id', '183', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14037, 'n_spawn_table_id',         '183', NULL, NULL, 1)," // lower level
			" ('1P_SDColony06_P', 14037, 'n_default_spawn_table_id', '183', NULL, NULL, 1),"
			// --- Tick Incubators (163) — single-location nests.
			//     14039 confirmed by user playtest (boss approach) ---
			" ('1P_SDColony06_P', 14033, 'n_spawn_table_id',         '163', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14033, 'n_default_spawn_table_id', '163', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14039, 'n_spawn_table_id',         '163', NULL, NULL, 1)," // boss approach
			" ('1P_SDColony06_P', 14039, 'n_default_spawn_table_id', '163', NULL, NULL, 1),"
			// --- Wasp Incubators (188) — elevated in-place perches (z=616)
			//     + 14046 in the boss room (user playtest: OG had a Wasp
			//     Incubator there, not a wasp patrol) ---
			" ('1P_SDColony06_P', 14023, 'n_spawn_table_id',         '188', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14023, 'n_default_spawn_table_id', '188', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14024, 'n_spawn_table_id',         '188', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14024, 'n_default_spawn_table_id', '188', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14046, 'n_spawn_table_id',         '188', NULL, NULL, 1)," // boss room
			" ('1P_SDColony06_P', 14046, 'n_default_spawn_table_id', '188', NULL, NULL, 1),"
			// --- tick pack in the mid trench (in-place spawn) ---
			" ('1P_SDColony06_P', 14036, 'n_spawn_table_id',         '184', NULL, NULL, 1)," // Ticks x3 + Ant
			" ('1P_SDColony06_P', 14036, 'n_default_spawn_table_id', '184', NULL, NULL, 1),"
			// --- b_respawn=0 on all 27 factories (retail shape) ---
			" ('1P_SDColony06_P', 14018, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14019, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14020, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14021, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14022, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14023, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14024, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14025, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14026, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14027, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14028, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14029, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14030, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14031, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14032, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14033, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14034, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14035, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14036, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14037, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14038, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14039, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14040, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14041, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14042, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14046, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14198, 'b_respawn', '0', NULL, NULL, 1),"
			// --- bots on task force / team 2 ---
			" ('1P_SDColony06_P', 14018, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14018, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14019, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14019, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14020, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14020, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14021, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14021, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14022, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14022, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14023, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14023, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14024, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14024, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14025, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14025, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14026, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14026, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14027, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14027, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14028, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14028, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14029, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14029, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14030, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14030, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14031, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14031, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14032, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14032, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14033, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14033, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14034, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14034, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14035, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14035, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14036, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14036, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14037, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14037, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14038, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14038, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14039, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14039, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14040, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14040, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14041, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14041, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14042, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14042, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14046, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14046, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14198, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14198, 's_n_team_number', '2', NULL, NULL, 1),"
			// --- boss objective: baked s_bAutoSpawn=FALSE on this map only ---
			" ('1P_SDColony06_P', 13288, 's_b_auto_spawn', '1', NULL, NULL, 1),"
			// --- beacons: 13997 spawn-room, 13998 checkpoint (kismet
			//     PlayerCountHit Turn On, m_b_beacon_exit=1 baked) ---
			" ('1P_SDColony06_P', 13997, 'm_n_priority',    '1', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 13997, 's_n_task_force',  '1', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 13997, 's_n_team_number', '1', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 13998, 'm_n_priority',    '1', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 13998, 's_n_task_force',  '1', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 13998, 's_n_team_number', '1', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 13998, 's_b_auto_spawn',  '0', NULL, NULL, 1),"
			// --- player start ---
			" ('1P_SDColony06_P', 13996, 'm_n_task_force',  '1', NULL, NULL, 1),"
			// --- device volumes: 14047 sits over the spawn room -> standard
			//     2801 invulnerability pad; 13972/13973 are stacked deep under
			//     the boss pit (z=-1600/-1817) -> 5450 kill volumes. The nine
			//     13409 placements have no kismet reference and no identifiable
			//     role -> left unconfigured (inert).
			" ('1P_SDColony06_P', 14047, 's_n_device_id',   '2801', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14047, 's_n_task_force',  '1',    NULL, NULL, 1),"
			" ('1P_SDColony06_P', 14047, 's_n_team_number', '1',    NULL, NULL, 1),"
			" ('1P_SDColony06_P', 13972, 's_n_device_id',   '5450', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 13972, 's_n_task_force',  '2',    NULL, NULL, 1),"
			" ('1P_SDColony06_P', 13972, 's_n_team_number', '2',    NULL, NULL, 1),"
			" ('1P_SDColony06_P', 13973, 's_n_device_id',   '5450', NULL, NULL, 1),"
			" ('1P_SDColony06_P', 13973, 's_n_task_force',  '2',    NULL, NULL, 1),"
			" ('1P_SDColony06_P', 13973, 's_n_team_number', '2',    NULL, NULL, 1);";
		result = sqlite3_exec(db, kV134_sdcolony06, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v134 (map_object_config sdcolony06): %s\n", err); return; }

		// map_game_info: 1468 = the kismet "Quest Version" id (not in the
		// production list — the map never shipped) so the side areas open at
		// mission start. 66983 = "28 Nights Later"; 8024 =
		// HUD_MissionLoads_1_5.1P_DN_Colony_06 (authored for this map).
		result = sqlite3_exec(db,
			"INSERT OR REPLACE INTO map_game_info (map_game_id, map_name, game_class, gameplay_type_value_id, friendly_name_msg_id, entry_background_image_res_id) VALUES "
			"(1468, '1P_SDColony06_P', 'TgGame.TgGame_Mission', 1553, 66983, 8024);",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v134 (map_game_info sdcolony06): %s\n", err); return; }

		Logger::Log("db", "v134: seeded map_object_config + map_game_info "
			"(1468, 28 Nights Later) for 1P_SDColony06_P\n");
	}

	if (version < 135) {
		// v135: 1P_SDColony05_P ("Terminus Water Station", never released —
		// the Underground Waterway beneath Terminus; no retail production id
		// -> custom 100015). Elevator doors open via the MissionStarted
		// matinee. Kismet player-count wiring (dormant until
		// TgSeqAct_GetPlayerCount gets wired, same as the other colony maps):
		//   Turn On at 2+: 13955/13959/13960/13962/14000/14010/14193
		//   Turn On at 3+ adds: 13949/13961/14001/14012/14014/14015/14194/
		//     14196/14280/14281/14282
		//   "Chest encounter" (14274/14275 + TgChestActor): retail turns it
		//     OFF right after mission start (both compare branches feed Turn
		//     Off) — the initial spawn survives either way, so baked
		//     auto-spawn stays.
		// Objective 13288 bakes table 104 = Colony Overseer + s_bAutoSpawn=
		// TRUE -> no config. Varied roster from the retail colony tables
		// (multi-difficulty) + the incubator dev tables; 13 factories patrol
		// VolumePathNodes (flying) -> wasp tables.
		const char* kV135_sdcolony05 =
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES"
			// --- standard colony mix (167: drones/soldiers/ants/wasps) ---
			" ('1P_SDColony05_P', 13936, 'n_spawn_table_id',         '167', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13936, 'n_default_spawn_table_id', '167', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13938, 'n_spawn_table_id',         '167', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13938, 'n_default_spawn_table_id', '167', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13950, 'n_spawn_table_id',         '167', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13950, 'n_default_spawn_table_id', '167', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13949, 'n_spawn_table_id',         '167', NULL, NULL, 1)," // 3p+ extra
			" ('1P_SDColony05_P', 13949, 'n_default_spawn_table_id', '167', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13959, 'n_spawn_table_id',         '167', NULL, NULL, 1)," // 2p+ extra
			" ('1P_SDColony05_P', 13959, 'n_default_spawn_table_id', '167', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13960, 'n_spawn_table_id',         '167', NULL, NULL, 1)," // 2p+ extra
			" ('1P_SDColony05_P', 13960, 'n_default_spawn_table_id', '167', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13962, 'n_spawn_table_id',         '167', NULL, NULL, 1)," // 2p+ extra
			" ('1P_SDColony05_P', 13962, 'n_default_spawn_table_id', '167', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14001, 'n_spawn_table_id',         '167', NULL, NULL, 1)," // 3p+ extra
			" ('1P_SDColony05_P', 14001, 'n_default_spawn_table_id', '167', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14015, 'n_spawn_table_id',         '167', NULL, NULL, 1)," // 3p+ extra
			" ('1P_SDColony05_P', 14015, 'n_default_spawn_table_id', '167', NULL, NULL, 1),"
			// --- heavy mix + sand spiders (99) ---
			" ('1P_SDColony05_P', 13948, 'n_spawn_table_id',         '99',  NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13948, 'n_default_spawn_table_id', '99',  NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14000, 'n_spawn_table_id',         '99',  NULL, NULL, 1)," // 2p+ extra
			" ('1P_SDColony05_P', 14000, 'n_default_spawn_table_id', '99',  NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13961, 'n_spawn_table_id',         '99',  NULL, NULL, 1)," // 3p+ extra
			" ('1P_SDColony05_P', 13961, 'n_default_spawn_table_id', '99',  NULL, NULL, 1),"
			// --- elites: sentry/guardian/scorpion (168) ---
			" ('1P_SDColony05_P', 13953, 'n_spawn_table_id',         '168', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13953, 'n_default_spawn_table_id', '168', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14012, 'n_spawn_table_id',         '168', NULL, NULL, 1)," // 3p+ extra
			" ('1P_SDColony05_P', 14012, 'n_default_spawn_table_id', '168', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14274, 'n_spawn_table_id',         '168', NULL, NULL, 1)," // chest guards
			" ('1P_SDColony05_P', 14274, 'n_default_spawn_table_id', '168', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14275, 'n_spawn_table_id',         '168', NULL, NULL, 1)," // chest guards
			" ('1P_SDColony05_P', 14275, 'n_default_spawn_table_id', '168', NULL, NULL, 1),"
			// --- Colony Guardians (170: 1/1/2/3 by tier) ---
			" ('1P_SDColony05_P', 14013, 'n_spawn_table_id',         '170', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14013, 'n_default_spawn_table_id', '170', NULL, NULL, 1),"
			// --- flying wasp patrols (147) — VolumePathNode routes ---
			" ('1P_SDColony05_P', 13941, 'n_spawn_table_id',         '147', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13941, 'n_default_spawn_table_id', '147', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13947, 'n_spawn_table_id',         '147', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13947, 'n_default_spawn_table_id', '147', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14118, 'n_spawn_table_id',         '147', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14118, 'n_default_spawn_table_id', '147', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13955, 'n_spawn_table_id',         '147', NULL, NULL, 1)," // 2p+ extra
			" ('1P_SDColony05_P', 13955, 'n_default_spawn_table_id', '147', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14193, 'n_spawn_table_id',         '147', NULL, NULL, 1)," // 2p+ extra
			" ('1P_SDColony05_P', 14193, 'n_default_spawn_table_id', '147', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14014, 'n_spawn_table_id',         '147', NULL, NULL, 1)," // 3p+ extra
			" ('1P_SDColony05_P', 14014, 'n_default_spawn_table_id', '147', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14194, 'n_spawn_table_id',         '147', NULL, NULL, 1)," // 3p+ extra
			" ('1P_SDColony05_P', 14194, 'n_default_spawn_table_id', '147', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14281, 'n_spawn_table_id',         '147', NULL, NULL, 1)," // 3p+ extra
			" ('1P_SDColony05_P', 14281, 'n_default_spawn_table_id', '147', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14282, 'n_spawn_table_id',         '147', NULL, NULL, 1)," // 3p+ extra
			" ('1P_SDColony05_P', 14282, 'n_default_spawn_table_id', '147', NULL, NULL, 1),"
			// --- big wasp swarms (162: 6-12) — the 20-node flying routes ---
			" ('1P_SDColony05_P', 13944, 'n_spawn_table_id',         '162', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13944, 'n_default_spawn_table_id', '162', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14011, 'n_spawn_table_id',         '162', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14011, 'n_default_spawn_table_id', '162', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14010, 'n_spawn_table_id',         '162', NULL, NULL, 1)," // 2p+ extra
			" ('1P_SDColony05_P', 14010, 'n_default_spawn_table_id', '162', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14280, 'n_spawn_table_id',         '162', NULL, NULL, 1)," // 3p+ extra
			" ('1P_SDColony05_P', 14280, 'n_default_spawn_table_id', '162', NULL, NULL, 1),"
			// --- incubators (single-location nests) ---
			" ('1P_SDColony05_P', 14127, 'n_spawn_table_id',         '163', NULL, NULL, 1)," // Tick Incubator
			" ('1P_SDColony05_P', 14127, 'n_default_spawn_table_id', '163', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14272, 'n_spawn_table_id',         '163', NULL, NULL, 1)," // Tick Incubator
			" ('1P_SDColony05_P', 14272, 'n_default_spawn_table_id', '163', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14195, 'n_spawn_table_id',         '188', NULL, NULL, 1)," // Wasp Incubator
			" ('1P_SDColony05_P', 14195, 'n_default_spawn_table_id', '188', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14196, 'n_spawn_table_id',         '188', NULL, NULL, 1)," // Wasp Incubator, 3p+ extra
			" ('1P_SDColony05_P', 14196, 'n_default_spawn_table_id', '188', NULL, NULL, 1),"
			// --- kismet-toggled player-count extras start dormant ---
			" ('1P_SDColony05_P', 13949, 'b_auto_spawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13955, 'b_auto_spawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13959, 'b_auto_spawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13960, 'b_auto_spawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13961, 'b_auto_spawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13962, 'b_auto_spawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14000, 'b_auto_spawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14001, 'b_auto_spawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14010, 'b_auto_spawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14012, 'b_auto_spawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14014, 'b_auto_spawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14015, 'b_auto_spawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14193, 'b_auto_spawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14194, 'b_auto_spawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14196, 'b_auto_spawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14280, 'b_auto_spawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14281, 'b_auto_spawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14282, 'b_auto_spawn', '0', NULL, NULL, 1),"
			// --- b_respawn=0 on all 34 factories (retail shape) ---
			" ('1P_SDColony05_P', 13936, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13938, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13941, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13944, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13947, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13948, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13949, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13950, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13953, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13955, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13959, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13960, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13961, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13962, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14000, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14001, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14010, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14011, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14012, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14013, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14014, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14015, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14118, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14127, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14193, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14194, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14195, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14196, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14272, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14274, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14275, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14280, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14281, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14282, 'b_respawn', '0', NULL, NULL, 1),"
			// --- bots on task force / team 2 ---
			" ('1P_SDColony05_P', 13936, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13936, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13938, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13938, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13941, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13941, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13944, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13944, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13947, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13947, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13948, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13948, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13949, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13949, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13950, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13950, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13953, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13953, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13955, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13955, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13959, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13959, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13960, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13960, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13961, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13961, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13962, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13962, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14000, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14000, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14001, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14001, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14010, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14010, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14011, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14011, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14012, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14012, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14013, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14013, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14014, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14014, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14015, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14015, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14118, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14118, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14127, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14127, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14193, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14193, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14194, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14194, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14195, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14195, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14196, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14196, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14272, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14272, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14274, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14274, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14275, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14275, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14280, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14280, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14281, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14281, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14282, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 14282, 's_n_team_number', '2', NULL, NULL, 1),"
			// --- beacons: 13958 spawn-room, 13956 checkpoint (kismet
			//     PlayerCountHit Turn On + checkpoint-door matinee,
			//     m_b_beacon_exit=1 baked) ---
			" ('1P_SDColony05_P', 13958, 'm_n_priority',    '1', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13958, 's_n_task_force',  '1', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13958, 's_n_team_number', '1', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13956, 'm_n_priority',    '1', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13956, 's_n_task_force',  '1', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13956, 's_n_team_number', '1', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13956, 's_b_auto_spawn',  '0', NULL, NULL, 1),"
			// --- player start ---
			" ('1P_SDColony05_P', 13934, 'm_n_task_force',  '1', NULL, NULL, 1),"
			// --- device volumes: 13957 sits over the spawn room -> standard
			//     2801 invulnerability pad; 13965 (x2, stacked at z=-160/-376)
			//     and 13964 (z=-152) sit far below the walkable areas
			//     -> 5450 kill volumes (deep-water/shaft bottoms).
			" ('1P_SDColony05_P', 13957, 's_n_device_id',   '2801', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13957, 's_n_task_force',  '1',    NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13957, 's_n_team_number', '1',    NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13964, 's_n_device_id',   '5450', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13964, 's_n_task_force',  '2',    NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13964, 's_n_team_number', '2',    NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13965, 's_n_device_id',   '5450', NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13965, 's_n_task_force',  '2',    NULL, NULL, 1),"
			" ('1P_SDColony05_P', 13965, 's_n_team_number', '2',    NULL, NULL, 1);";
		result = sqlite3_exec(db, kV135_sdcolony05, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v135 (map_object_config sdcolony05): %s\n", err); return; }

		// map_game_info: never released, no retail production id -> custom
		// 100015. 66988 = "Terminus Water Station"; 8023 =
		// HUD_MissionLoads_1_5.1P_DN_Colony_05 (authored for this map).
		result = sqlite3_exec(db,
			"INSERT OR REPLACE INTO map_game_info (map_game_id, map_name, game_class, gameplay_type_value_id, friendly_name_msg_id, entry_background_image_res_id) VALUES "
			"(100015, '1P_SDColony05_P', 'TgGame.TgGame_Mission', 1553, 66988, 8023);",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v135 (map_game_info sdcolony05): %s\n", err); return; }

		Logger::Log("db", "v135: seeded map_object_config + map_game_info "
			"(100015, Terminus Water Station) for 1P_SDColony05_P\n");
	}

	if (version < 136) {
		// v136: SDColony06 roster corrections from the user's OG-server
		// playtest (2026-07-16), for DBs that already ran the original v134
		// (v134 itself was fixed in place for fresh DBs): 14018/14019 near
		// the player spawn are Corrupted Bruisers (183, not Tick Incubators),
		// 14039 at the boss approach is a Tick Incubator (163, not a
		// Bruiser), 14046 in the boss room is a Wasp Incubator (188, not a
		// wasp patrol). Value-gated so manual edits stick.
		const char* kV136_sdcolony06_fixups =
			"UPDATE map_object_config SET value = '183' "
			"  WHERE map_name = '1P_SDColony06_P' AND map_object_id IN (14018, 14019) "
			"    AND column_name IN ('n_spawn_table_id', 'n_default_spawn_table_id') AND value = '163';"
			"UPDATE map_object_config SET value = '163' "
			"  WHERE map_name = '1P_SDColony06_P' AND map_object_id = 14039 "
			"    AND column_name IN ('n_spawn_table_id', 'n_default_spawn_table_id') AND value = '183';"
			"UPDATE map_object_config SET value = '188' "
			"  WHERE map_name = '1P_SDColony06_P' AND map_object_id = 14046 "
			"    AND column_name IN ('n_spawn_table_id', 'n_default_spawn_table_id') AND value = '180';";
		result = sqlite3_exec(db, kV136_sdcolony06_fixups, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v136 (sdcolony06 roster fixups): %s\n", err); return; }

		Logger::Log("db", "v136: SDColony06 roster fixups (14018/14019 -> 183, "
			"14039 -> 163, 14046 -> 188)\n");
	}

	if (version < 137) {
		// v137: 1P_SDColony02_P ("Bolonov's Entourage", retail msg 67001 —
		// the Find-Dr-Bolonov quest instance, originally playable only with
		// the quest active). map_game_id 1471 = the kismet "quest version":
		// LevelLoaded -> CompareInt(MapGameId==1471) streams the
		// 1P_SDColony02_Quest sublevel (Bolonov / escort content; streaming
		// reaches clients via the native ClientUpdateLevelStreamingStatus
		// RPC) and sets the 'door' bool; boss capture (objective 13288,
		// AttackerCaptured) -> CompareBool -> escort-door matinee +
		// SetMissionTime Increment 300s + Music_Escort. The 1437 compare is
		// wired to nothing (non-quest visit: no sublevel, door stays shut) —
		// 1437 is what we ship (see the map_game_info comment below).
		// Mid-map cave doors: TgSeqEvent_BotDied (playerOnly) on factory
		// 13857 pumps a SeqAct_Switch whose Link 4 plays the cave-door
		// matinee -> 13857 gets table 99 (up to 10 bots) so the kill counter
		// always has enough deaths. Player-spawn doors ride MissionStarted.
		// Checkpoint beacon 13875 (m_b_beacon_exit=1 baked) is turned on by
		// TgSeqEvent_PlayerCountHit -> SeqAct_Toggle. The 3+-player
		// CompareFloat/Toggle chain is unwired -> no dormant factories.
		// Objective 13288 bakes s_b_auto_spawn=1 + table 104 (Colony
		// Overseer) -> no boss override needed.
		//
		// Roster: colony-node interior, varied retail colony tables —
		// 99/167 mixed drone-soldier-wasp packs, 101 drone trio,
		// 168 Colony Soldier, 100/102 Sand Spiders, 163 Tick Incubator,
		// 188 Wasp Incubator, 170 Colony Guardian.
		const char* kV137_sdcolony02 =
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES"
			// --- mixed packs (99: drones x5 + soldiers x2 + spider + wasps x2) ---
			" ('1P_SDColony02_P', 13857, 'n_spawn_table_id',         '99',  NULL, NULL, 1)," // cave-door kill counter
			" ('1P_SDColony02_P', 13857, 'n_default_spawn_table_id', '99',  NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13869, 'n_spawn_table_id',         '99',  NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13869, 'n_default_spawn_table_id', '99',  NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13876, 'n_spawn_table_id',         '99',  NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13876, 'n_default_spawn_table_id', '99',  NULL, NULL, 1),"
			// --- mixed packs (167: drones x3 + soldier + wasps x2) ---
			" ('1P_SDColony02_P', 13858, 'n_spawn_table_id',         '167', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13858, 'n_default_spawn_table_id', '167', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13868, 'n_spawn_table_id',         '167', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13868, 'n_default_spawn_table_id', '167', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13872, 'n_spawn_table_id',         '167', NULL, NULL, 1)," // boss room
			" ('1P_SDColony02_P', 13872, 'n_default_spawn_table_id', '167', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 14218, 'n_spawn_table_id',         '167', NULL, NULL, 1)," // mid-arena patroller
			" ('1P_SDColony02_P', 14218, 'n_default_spawn_table_id', '167', NULL, NULL, 1),"
			// --- drone trios (101) ---
			" ('1P_SDColony02_P', 13879, 'n_spawn_table_id',         '101', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13879, 'n_default_spawn_table_id', '101', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13976, 'n_spawn_table_id',         '101', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13976, 'n_default_spawn_table_id', '101', NULL, NULL, 1),"
			// --- Colony Soldiers (168) ---
			" ('1P_SDColony02_P', 13867, 'n_spawn_table_id',         '168', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13867, 'n_default_spawn_table_id', '168', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 14207, 'n_spawn_table_id',         '168', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 14207, 'n_default_spawn_table_id', '168', NULL, NULL, 1),"
			// --- Sand Spiders (100 single, 102 pair) ---
			" ('1P_SDColony02_P', 13988, 'n_spawn_table_id',         '100', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13988, 'n_default_spawn_table_id', '100', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13987, 'n_spawn_table_id',         '102', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13987, 'n_default_spawn_table_id', '102', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 14208, 'n_spawn_table_id',         '102', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 14208, 'n_default_spawn_table_id', '102', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 14197, 'n_spawn_table_id',         '102', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 14197, 'n_default_spawn_table_id', '102', NULL, NULL, 1),"
			// --- Tick Incubators (163) ---
			" ('1P_SDColony02_P', 13878, 'n_spawn_table_id',         '163', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13878, 'n_default_spawn_table_id', '163', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13975, 'n_spawn_table_id',         '163', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13975, 'n_default_spawn_table_id', '163', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13980, 'n_spawn_table_id',         '163', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13980, 'n_default_spawn_table_id', '163', NULL, NULL, 1),"
			// --- Wasp Incubators (188) ---
			" ('1P_SDColony02_P', 14217, 'n_spawn_table_id',         '188', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 14217, 'n_default_spawn_table_id', '188', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13871, 'n_spawn_table_id',         '188', NULL, NULL, 1)," // boss room
			" ('1P_SDColony02_P', 13871, 'n_default_spawn_table_id', '188', NULL, NULL, 1),"
			// --- Colony Guardians (170) ---
			" ('1P_SDColony02_P', 14080, 'n_spawn_table_id',         '170', NULL, NULL, 1)," // cave-door twin
			" ('1P_SDColony02_P', 14080, 'n_default_spawn_table_id', '170', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 14009, 'n_spawn_table_id',         '170', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 14009, 'n_default_spawn_table_id', '170', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 14209, 'n_spawn_table_id',         '170', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 14209, 'n_default_spawn_table_id', '170', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 14066, 'n_spawn_table_id',         '170', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 14066, 'n_default_spawn_table_id', '170', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13981, 'n_spawn_table_id',         '170', NULL, NULL, 1)," // boss room
			" ('1P_SDColony02_P', 13981, 'n_default_spawn_table_id', '170', NULL, NULL, 1),"
			// --- b_respawn=0 on all 25 factories (retail shape) ---
			" ('1P_SDColony02_P', 13857, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13858, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13867, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13868, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13869, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13871, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13872, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13876, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13878, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13879, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13975, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13976, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13980, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13981, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13987, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13988, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 14009, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 14066, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 14080, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 14197, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 14207, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 14208, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 14209, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 14217, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 14218, 'b_respawn', '0', NULL, NULL, 1),"
			// --- bots on task force / team 2 ---
			" ('1P_SDColony02_P', 13857, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13857, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13858, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13858, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13867, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13867, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13868, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13868, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13869, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13869, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13871, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13871, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13872, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13872, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13876, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13876, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13878, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13878, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13879, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13879, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13975, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13975, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13976, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13976, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13980, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13980, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13981, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13981, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13987, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13987, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13988, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13988, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 14009, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 14009, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 14066, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 14066, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 14080, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 14080, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 14197, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 14197, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 14207, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 14207, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 14208, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 14208, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 14209, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 14209, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 14217, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 14217, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 14218, 's_n_task_force', '2', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 14218, 's_n_team_number', '2', NULL, NULL, 1),"
			// --- beacons: 12456 spawn-room; 13875 checkpoint near the boss
			//     (m_b_beacon_exit=1 baked, kismet PlayerCountHit Turn On) ---
			" ('1P_SDColony02_P', 12456, 'm_n_priority',    '1', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 12456, 's_n_task_force',  '1', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 12456, 's_n_team_number', '1', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13875, 'm_n_priority',    '1', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13875, 's_n_task_force',  '1', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13875, 's_n_team_number', '1', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13875, 's_b_auto_spawn',  '0', NULL, NULL, 1),"
			// --- player start ---
			" ('1P_SDColony02_P', 13285, 'm_n_task_force',  '1', NULL, NULL, 1),"
			// --- device volumes: 12552 sits over the spawn room (z=1996 above
			//     the z=1766 start) -> standard 2801 invulnerability pad;
			//     13972/13973/13989 sit below the deep-cave floor
			//     (z=-3648..-3936 under the z=-3274 walkable) -> 5450 kill
			//     volumes (pit bottoms).
			" ('1P_SDColony02_P', 12552, 's_n_device_id',   '2801', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 12552, 's_n_task_force',  '1',    NULL, NULL, 1),"
			" ('1P_SDColony02_P', 12552, 's_n_team_number', '1',    NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13972, 's_n_device_id',   '5450', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13972, 's_n_task_force',  '2',    NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13972, 's_n_team_number', '2',    NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13973, 's_n_device_id',   '5450', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13973, 's_n_task_force',  '2',    NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13973, 's_n_team_number', '2',    NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13989, 's_n_device_id',   '5450', NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13989, 's_n_task_force',  '2',    NULL, NULL, 1),"
			" ('1P_SDColony02_P', 13989, 's_n_team_number', '2',    NULL, NULL, 1);";
		result = sqlite3_exec(db, kV137_sdcolony02, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v137 (map_object_config sdcolony02): %s\n", err); return; }

		// map_game_info: 1437 = the kismet NON-quest id (the 1471 quest
		// version was playtested 2026-07-16: sublevel streams, escort runs,
		// but nothing happens at the escort's end — the quest turn-in doesn't
		// exist server-side — so the mission couldn't finish; 1437 skips the
		// sublevel and the mission ends at the boss). 67001 = "Bolonov's
		// Entourage"; 8021 = HUD_MissionLoads_1_5.1P_DN_Colony_02 (authored
		// for this map).
		result = sqlite3_exec(db,
			"INSERT OR REPLACE INTO map_game_info (map_game_id, map_name, game_class, gameplay_type_value_id, friendly_name_msg_id, entry_background_image_res_id) VALUES "
			"(1437, '1P_SDColony02_P', 'TgGame.TgGame_Mission', 1553, 67001, 8021);",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v137 (map_game_info sdcolony02): %s\n", err); return; }

		Logger::Log("db", "v137: seeded map_object_config + map_game_info "
			"(1437, Bolonov's Entourage) for 1P_SDColony02_P\n");
	}

	if (version < 138) {
		// v138: SDColony02 quest->non-quest flip for DBs that already ran the
		// original v137 (which seeded map_game_id 1471; v137 itself was fixed
		// in place for fresh DBs). Gated on the 1471 row still existing so
		// manual edits stick.
		result = sqlite3_exec(db,
			"UPDATE map_game_info SET map_game_id = 1437 "
			"WHERE map_game_id = 1471 AND map_name = '1P_SDColony02_P';",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v138 (sdcolony02 1471 -> 1437): %s\n", err); return; }

		Logger::Log("db", "v138: SDColony02 map_game_id 1471 -> 1437 (non-quest)\n");
	}

	if (version < 139) {
		// v139: 1P_SDColony01_P corrections (map configured long ago, moving
		// from the disabled specops entry into desert_pve — control-server
		// seed handles the pool move):
		// - checkpoint/exit beacon 13440 (m_b_beacon_exit=1 baked) is
		//   kismet-activated (TgSeqEvent_PlayerCountHit -> SeqAct_Toggle
		//   Turn On) but was pre-spawning -> s_b_auto_spawn=0 (same
		//   treatment as SDColony05 13956 / SDColony02 13875).
		// - spawn-pad volumes 8990 + 12552 over the player start were
		//   unconfigured -> 2801 invulnerability pads (same id pair as
		//   SDColony03).
		// - electric barriers: kismet loops (LevelLoaded -> Delay 10/5/7s ->
		//   matinee with beam/spark/warmup emitters, event keys 'Kill' ->
		//   Toggle Turn On / 'Off' -> Turn Off) cycle bPainCausing on
		//   volumes 13466/13541 (tunnel pair) + 13542, but no device was
		//   configured so an active barrier did nothing -> 5450
		//   "High Voltage" (KillEffect), matching SDColony03's working
		//   barriers (same volume id 13466 there). setupDevice arms them;
		//   the kismet toggles gate the pain windows.
		result = sqlite3_exec(db,
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES"
			" ('1P_SDColony01_P', 13440, 's_b_auto_spawn',  '0',    NULL, NULL, 1),"
			" ('1P_SDColony01_P', 8990,  's_n_device_id',   '2801', NULL, NULL, 1),"
			" ('1P_SDColony01_P', 8990,  's_n_task_force',  '1',    NULL, NULL, 1),"
			" ('1P_SDColony01_P', 8990,  's_n_team_number', '1',    NULL, NULL, 1),"
			" ('1P_SDColony01_P', 12552, 's_n_device_id',   '2801', NULL, NULL, 1),"
			" ('1P_SDColony01_P', 12552, 's_n_task_force',  '1',    NULL, NULL, 1),"
			" ('1P_SDColony01_P', 12552, 's_n_team_number', '1',    NULL, NULL, 1),"
			" ('1P_SDColony01_P', 13466, 's_n_device_id',   '5450', NULL, NULL, 1),"
			" ('1P_SDColony01_P', 13466, 's_n_task_force',  '2',    NULL, NULL, 1),"
			" ('1P_SDColony01_P', 13466, 's_n_team_number', '2',    NULL, NULL, 1),"
			" ('1P_SDColony01_P', 13541, 's_n_device_id',   '5450', NULL, NULL, 1),"
			" ('1P_SDColony01_P', 13541, 's_n_task_force',  '2',    NULL, NULL, 1),"
			" ('1P_SDColony01_P', 13541, 's_n_team_number', '2',    NULL, NULL, 1),"
			" ('1P_SDColony01_P', 13542, 's_n_device_id',   '5450', NULL, NULL, 1),"
			" ('1P_SDColony01_P', 13542, 's_n_task_force',  '2',    NULL, NULL, 1),"
			" ('1P_SDColony01_P', 13542, 's_n_team_number', '2',    NULL, NULL, 1);",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v139 (map_object_config sdcolony01): %s\n", err); return; }

		// map_game_info: no MapGameId compare anywhere in this map's kismet,
		// so the canonical retail id is unrecoverable and irrelevant ->
		// custom 100016 (pattern: 100014 SDColony03 / 100015 SDColony05).
		// 55531 = "Recursive Colony Node 1393"; 6844 =
		// HUD_MissionLoads.PVE_SD.SDColony_01 (authored for this map).
		result = sqlite3_exec(db,
			"INSERT OR REPLACE INTO map_game_info (map_game_id, map_name, game_class, gameplay_type_value_id, friendly_name_msg_id, entry_background_image_res_id) VALUES "
			"(100016, '1P_SDColony01_P', 'TgGame.TgGame_Mission', 1553, 55531, 6844);",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v139 (map_game_info sdcolony01): %s\n", err); return; }

		Logger::Log("db", "v139: SDColony01 fixups (checkpoint beacon no-prespawn, "
			"2801 spawn pads, 5450 electric barriers) + map_game_info "
			"(100016, Recursive Colony Node 1393)\n");
	}

	if (version < 140) {
		// v140: SDColony01 barrier device correction for DBs that already ran
		// the original v139 (which used 5079 Electricity; v139 itself was
		// fixed in place for fresh DBs). SDColony03's working barriers use
		// 5450 High Voltage — same hazard kind, same volume id 13466 there.
		// Value-gated so manual edits stick.
		result = sqlite3_exec(db,
			"UPDATE map_object_config SET value = '5450' "
			"WHERE map_name = '1P_SDColony01_P' AND map_object_id IN (13466, 13541, 13542) "
			"  AND column_name = 's_n_device_id' AND value = '5079';",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v140 (sdcolony01 5079 -> 5450): %s\n", err); return; }

		Logger::Log("db", "v140: SDColony01 electric barriers 5079 -> 5450 (High Voltage)\n");
	}

	if (version < 141) {
		// v141: 1P_SDDweller01_P ("Canyon Encampment", retail msg 57316).
		// First dweller-camp map: human enemies (Kanar / Legion), no colony.
		// Kismet is minimal: LevelLoaded streams the sound sublevel;
		// TgSeqEvent_PlayerCountHit -> SeqAct_Toggle turns on the exit
		// beacon; the Touch -> CauseDamage(poison) chain is unbound
		// (originator None) and inert; the Scarab door matinees + GameOver
		// sounds are client cosmetics. No MapGameId compare anywhere.
		// Boss is baked: TgMissionObjective_Bot 13328 ("01_DES_DwellerBoss",
		// msg 55954) bakes s_b_auto_spawn=1 + spawn table 84 -> Legion
		// Champion (bot 1507 at difficulty 1029) — no override needed.
		//
		// Roster: retail tables 78-84 form the dweller cluster around the
		// baked boss table 84 (76/77/83/85 belong to other maps; none of
		// 78-84 used anywhere else by us). Assignment is ours, sized by each
		// factory's location-list count and position along the south (start,
		// y=-14627) -> north (exit, y=+10798) canyon:
		//   78 = Legion Soldier x4 + Kanar Tribesman x3 (+50% one more)
		//   79 = Legion Soldier x5 + Tribesman x3 + Bruiser + Raider (+25%)
		//   80 = Kanar Sniper x1     81 = Kanar Tribesman x4
		//   82 = Legion Bruiser x1
		// All factories bake b_respawn=1 with no tables (same dump state as
		// SDColony02) -> b_respawn=0, tf/team 2.
		// NOT configured (retail intent unrecoverable, no kismet trigger):
		// the 9 TgDeployableFactory hazard emitters (13478-13482 valley,
		// 13489-13492 ravine).
		const char* kV141_sddweller01 =
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES"
			// --- camps (78): 13327 south plateau (12 locs), 13330 start
			//     canyon high ground (10), 13333 mid camp (9), 13468 camp
			//     above the boss pit (7) ---
			" ('1P_SDDweller01_P', 13327, 'n_spawn_table_id',         '78', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13327, 'n_default_spawn_table_id', '78', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13330, 'n_spawn_table_id',         '78', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13330, 'n_default_spawn_table_id', '78', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13333, 'n_spawn_table_id',         '78', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13333, 'n_default_spawn_table_id', '78', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13468, 'n_spawn_table_id',         '78', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13468, 'n_default_spawn_table_id', '78', NULL, NULL, 1),"
			// --- heavy camps (79): 13335 north camp guarding the exit
			//     approach (13 locs), 13471 mid-low camp (9) ---
			" ('1P_SDDweller01_P', 13335, 'n_spawn_table_id',         '79', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13335, 'n_default_spawn_table_id', '79', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13471, 'n_spawn_table_id',         '79', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13471, 'n_default_spawn_table_id', '79', NULL, NULL, 1),"
			// --- snipers (80): 13342 lone perch over the start valley
			//     (single location), 13343 mid ledge (3) ---
			" ('1P_SDDweller01_P', 13342, 'n_spawn_table_id',         '80', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13342, 'n_default_spawn_table_id', '80', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13343, 'n_spawn_table_id',         '80', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13343, 'n_default_spawn_table_id', '80', NULL, NULL, 1),"
			// --- tribesmen (81): 13344 exit area (6 locs), 13355 west camp
			//     (7), 13467 alarm reinforcements near the boss pit
			//     (b_spawn_on_alarm=1 baked, 4 locs), 13472 far west (7) ---
			" ('1P_SDDweller01_P', 13344, 'n_spawn_table_id',         '81', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13344, 'n_default_spawn_table_id', '81', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13355, 'n_spawn_table_id',         '81', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13355, 'n_default_spawn_table_id', '81', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13467, 'n_spawn_table_id',         '81', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13467, 'n_default_spawn_table_id', '81', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13472, 'n_spawn_table_id',         '81', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13472, 'n_default_spawn_table_id', '81', NULL, NULL, 1),"
			// --- bruiser (82): 13470 northwest ledge (4 locs) ---
			" ('1P_SDDweller01_P', 13470, 'n_spawn_table_id',         '82', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13470, 'n_default_spawn_table_id', '82', NULL, NULL, 1),"
			// --- all factories: single-clear mission, enemy team ---
			" ('1P_SDDweller01_P', 13327, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13330, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13333, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13335, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13342, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13343, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13344, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13355, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13467, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13468, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13470, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13471, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13472, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13327, 's_n_task_force',  '2', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13327, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13330, 's_n_task_force',  '2', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13330, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13333, 's_n_task_force',  '2', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13333, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13335, 's_n_task_force',  '2', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13335, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13342, 's_n_task_force',  '2', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13342, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13343, 's_n_task_force',  '2', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13343, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13344, 's_n_task_force',  '2', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13344, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13355, 's_n_task_force',  '2', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13355, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13467, 's_n_task_force',  '2', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13467, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13468, 's_n_task_force',  '2', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13468, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13470, 's_n_task_force',  '2', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13470, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13471, 's_n_task_force',  '2', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13471, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13472, 's_n_task_force',  '2', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13472, 's_n_team_number', '2', NULL, NULL, 1),"
			// --- beacons: 13353 entry (at the start omega/equip zone);
			//     13354 exit (m_b_beacon_exit=1 baked, kismet PlayerCountHit
			//     Turn On -> no prespawn) ---
			" ('1P_SDDweller01_P', 13353, 'm_n_priority',    '1', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13353, 's_n_task_force',  '1', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13353, 's_n_team_number', '1', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13354, 'm_n_priority',    '1', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13354, 's_n_task_force',  '1', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13354, 's_n_team_number', '1', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13354, 's_b_auto_spawn',  '0', NULL, NULL, 1),"
			// --- device volumes: flooded floors with submerged electric
			//     cables (two low areas: 13484/13485/13486 ravine z=-320,
			//     13483/13495 valley z=80; 13397 northwest low z=-940) ->
			//     5450 "High Voltage" (KillEffect), same device as the
			//     SDColony01/03 barriers ---
			" ('1P_SDDweller01_P', 13397, 's_n_device_id',   '5450', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13397, 's_n_task_force',  '2',    NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13397, 's_n_team_number', '2',    NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13483, 's_n_device_id',   '5450', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13483, 's_n_task_force',  '2',    NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13483, 's_n_team_number', '2',    NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13484, 's_n_device_id',   '5450', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13484, 's_n_task_force',  '2',    NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13484, 's_n_team_number', '2',    NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13485, 's_n_device_id',   '5450', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13485, 's_n_task_force',  '2',    NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13485, 's_n_team_number', '2',    NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13486, 's_n_device_id',   '5450', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13486, 's_n_task_force',  '2',    NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13486, 's_n_team_number', '2',    NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13495, 's_n_device_id',   '5450', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13495, 's_n_task_force',  '2',    NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13495, 's_n_team_number', '2',    NULL, NULL, 1);";
		result = sqlite3_exec(db, kV141_sddweller01, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v141 (map_object_config sddweller01): %s\n", err); return; }

		// map_game_info: no MapGameId compare in kismet and no resource link
		// to a retail id -> custom 100017 (pattern: 100014-100016 colony
		// maps). 57316 = "Canyon Encampment"; 6847 =
		// HUD_MissionLoads.PVE_SD.SD_DDweller_01 (authored for this map).
		// No TgStartPoint on this map — players use the baked PlayerStarts.
		result = sqlite3_exec(db,
			"INSERT OR REPLACE INTO map_game_info (map_game_id, map_name, game_class, gameplay_type_value_id, friendly_name_msg_id, entry_background_image_res_id) VALUES "
			"(100017, '1P_SDDweller01_P', 'TgGame.TgGame_Mission', 1553, 57316, 6847);",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v141 (map_game_info sddweller01): %s\n", err); return; }

		Logger::Log("db", "v141: seeded map_object_config + map_game_info "
			"(100017, Canyon Encampment) for 1P_SDDweller01_P\n");
	}

	if (version < 142) {
		// v142: SDDweller01 boss table override. The baked objective table 84
		// has retail rows at several tiers and the cascade fills per-table
		// from the highest tier <= current, so at desert_pve_high
		// (difficulty 1030) table 84 resolves to its 1030 row = bot 929
		// "FLAG BOT" (retail junk) instead of the 1029 Legion Champion
		// (playtested 2026-07-16). Custom table 10006 exists only at Novice
		// (1467, lowest tier) -> the cascade reaches its Legion Champion row
		// at EVERY difficulty.
		result = sqlite3_exec(db,
			"INSERT INTO mod_data_set_bot_spawn_tables "
			"  (bot_spawn_table_id, difficulty_value_id, player_profile_id, spawn_group, "
			"   enemy_bot_id, bot_count, spawn_chance, team_size, multiple_class_flag, "
			"   bot_balance_multiplier, spawn_group_min, spawn_group_max, spawn_group_respawn_sec) "
			"VALUES (10006, 1467, 0, 1, 1507, 1, 1.0, 0, 0, 1.0, 0, 0, 0);",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v142 (mod spawn table 10006): %s\n", err); return; }

		result = sqlite3_exec(db,
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES"
			" ('1P_SDDweller01_P', 13328, 's_n_spawn_table_id', '10006', NULL, NULL, 1);",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v142 (sddweller01 boss override): %s\n", err); return; }

		Logger::Log("db", "v142: SDDweller01 boss objective -> mod table 10006 "
			"(Legion Champion at all difficulties)\n");
	}

	if (version < 143) {
		// v143: 1P_SDDweller02_P ("Shifting Sands", retail msg 54117) + EMP
		// posts on both dweller maps.
		// Kismet: LevelLoaded streams the sound sublevel and drives a
		// cosmetic sand-geyser system (RandomSwitch -> Toggle 4-emitter
		// clusters + spray sound, 1s later Turn Off, loop); PlayerCountHit
		// -> Toggle turns on exit beacon 13313; a one-shot Touch ->
		// GetTaskForceNumber -> CompareInt -> music cue; GameOver/FadedIn
		// sounds are client-only. No MapGameId compare -> custom 100018.
		// Boss: objective 13288 bakes s_b_auto_spawn=1 + table 84 -> same
		// FLAG-BOT-at-1030 hazard as Dweller01 -> override to mod table
		// 10006 (Legion Champion at all tiers, seeded in v142).
		// Standard start kit baked: start point 13285, spawn beacon 12456,
		// pad volumes 8990/12552 (same id trio as the colony maps).
		// NOT configured: the 10 device volumes sharing map_object_id 13409
		// (walkable-height spots near the camps, purpose unrecoverable; the
		// same unidentified id recurs on SDColony06).
		//
		// Roster: same dweller cluster 78-82 as SDDweller01 (see v141),
		// sized by location-list counts; start (4904,4938) -> south valley
		// -> boss far east (20404,-12349).
		const char* kV143_sddweller02 =
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES"
			// --- camps (78): 13286 west camp (7 locs), 13303 south valley
			//     overlook (11), 13309 east approach (12), 13340 boss camp (6) ---
			" ('1P_SDDweller02_P', 13286, 'n_spawn_table_id',         '78', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13286, 'n_default_spawn_table_id', '78', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13303, 'n_spawn_table_id',         '78', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13303, 'n_default_spawn_table_id', '78', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13309, 'n_spawn_table_id',         '78', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13309, 'n_default_spawn_table_id', '78', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13340, 'n_spawn_table_id',         '78', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13340, 'n_default_spawn_table_id', '78', NULL, NULL, 1),"
			// --- heavy camps (79): 13319 mid-west (11 locs), 13326 mid-east
			//     high ground (9) ---
			" ('1P_SDDweller02_P', 13319, 'n_spawn_table_id',         '79', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13319, 'n_default_spawn_table_id', '79', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13326, 'n_spawn_table_id',         '79', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13326, 'n_default_spawn_table_id', '79', NULL, NULL, 1),"
			// --- snipers (80): high perches with 1-3 locations ---
			" ('1P_SDDweller02_P', 13315, 'n_spawn_table_id',         '80', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13315, 'n_default_spawn_table_id', '80', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13316, 'n_spawn_table_id',         '80', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13316, 'n_default_spawn_table_id', '80', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13317, 'n_spawn_table_id',         '80', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13317, 'n_default_spawn_table_id', '80', NULL, NULL, 1),"
			// --- tribesmen (81): 13396 southeast, 13399 east ---
			" ('1P_SDDweller02_P', 13396, 'n_spawn_table_id',         '81', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13396, 'n_default_spawn_table_id', '81', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13399, 'n_spawn_table_id',         '81', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13399, 'n_default_spawn_table_id', '81', NULL, NULL, 1),"
			// --- bruiser (82): 13341 boss-pit alarm reinforcement
			//     (b_spawn_on_alarm=1 baked) ---
			" ('1P_SDDweller02_P', 13341, 'n_spawn_table_id',         '82', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13341, 'n_default_spawn_table_id', '82', NULL, NULL, 1),"
			// --- all factories: single-clear mission, enemy team ---
			" ('1P_SDDweller02_P', 13286, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13303, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13309, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13315, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13316, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13317, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13319, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13326, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13340, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13341, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13396, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13399, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13286, 's_n_task_force',  '2', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13286, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13303, 's_n_task_force',  '2', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13303, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13309, 's_n_task_force',  '2', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13309, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13315, 's_n_task_force',  '2', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13315, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13316, 's_n_task_force',  '2', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13316, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13317, 's_n_task_force',  '2', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13317, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13319, 's_n_task_force',  '2', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13319, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13326, 's_n_task_force',  '2', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13326, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13340, 's_n_task_force',  '2', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13340, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13341, 's_n_task_force',  '2', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13341, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13396, 's_n_task_force',  '2', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13396, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13399, 's_n_task_force',  '2', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13399, 's_n_team_number', '2', NULL, NULL, 1),"
			// --- boss override (same table-84 tier hazard as Dweller01) ---
			" ('1P_SDDweller02_P', 13288, 's_n_spawn_table_id', '10006', NULL, NULL, 1),"
			// --- beacons: 12456 spawn-room; 13313 exit (m_b_beacon_exit=1
			//     baked, kismet PlayerCountHit Turn On -> no prespawn) ---
			" ('1P_SDDweller02_P', 12456, 'm_n_priority',    '1', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 12456, 's_n_task_force',  '1', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 12456, 's_n_team_number', '1', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13313, 'm_n_priority',    '1', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13313, 's_n_task_force',  '1', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13313, 's_n_team_number', '1', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13313, 's_b_auto_spawn',  '0', NULL, NULL, 1),"
			// --- player start ---
			" ('1P_SDDweller02_P', 13285, 'm_n_task_force',  '1', NULL, NULL, 1),"
			// --- spawn pads over the start (standard 8990/12552 pair) ---
			" ('1P_SDDweller02_P', 8990,  's_n_device_id',   '2801', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 8990,  's_n_task_force',  '1',    NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 8990,  's_n_team_number', '1',    NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 12552, 's_n_device_id',   '2801', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 12552, 's_n_task_force',  '1',    NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 12552, 's_n_team_number', '1',    NULL, NULL, 1),"
			// --- EMP posts: deployable 210 "Dweller EMP Field Generator"
			//     (destructible pole, 1500 HP, device 6264 saps Power Pool
			//     20/s + stun-flag in an aura). No kismet trigger ->
			//     s_b_auto_spawn=1 (PostBeginPlay spawn); enemy-owned so
			//     players are the target and can destroy them. ---
			" ('1P_SDDweller02_P', 13400, 's_n_selected_object_id', '210', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13400, 's_n_task_force',         '2',   NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13400, 's_n_team_number',        '2',   NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13400, 's_b_auto_spawn',         '1',   NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13401, 's_n_selected_object_id', '210', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13401, 's_n_task_force',         '2',   NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13401, 's_n_team_number',        '2',   NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13401, 's_b_auto_spawn',         '1',   NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13402, 's_n_selected_object_id', '210', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13402, 's_n_task_force',         '2',   NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13402, 's_n_team_number',        '2',   NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13402, 's_b_auto_spawn',         '1',   NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13403, 's_n_selected_object_id', '210', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13403, 's_n_task_force',         '2',   NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13403, 's_n_team_number',        '2',   NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13403, 's_b_auto_spawn',         '1',   NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13404, 's_n_selected_object_id', '210', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13404, 's_n_task_force',         '2',   NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13404, 's_n_team_number',        '2',   NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13404, 's_b_auto_spawn',         '1',   NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13405, 's_n_selected_object_id', '210', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13405, 's_n_task_force',         '2',   NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13405, 's_n_team_number',        '2',   NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13405, 's_b_auto_spawn',         '1',   NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13406, 's_n_selected_object_id', '210', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13406, 's_n_task_force',         '2',   NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13406, 's_n_team_number',        '2',   NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13406, 's_b_auto_spawn',         '1',   NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13407, 's_n_selected_object_id', '210', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13407, 's_n_task_force',         '2',   NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13407, 's_n_team_number',        '2',   NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13407, 's_b_auto_spawn',         '1',   NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13421, 's_n_selected_object_id', '210', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13421, 's_n_task_force',         '2',   NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13421, 's_n_team_number',        '2',   NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13421, 's_b_auto_spawn',         '1',   NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13422, 's_n_selected_object_id', '210', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13422, 's_n_task_force',         '2',   NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13422, 's_n_team_number',        '2',   NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13422, 's_b_auto_spawn',         '1',   NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13423, 's_n_selected_object_id', '210', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13423, 's_n_task_force',         '2',   NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13423, 's_n_team_number',        '2',   NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13423, 's_b_auto_spawn',         '1',   NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13424, 's_n_selected_object_id', '210', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13424, 's_n_task_force',         '2',   NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13424, 's_n_team_number',        '2',   NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13424, 's_b_auto_spawn',         '1',   NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13426, 's_n_selected_object_id', '210', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13426, 's_n_task_force',         '2',   NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13426, 's_n_team_number',        '2',   NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13426, 's_b_auto_spawn',         '1',   NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13427, 's_n_selected_object_id', '210', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13427, 's_n_task_force',         '2',   NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13427, 's_n_team_number',        '2',   NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13427, 's_b_auto_spawn',         '1',   NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13428, 's_n_selected_object_id', '210', NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13428, 's_n_task_force',         '2',   NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13428, 's_n_team_number',        '2',   NULL, NULL, 1),"
			" ('1P_SDDweller02_P', 13428, 's_b_auto_spawn',         '1',   NULL, NULL, 1),"
			// --- Dweller01 EMP posts (same prop set; 13478-13482 valley,
			//     13489-13492 ravine, left unconfigured in v141) ---
			" ('1P_SDDweller01_P', 13478, 's_n_selected_object_id', '210', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13478, 's_n_task_force',         '2',   NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13478, 's_n_team_number',        '2',   NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13478, 's_b_auto_spawn',         '1',   NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13479, 's_n_selected_object_id', '210', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13479, 's_n_task_force',         '2',   NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13479, 's_n_team_number',        '2',   NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13479, 's_b_auto_spawn',         '1',   NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13480, 's_n_selected_object_id', '210', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13480, 's_n_task_force',         '2',   NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13480, 's_n_team_number',        '2',   NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13480, 's_b_auto_spawn',         '1',   NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13481, 's_n_selected_object_id', '210', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13481, 's_n_task_force',         '2',   NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13481, 's_n_team_number',        '2',   NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13481, 's_b_auto_spawn',         '1',   NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13482, 's_n_selected_object_id', '210', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13482, 's_n_task_force',         '2',   NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13482, 's_n_team_number',        '2',   NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13482, 's_b_auto_spawn',         '1',   NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13489, 's_n_selected_object_id', '210', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13489, 's_n_task_force',         '2',   NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13489, 's_n_team_number',        '2',   NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13489, 's_b_auto_spawn',         '1',   NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13490, 's_n_selected_object_id', '210', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13490, 's_n_task_force',         '2',   NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13490, 's_n_team_number',        '2',   NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13490, 's_b_auto_spawn',         '1',   NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13491, 's_n_selected_object_id', '210', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13491, 's_n_task_force',         '2',   NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13491, 's_n_team_number',        '2',   NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13491, 's_b_auto_spawn',         '1',   NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13492, 's_n_selected_object_id', '210', NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13492, 's_n_task_force',         '2',   NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13492, 's_n_team_number',        '2',   NULL, NULL, 1),"
			" ('1P_SDDweller01_P', 13492, 's_b_auto_spawn',         '1',   NULL, NULL, 1);";
		result = sqlite3_exec(db, kV143_sddweller02, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v143 (map_object_config sddweller02): %s\n", err); return; }

		// map_game_info: 54117 = "Shifting Sands"; 6594 =
		// HUD_MissionLoads.PVE_SD.1P_SDDweller02_P (authored for this map;
		// 6726 is the separate Portalled variant).
		result = sqlite3_exec(db,
			"INSERT OR REPLACE INTO map_game_info (map_game_id, map_name, game_class, gameplay_type_value_id, friendly_name_msg_id, entry_background_image_res_id) VALUES "
			"(100018, '1P_SDDweller02_P', 'TgGame.TgGame_Mission', 1553, 54117, 6594);",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v143 (map_game_info sddweller02): %s\n", err); return; }

		Logger::Log("db", "v143: seeded map_object_config + map_game_info "
			"(100018, Shifting Sands) for 1P_SDDweller02_P + EMP posts "
			"(deployable 210) on both dweller maps\n");
	}

	if (version < 144) {
		// v144: 1P_SDDweller03_P ("Dweller Hideout", retail msg 55320) —
		// final dweller map. Kismet is the same kit as Dweller02: sound
		// sublevel streaming, cosmetic sand geysers, PlayerCountHit ->
		// Toggle turns on exit beacon 13313, touch music cue; no MapGameId
		// compare -> custom 100019. No deployable factories (no EMP posts)
		// and no hazard volumes — the only two device volumes are the
		// standard 8990/12552 spawn pads. Start kit baked: start 13285,
		// spawn beacon 12456 (south, y=-11400), boss far north (y=+17900).
		//
		// Boss: objective 13356 bakes s_b_auto_spawn=1 + table 97 (Legion
		// Inquisitor at 1029 — but Legion Jailor at Adept 1468, nothing at
		// Novice). User wants Legion Inquisitor at ALL difficulties -> mod
		// table 10007 (bot 1531 at Novice 1467, cascade reaches it from
		// every tier) + override, same pattern as 10006.
		//
		// One factory bakes map_object_id=0 (3 locs, mid-map at
		// 12975,6165). Config rows with map_object_id=0 are safe here:
		// they're keyed per-map, and the only OTHER id-0 actors on this map
		// (alarm point, modify-pawn volumes) don't read these columns.
		result = sqlite3_exec(db,
			"INSERT INTO mod_data_set_bot_spawn_tables "
			"  (bot_spawn_table_id, difficulty_value_id, player_profile_id, spawn_group, "
			"   enemy_bot_id, bot_count, spawn_chance, team_size, multiple_class_flag, "
			"   bot_balance_multiplier, spawn_group_min, spawn_group_max, spawn_group_respawn_sec) "
			"VALUES (10007, 1467, 0, 1, 1531, 1, 1.0, 0, 0, 1.0, 0, 0, 0);",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v144 (mod spawn table 10007): %s\n", err); return; }

		const char* kV144_sddweller03 =
			"INSERT INTO map_object_config (map_name, map_object_id, column_name, value, variant_group, variant_id, weight) VALUES"
			// --- camps (78): 13312 south-mid (10 locs), 13318 boss camp (7),
			//     13346 first camp after start (7), 13348 mid-north (8) ---
			" ('1P_SDDweller03_P', 13312, 'n_spawn_table_id',         '78', NULL, NULL, 1),"
			" ('1P_SDDweller03_P', 13312, 'n_default_spawn_table_id', '78', NULL, NULL, 1),"
			" ('1P_SDDweller03_P', 13318, 'n_spawn_table_id',         '78', NULL, NULL, 1),"
			" ('1P_SDDweller03_P', 13318, 'n_default_spawn_table_id', '78', NULL, NULL, 1),"
			" ('1P_SDDweller03_P', 13346, 'n_spawn_table_id',         '78', NULL, NULL, 1),"
			" ('1P_SDDweller03_P', 13346, 'n_default_spawn_table_id', '78', NULL, NULL, 1),"
			" ('1P_SDDweller03_P', 13348, 'n_spawn_table_id',         '78', NULL, NULL, 1),"
			" ('1P_SDDweller03_P', 13348, 'n_default_spawn_table_id', '78', NULL, NULL, 1),"
			// --- heavy camps (79): 13345 mid (9 locs), 13347 exit-beacon
			//     overlook (9, z=+392 high ground) ---
			" ('1P_SDDweller03_P', 13345, 'n_spawn_table_id',         '79', NULL, NULL, 1),"
			" ('1P_SDDweller03_P', 13345, 'n_default_spawn_table_id', '79', NULL, NULL, 1),"
			" ('1P_SDDweller03_P', 13347, 'n_spawn_table_id',         '79', NULL, NULL, 1),"
			" ('1P_SDDweller03_P', 13347, 'n_default_spawn_table_id', '79', NULL, NULL, 1),"
			// --- sniper (80): 13349 single-location perch ---
			" ('1P_SDDweller03_P', 13349, 'n_spawn_table_id',         '80', NULL, NULL, 1),"
			" ('1P_SDDweller03_P', 13349, 'n_default_spawn_table_id', '80', NULL, NULL, 1),"
			// --- tribesmen (81): 13350 boss-camp alarm reinforcement
			//     (b_spawn_on_alarm=1 baked, 4 locs) ---
			" ('1P_SDDweller03_P', 13350, 'n_spawn_table_id',         '81', NULL, NULL, 1),"
			" ('1P_SDDweller03_P', 13350, 'n_default_spawn_table_id', '81', NULL, NULL, 1),"
			// --- bruiser (82): the id-0 factory (3 locs, mid-map) ---
			" ('1P_SDDweller03_P', 0,     'n_spawn_table_id',         '82', NULL, NULL, 1),"
			" ('1P_SDDweller03_P', 0,     'n_default_spawn_table_id', '82', NULL, NULL, 1),"
			// --- all factories: single-clear mission, enemy team ---
			" ('1P_SDDweller03_P', 13312, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDDweller03_P', 13318, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDDweller03_P', 13345, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDDweller03_P', 13346, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDDweller03_P', 13347, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDDweller03_P', 13348, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDDweller03_P', 13349, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDDweller03_P', 13350, 'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDDweller03_P', 0,     'b_respawn', '0', NULL, NULL, 1),"
			" ('1P_SDDweller03_P', 13312, 's_n_task_force',  '2', NULL, NULL, 1),"
			" ('1P_SDDweller03_P', 13312, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDDweller03_P', 13318, 's_n_task_force',  '2', NULL, NULL, 1),"
			" ('1P_SDDweller03_P', 13318, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDDweller03_P', 13345, 's_n_task_force',  '2', NULL, NULL, 1),"
			" ('1P_SDDweller03_P', 13345, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDDweller03_P', 13346, 's_n_task_force',  '2', NULL, NULL, 1),"
			" ('1P_SDDweller03_P', 13346, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDDweller03_P', 13347, 's_n_task_force',  '2', NULL, NULL, 1),"
			" ('1P_SDDweller03_P', 13347, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDDweller03_P', 13348, 's_n_task_force',  '2', NULL, NULL, 1),"
			" ('1P_SDDweller03_P', 13348, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDDweller03_P', 13349, 's_n_task_force',  '2', NULL, NULL, 1),"
			" ('1P_SDDweller03_P', 13349, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDDweller03_P', 13350, 's_n_task_force',  '2', NULL, NULL, 1),"
			" ('1P_SDDweller03_P', 13350, 's_n_team_number', '2', NULL, NULL, 1),"
			" ('1P_SDDweller03_P', 0,     's_n_task_force',  '2', NULL, NULL, 1),"
			" ('1P_SDDweller03_P', 0,     's_n_team_number', '2', NULL, NULL, 1),"
			// --- boss override: Legion Inquisitor at all difficulties ---
			" ('1P_SDDweller03_P', 13356, 's_n_spawn_table_id', '10007', NULL, NULL, 1),"
			// --- beacons: 12456 spawn-room; 13313 exit (m_b_beacon_exit=1
			//     baked, kismet PlayerCountHit Turn On -> no prespawn) ---
			" ('1P_SDDweller03_P', 12456, 'm_n_priority',    '1', NULL, NULL, 1),"
			" ('1P_SDDweller03_P', 12456, 's_n_task_force',  '1', NULL, NULL, 1),"
			" ('1P_SDDweller03_P', 12456, 's_n_team_number', '1', NULL, NULL, 1),"
			" ('1P_SDDweller03_P', 13313, 'm_n_priority',    '1', NULL, NULL, 1),"
			" ('1P_SDDweller03_P', 13313, 's_n_task_force',  '1', NULL, NULL, 1),"
			" ('1P_SDDweller03_P', 13313, 's_n_team_number', '1', NULL, NULL, 1),"
			" ('1P_SDDweller03_P', 13313, 's_b_auto_spawn',  '0', NULL, NULL, 1),"
			// --- player start ---
			" ('1P_SDDweller03_P', 13285, 'm_n_task_force',  '1', NULL, NULL, 1),"
			// --- spawn pads over the start (standard 8990/12552 pair) ---
			" ('1P_SDDweller03_P', 8990,  's_n_device_id',   '2801', NULL, NULL, 1),"
			" ('1P_SDDweller03_P', 8990,  's_n_task_force',  '1',    NULL, NULL, 1),"
			" ('1P_SDDweller03_P', 8990,  's_n_team_number', '1',    NULL, NULL, 1),"
			" ('1P_SDDweller03_P', 12552, 's_n_device_id',   '2801', NULL, NULL, 1),"
			" ('1P_SDDweller03_P', 12552, 's_n_task_force',  '1',    NULL, NULL, 1),"
			" ('1P_SDDweller03_P', 12552, 's_n_team_number', '1',    NULL, NULL, 1);";
		result = sqlite3_exec(db, kV144_sddweller03, nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v144 (map_object_config sddweller03): %s\n", err); return; }

		// map_game_info: 55320 = "Dweller Hideout"; 6622 =
		// HUD_MissionLoads.PVE_SD.1P_SDDweller03_P (authored for this map;
		// 6725 is the separate Portalled variant).
		result = sqlite3_exec(db,
			"INSERT OR REPLACE INTO map_game_info (map_game_id, map_name, game_class, gameplay_type_value_id, friendly_name_msg_id, entry_background_image_res_id) VALUES "
			"(100019, '1P_SDDweller03_P', 'TgGame.TgGame_Mission', 1553, 55320, 6622);",
			nullptr, nullptr, &err);
		if (result != SQLITE_OK) { Logger::Log("db", "Failed v144 (map_game_info sddweller03): %s\n", err); return; }

		Logger::Log("db", "v144: seeded map_object_config + map_game_info "
			"(100019, Dweller Hideout) for 1P_SDDweller03_P; boss -> mod "
			"table 10007 (Legion Inquisitor at all difficulties)\n");
	}

	// VR heal pad: enforce the pad device unconditionally (idempotent) —
	// branch-divergent DBs have version counters past the v101/v102 gates.
	// 2064 = Medical Station pulse (1.0s refire, FX 432 visual pulse);
	// 5134 = ER-2 flat +100, no FX. Testing 2064 for feel-comparison vs 5134.
	result = sqlite3_exec(db,
		"UPDATE map_object_config SET value = '2064' "
		"WHERE map_name = 'Dome3_VR_Arena_P' AND map_object_id = 11041 "
		"  AND column_name = 's_n_device_id' AND value <> '2064';",
		nullptr, nullptr, &err);
	if (result != SQLITE_OK) { Logger::Log("db", "Failed VR heal pad device enforce: %s\n", err); return; }

	result = sqlite3_exec(db, "UPDATE version_info SET version = 144", nullptr, nullptr, &err);
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

