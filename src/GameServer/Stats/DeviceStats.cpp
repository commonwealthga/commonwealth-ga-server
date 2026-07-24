#include "src/GameServer/Stats/DeviceStats.hpp"
#include "src/GameServer/Engine/World/GetWorldInfo/World__GetWorldInfo.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/Database/Database.hpp"

#include "sqlite3.h"

#include <unordered_map>

namespace {

constexpr int kNumRows   = 9;   // ATgRepInfo_Player::r_DeviceStats[9]
constexpr int kFieldDPM  = 5;
constexpr int kFieldHPM  = 6;

// Seconds since the mission timer started. Falls back to level time when the
// timer hasn't been armed (warmup, home maps) so DPM never divides by zero.
float MissionElapsedSeconds() {
	if (Globals::Get().GWorld == nullptr) return 1.0f;
	AWorldInfo* WI = World__GetWorldInfo::CallOriginal((UWorld*)Globals::Get().GWorld, nullptr, 0);
	if (WI == nullptr) return 1.0f;

	float elapsed = WI->TimeSeconds;
	ATgGame* Game = (ATgGame*)Globals::Get().GGameInfo;
	if (Game != nullptr && Game->s_fMissionTimerStartedAt > 0.0f) {
		elapsed = WI->TimeSeconds - Game->s_fMissionTimerStartedAt;
	}
	return elapsed < 1.0f ? 1.0f : elapsed;
}

std::unordered_map<int, int> g_modeToDevice;
bool g_modeMapLoaded = false;

std::unordered_map<int, int> g_deployableToDevice;
bool g_deployableMapLoaded = false;

}  // namespace

int DeviceStats::DeviceIdFromModeId(int deviceModeId) {
	if (deviceModeId <= 0) return 0;

	if (!g_modeMapLoaded) {
		g_modeMapLoaded = true;
		sqlite3* db = Database::GetConnection();
		if (db != nullptr) {
			sqlite3_stmt* stmt = nullptr;
			const char* sql =
				"SELECT device_mode_id, device_id "
				"FROM asm_data_set_devices_data_set_device_modes";
			if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
				while (sqlite3_step(stmt) == SQLITE_ROW) {
					g_modeToDevice[sqlite3_column_int(stmt, 0)] =
						sqlite3_column_int(stmt, 1);
				}
			}
			sqlite3_finalize(stmt);
		}
	}

	auto it = g_modeToDevice.find(deviceModeId);
	return it == g_modeToDevice.end() ? 0 : it->second;
}

int DeviceStats::DeviceIdFromDeployableId(int deployableId) {
	if (deployableId <= 0) return 0;

	if (!g_deployableMapLoaded) {
		g_deployableMapLoaded = true;
		sqlite3* db = Database::GetConnection();
		if (db != nullptr) {
			// Both spawn routes: the mode's own deployable_id, and the
			// deployable its projectile spawns (mines, spider grenades). The
			// projectile join must exclude device_projectile_id = 0 or every
			// projectile-less mode matches every projectile-less row.
			// slot_used_value_id filter keeps bot/test devices out, so the
			// counts below only see player-equippable spawners.
			const char* sql =
				"SELECT dep.deployable_id, MIN(m.device_id), COUNT(DISTINCT m.device_id) "
				"FROM asm_data_set_devices_data_set_device_modes m "
				"JOIN asm_data_set_devices d ON d.device_id = m.device_id "
				"LEFT JOIN asm_data_set_projectiles p "
				"  ON p.device_projectile_id = m.device_projectile_id "
				"  AND m.device_projectile_id != 0 "
				"JOIN asm_data_set_deployables dep "
				"  ON (m.deployable_id != 0 AND dep.deployable_id = m.deployable_id) "
				"  OR (p.spawn_deployable_id != 0 AND dep.deployable_id = p.spawn_deployable_id) "
				"WHERE d.slot_used_value_id IN (388,389,390,393,394,476,741,806,981,1482) "
				"GROUP BY dep.deployable_id";
			sqlite3_stmt* stmt = nullptr;
			if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
				while (sqlite3_step(stmt) == SQLITE_ROW) {
					if (sqlite3_column_int(stmt, 2) != 1) continue;  // ambiguous
					g_deployableToDevice[sqlite3_column_int(stmt, 0)] =
						sqlite3_column_int(stmt, 1);
				}
			}
			sqlite3_finalize(stmt);
		}
	}

	auto it = g_deployableToDevice.find(deployableId);
	return it == g_deployableToDevice.end() ? 0 : it->second;
}

void DeviceStats::Credit(ATgPawn* CreditPawn, int deviceId, int field, int amount) {
	if (CreditPawn == nullptr || deviceId <= 0 || amount <= 0) return;

	ATgRepInfo_Player* PRI = (ATgRepInfo_Player*)CreditPawn->PlayerReplicationInfo;
	if (PRI == nullptr) return;

	FDeviceStatInfo* row = nullptr;
	for (int i = 0; i < kNumRows; i++) {
		if (PRI->r_DeviceStats[i].Stats[kId] == deviceId) { row = &PRI->r_DeviceStats[i]; break; }
	}
	if (row == nullptr) {
		for (int i = 0; i < kNumRows; i++) {
			if (PRI->r_DeviceStats[i].Stats[kId] == 0) {
				row = &PRI->r_DeviceStats[i];
				row->Stats[kId] = deviceId;
				break;
			}
		}
	}
	// ponytail: 10th+ distinct device in one match is dropped, not evicted —
	// the client only renders 8 rows anyway. Add LRU eviction if it matters.
	if (row == nullptr) return;

	row->Stats[field] += amount;

	const float minutes = MissionElapsedSeconds() / 60.0f;
	row->Stats[kFieldDPM] = (int)(row->Stats[kDamage]  / minutes);
	row->Stats[kFieldHPM] = (int)(row->Stats[kHealing] / minutes);
}
