#include "src/GameServer/TgGame/_deployable_classify/DeployableClassify.hpp"

#include "src/Database/Database.hpp"

#include <unordered_map>

namespace DeployableClassify {

bool IsKnownDeployableId(int nDeployableId) {
	if (nDeployableId <= 0) return false;

	static std::unordered_map<int, int> s_cache;
	auto it = s_cache.find(nDeployableId);
	if (it != s_cache.end()) return it->second == 1;

	int exists = 0;
	sqlite3* db = Database::GetConnection();
	if (!db) return false;

	sqlite3_stmt* stmt = nullptr;
	if (sqlite3_prepare_v2(db,
		"SELECT 1 FROM asm_data_set_deployables WHERE deployable_id = ? LIMIT 1;",
		-1, &stmt, nullptr) == SQLITE_OK) {
		sqlite3_bind_int(stmt, 1, nDeployableId);
		if (sqlite3_step(stmt) == SQLITE_ROW) {
			exists = 1;
		}
		sqlite3_finalize(stmt);
	}
	s_cache[nDeployableId] = exists;
	return exists == 1;
}

bool IsTimerBomb(int nDeployableId) {
	// 1 = bomb, 0 = non-bomb. Missing DB row treated as non-bomb.
	static std::unordered_map<int, int> s_cache;
	auto it = s_cache.find(nDeployableId);
	if (it != s_cache.end()) return it->second == 1;

	int bombFlag = 0;
	sqlite3* db = Database::GetConnection();
	if (db) {
		sqlite3_stmt* stmt = nullptr;
		if (sqlite3_prepare_v2(db,
			"SELECT show_countdown_timer_flag FROM asm_data_set_deployables "
			"WHERE deployable_id = ? LIMIT 1;",
			-1, &stmt, nullptr) == SQLITE_OK) {
			sqlite3_bind_int(stmt, 1, nDeployableId);
			if (sqlite3_step(stmt) == SQLITE_ROW) {
				bombFlag = sqlite3_column_int(stmt, 0);
			}
			sqlite3_finalize(stmt);
		}
	}
	s_cache[nDeployableId] = bombFlag;
	return bombFlag == 1;
}

bool DeploysOnSelf(int nDeployableId) {
	// 1 = self-spawning (any device-mode has require_los_flag=0), 0 = aim-traced.
	// Cached per id. Defaults to 0 (aim-traced) if no DB row matches.
	static std::unordered_map<int, int> s_cache;
	auto it = s_cache.find(nDeployableId);
	if (it != s_cache.end()) return it->second == 1;

	int selfDeploy = 0;
	sqlite3* db = Database::GetConnection();
	if (db) {
		sqlite3_stmt* stmt = nullptr;
		if (sqlite3_prepare_v2(db,
			"SELECT 1 FROM asm_data_set_devices_data_set_device_modes "
			"WHERE deployable_id = ? AND require_los_flag = 0 LIMIT 1;",
			-1, &stmt, nullptr) == SQLITE_OK) {
			sqlite3_bind_int(stmt, 1, nDeployableId);
			if (sqlite3_step(stmt) == SQLITE_ROW) {
				selfDeploy = 1;
			}
			sqlite3_finalize(stmt);
		}
	}
	s_cache[nDeployableId] = selfDeploy;
	return selfDeploy == 1;
}

bool IsDestructible(int nDeployableId) {
	// 1 = destructible (HP > 0), 0 = indestructible (HP == 0 or NULL).
	// Missing DB row is treated as destructible — a station whose row was
	// dropped or mis-tagged should still be killable rather than invulnerable.
	static std::unordered_map<int, int> s_cache;
	auto it = s_cache.find(nDeployableId);
	if (it != s_cache.end()) return it->second == 1;

	int destructible = 1;
	sqlite3* db = Database::GetConnection();
	if (db) {
		sqlite3_stmt* stmt = nullptr;
		if (sqlite3_prepare_v2(db,
			"SELECT health FROM asm_data_set_deployables WHERE deployable_id = ? LIMIT 1;",
			-1, &stmt, nullptr) == SQLITE_OK) {
			sqlite3_bind_int(stmt, 1, nDeployableId);
			if (sqlite3_step(stmt) == SQLITE_ROW) {
				const int hp = sqlite3_column_int(stmt, 0);
				destructible = (hp > 0) ? 1 : 0;
			}
			sqlite3_finalize(stmt);
		}
	}
	s_cache[nDeployableId] = destructible;
	return destructible == 1;
}

}  // namespace DeployableClassify
