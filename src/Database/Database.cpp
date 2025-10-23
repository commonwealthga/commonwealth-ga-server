#include "src/Database/Database.hpp"
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
				physical_type_value_id INTEGER \
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
				bot_spawn_table_id INTEGER \
				task_force_number INTEGER \
			); \
		", nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to create obj_bot_factories table: %s\n", err);
			return;
		}
	}

	if (version < 3) {
		result = sqlite3_exec(db,
			"insert into obj_bot_factories (map_object_id, bot_spawn_table_id, task_force_number) values \
				(13639, 147, 1), \
				(13632, 86 , 1), \
				(13629, 99 , 1), \
				(13638, 148, 1), \
				(13647, 87 , 1), \
				(13630, 148, 1), \
				(13634, 148, 1), \
				(13633, 99 , 1), \
				(13637, 148, 1), \
				(13635, 102, 1), \
				(13636, 99 , 1), \
				(13640, 148, 1), \
				(13642, 86 , 1), \
				(13641, 99 , 1), \
				(13810, 102, 1), \
				(13645, 99 , 1), \
				(13644, 86 , 1), \
				(13646, 99 , 1), \
				(13664, 99 , 1), \
				(13648, 99 , 1), \
				(13650, 148, 1), \
				(13704, 86 , 1), \
				(13652, 148, 1), \
				(13651, 99 , 1), \
				(13694, 102, 1), \
				(13659, 102, 1), \
				(13661, 86 , 1), \
				(13655, 99 , 1), \
				(13656, 99 , 1), \
				(13657, 87 , 1), \
				(13660, 99 , 1), \
				(13654, 99 , 1), \
				(13692, 147, 1), \
				(13708, 86 , 1), \
				(13643, 148, 1), \
				(13709, 102, 1), \
				(13673, 149, 1), \
				(13805, 102, 1), \
				(13691, 87 , 1), \
				(13658, 87 , 1), \
				(13703, 149, 1), \
				(13662, 149, 1), \
				(13649, 99 , 1), \
				(13665, 102, 1), \
				(13802, 102, 1), \
				(13804, 102, 1), \
				(13803, 102, 1), \
				(13700, 102, 1), \
				(13806, 102, 1), \
				(13653, 149, 1), \
				(13849, 166, 2), \
				(13846, 166, 2), \
				(13847, 166, 2), \
				(13848, 166, 2),  \
				\
				(12712, 29, 2), \
				(12708, 29, 2), \
				(12710, 58, 2), \
				(12711, 29, 2), \
				(12714, 34, 2), \
				(12713, 34, 2), \
				(12709, 28, 2); \
			", nullptr, nullptr, &err);
		if (result != SQLITE_OK) {
			Logger::Log("db", "Failed to insert obj_bot_factories: %s\n", err);
			return;
		}
	}

	result = sqlite3_exec(db, "UPDATE version_info SET version = 3", nullptr, nullptr, &err);
	if (result != SQLITE_OK) {
		Logger::Log("db", "Failed to update version_info: %s\n", err);
		return;
	}
}

