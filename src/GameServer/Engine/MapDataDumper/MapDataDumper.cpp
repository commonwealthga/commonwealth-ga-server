#include "src/GameServer/Engine/MapDataDumper/MapDataDumper.hpp"
#include "src/GameServer/Engine/MapDataDumper/Writers/WriterCommon.hpp"
#include "src/Config/Config.hpp"
#include "src/Database/Database.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include "src/pch.hpp"

#include <cstring>

// Writer headers are appended below as each per-hierarchy task lands them.
// === BEGIN WRITER INCLUDES ===
#include "src/GameServer/Engine/MapDataDumper/Writers/TgActorFactory.hpp"
#include "src/GameServer/Engine/MapDataDumper/Writers/TgBotFactory.hpp"
#include "src/GameServer/Engine/MapDataDumper/Writers/TgBotFactorySpawnable.hpp"
#include "src/GameServer/Engine/MapDataDumper/Writers/TgBeaconFactory.hpp"
#include "src/GameServer/Engine/MapDataDumper/Writers/TgDeployableFactory.hpp"
#include "src/GameServer/Engine/MapDataDumper/Writers/TgDestructibleFactory.hpp"
#include "src/GameServer/Engine/MapDataDumper/Writers/TgHexItemFactory.hpp"
#include "src/GameServer/Engine/MapDataDumper/Writers/TgMissionObjective.hpp"
#include "src/GameServer/Engine/MapDataDumper/Writers/TgMissionObjective_Bot.hpp"
#include "src/GameServer/Engine/MapDataDumper/Writers/TgMissionObjective_Kismet.hpp"
#include "src/GameServer/Engine/MapDataDumper/Writers/TgMissionObjective_Proximity.hpp"
#include "src/GameServer/Engine/MapDataDumper/Writers/TgBaseObjective_CTFBot.hpp"
#include "src/GameServer/Engine/MapDataDumper/Writers/TgBaseObjective_KOTH.hpp"
#include "src/GameServer/Engine/MapDataDumper/Writers/TgMissionObjective_Escort.hpp"
#include "src/GameServer/Engine/MapDataDumper/Writers/TgStartPoint.hpp"
#include "src/GameServer/Engine/MapDataDumper/Writers/TgTeamPlayerStart.hpp"
#include "src/GameServer/Engine/MapDataDumper/Writers/TgOmegaVolume.hpp"
#include "src/GameServer/Engine/MapDataDumper/Writers/TgModifyPawnPropertiesVolume.hpp"
#include "src/GameServer/Engine/MapDataDumper/Writers/TgDeviceVolume.hpp"
#include "src/GameServer/Engine/MapDataDumper/Writers/TgFlagCaptureVolume.hpp"
#include "src/GameServer/Engine/MapDataDumper/Writers/TgHelpAlertVolume.hpp"
#include "src/GameServer/Engine/MapDataDumper/Writers/TgMissionListVolume.hpp"
#include "src/GameServer/Engine/MapDataDumper/Writers/TgNavigationPoint.hpp"
#include "src/GameServer/Engine/MapDataDumper/Writers/TgBotStart.hpp"
#include "src/GameServer/Engine/MapDataDumper/Writers/TgActionPoint.hpp"
#include "src/GameServer/Engine/MapDataDumper/Writers/TgAlarmPoint.hpp"
#include "src/GameServer/Engine/MapDataDumper/Writers/TgCoverPoint.hpp"
#include "src/GameServer/Engine/MapDataDumper/Writers/TgDefensePoint.hpp"
#include "src/GameServer/Engine/MapDataDumper/Writers/TgHoldSpot.hpp"
#include "src/GameServer/Engine/MapDataDumper/Writers/TgNavigationPointSpawnable.hpp"
#include "src/GameServer/Engine/MapDataDumper/Writers/TgPointOfInterest.hpp"
#include "src/GameServer/Engine/MapDataDumper/Writers/TgTeleporter.hpp"
// === END WRITER INCLUDES ===

using namespace MapDumpWriters;

namespace {

// Every map_* table that owns rows keyed by map_name. DELETEd at the start of
// each dump so the run is idempotent per map (re-running on the same map
// replaces; running on a different map adds without touching others).
const char* const kMapTables[] = {
	"map_tg_team_player_start",
	"map_tg_start_point",
	"map_tg_mission_objective",
	"map_tg_mission_objective_bot",
	"map_tg_mission_objective_kismet",
	"map_tg_mission_objective_proximity",
	"map_tg_base_objective_ctf_bot",
	"map_tg_base_objective_koth",
	"map_tg_mission_objective_escort",
	"map_tg_omega_volume",
	"map_tg_modify_pawn_properties_volume",
	"map_tg_device_volume",
	"map_tg_flag_capture_volume",
	"map_tg_help_alert_volume",
	"map_tg_mission_list_volume",
	"map_tg_actor_factory",
	"map_tg_bot_factory",
	"map_tg_bot_factory_spawnable",
	"map_tg_beacon_factory",
	"map_tg_deployable_factory",
	"map_tg_destructible_factory",
	"map_tg_hex_item_factory",
	"map_tg_navigation_point",
	"map_tg_bot_start",
	"map_tg_action_point",
	"map_tg_alarm_point",
	"map_tg_cover_point",
	"map_tg_defense_point",
	"map_tg_hold_spot",
	"map_tg_navigation_point_spawnable",
	"map_tg_point_of_interest",
	"map_tg_teleporter",
	"map_tg_bot_factory_location_list",
	"map_tg_bot_factory_patrol_path",
};
constexpr int kMapTableCount = sizeof(kMapTables) / sizeof(kMapTables[0]);

void DeletePriorRows(sqlite3* db, const std::string& mapName) {
	for (int i = 0; i < kMapTableCount; i++) {
		std::string sql = "DELETE FROM ";
		sql += kMapTables[i];
		sql += " WHERE map_name = ?";
		sqlite3_stmt* stmt = nullptr;
		if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
			Logger::Log("mapdump", "DELETE prepare failed for %s: %s\n",
				kMapTables[i], sqlite3_errmsg(db));
			continue;
		}
		sqlite3_bind_text(stmt, 1, mapName.c_str(), -1, SQLITE_TRANSIENT);
		sqlite3_step(stmt);
		sqlite3_finalize(stmt);
	}
}

void WriteByClass(sqlite3* db, AActor* actor, const std::string& mapName) {
	if (!actor || !actor->Class) return;
	std::string cls = ExtractClassName(actor->Class->GetFullName());
	if (cls.empty()) return;

	// Per-leaf-class branches inserted incrementally by writer tasks below.
	// Until they land, every actor falls through and is silently ignored.

	// === BEGIN DISPATCH BRANCHES ===
	if (cls == "TgBotFactory") {
		int id = static_cast<ATgActorFactory*>(actor)->m_nMapObjectId;
		WriteTgActorFactory(db, actor, mapName, cls, id);
		WriteTgBotFactory  (db, actor, mapName, cls, id);
	}
	else if (cls == "TgBotFactorySpawnable") {
		int id = static_cast<ATgActorFactory*>(actor)->m_nMapObjectId;
		WriteTgActorFactory       (db, actor, mapName, cls, id);
		WriteTgBotFactory         (db, actor, mapName, cls, id);
		WriteTgBotFactorySpawnable(db, actor, mapName, cls, id);
	}
	else if (cls == "TgBeaconFactory") {
		int id = static_cast<ATgActorFactory*>(actor)->m_nMapObjectId;
		WriteTgActorFactory (db, actor, mapName, cls, id);
		WriteTgBeaconFactory(db, actor, mapName, cls, id);
	}
	else if (cls == "TgDeployableFactory") {
		int id = static_cast<ATgActorFactory*>(actor)->m_nMapObjectId;
		WriteTgActorFactory     (db, actor, mapName, cls, id);
		WriteTgDeployableFactory(db, actor, mapName, cls, id);
	}
	else if (cls == "TgDestructibleFactory") {
		int id = static_cast<ATgActorFactory*>(actor)->m_nMapObjectId;
		WriteTgActorFactory       (db, actor, mapName, cls, id);
		WriteTgDestructibleFactory(db, actor, mapName, cls, id);
	}
	else if (cls == "TgHexItemFactory") {
		int id = static_cast<ATgActorFactory*>(actor)->m_nMapObjectId;
		WriteTgActorFactory  (db, actor, mapName, cls, id);
		WriteTgHexItemFactory(db, actor, mapName, cls, id);
	}
	else if (cls == "TgActorFactory") {
		int id = static_cast<ATgActorFactory*>(actor)->m_nMapObjectId;
		WriteTgActorFactory(db, actor, mapName, cls, id);
	}
	else if (cls == "TgMissionObjective_Bot") {
		int id = static_cast<ATgMissionObjective*>(actor)->m_nMapObjectId;
		WriteTgMissionObjective    (db, actor, mapName, cls, id);
		WriteTgMissionObjective_Bot(db, actor, mapName, cls, id);
	}
	else if (cls == "TgBaseObjective_CTFBot") {
		int id = static_cast<ATgMissionObjective*>(actor)->m_nMapObjectId;
		WriteTgMissionObjective    (db, actor, mapName, cls, id);
		WriteTgMissionObjective_Bot(db, actor, mapName, cls, id);
		WriteTgBaseObjective_CTFBot(db, actor, mapName, cls, id);
	}
	else if (cls == "TgMissionObjective_Kismet") {
		int id = static_cast<ATgMissionObjective*>(actor)->m_nMapObjectId;
		WriteTgMissionObjective       (db, actor, mapName, cls, id);
		WriteTgMissionObjective_Kismet(db, actor, mapName, cls, id);
	}
	else if (cls == "TgMissionObjective_Proximity") {
		int id = static_cast<ATgMissionObjective*>(actor)->m_nMapObjectId;
		WriteTgMissionObjective          (db, actor, mapName, cls, id);
		WriteTgMissionObjective_Proximity(db, actor, mapName, cls, id);
	}
	else if (cls == "TgBaseObjective_KOTH") {
		int id = static_cast<ATgMissionObjective*>(actor)->m_nMapObjectId;
		WriteTgMissionObjective          (db, actor, mapName, cls, id);
		WriteTgMissionObjective_Proximity(db, actor, mapName, cls, id);
		WriteTgBaseObjective_KOTH        (db, actor, mapName, cls, id);
	}
	else if (cls == "TgMissionObjective_Escort") {
		int id = static_cast<ATgMissionObjective*>(actor)->m_nMapObjectId;
		WriteTgMissionObjective          (db, actor, mapName, cls, id);
		WriteTgMissionObjective_Proximity(db, actor, mapName, cls, id);
		WriteTgMissionObjective_Escort   (db, actor, mapName, cls, id);
	}
	else if (cls == "TgMissionObjective") {
		int id = static_cast<ATgMissionObjective*>(actor)->m_nMapObjectId;
		WriteTgMissionObjective(db, actor, mapName, cls, id);
	}
	else if (cls == "TgTeamPlayerStart") {
		int id = static_cast<ATgStartPoint*>(actor)->m_nMapObjectId;
		WriteTgStartPoint     (db, actor, mapName, cls, id);
		WriteTgTeamPlayerStart(db, actor, mapName, cls, id);
	}
	else if (cls == "TgStartPoint") {
		int id = static_cast<ATgStartPoint*>(actor)->m_nMapObjectId;
		WriteTgStartPoint(db, actor, mapName, cls, id);
	}
	else if (cls == "TgOmegaVolume") {
		int id = static_cast<ATgOmegaVolume*>(actor)->m_nMapObjectId;
		WriteTgOmegaVolume(db, actor, mapName, cls, id);
	}
	else if (cls == "TgModifyPawnPropertiesVolume") {
		int id = static_cast<ATgModifyPawnPropertiesVolume*>(actor)->m_nMapObjectId;
		WriteTgModifyPawnPropertiesVolume(db, actor, mapName, cls, id);
	}
	else if (cls == "TgDeviceVolume") {
		int id = static_cast<ATgDeviceVolume*>(actor)->m_nMapObjectId;
		WriteTgDeviceVolume(db, actor, mapName, cls, id);
	}
	else if (cls == "TgFlagCaptureVolume") {
		int id = static_cast<ATgFlagCaptureVolume*>(actor)->m_nMapObjectId;
		WriteTgFlagCaptureVolume(db, actor, mapName, cls, id);
	}
	else if (cls == "TgHelpAlertVolume") {
		int id = static_cast<ATgHelpAlertVolume*>(actor)->m_nMapObjectId;
		WriteTgHelpAlertVolume(db, actor, mapName, cls, id);
	}
	else if (cls == "TgMissionListVolume") {
		int id = static_cast<ATgMissionListVolume*>(actor)->m_nMapObjectId;
		WriteTgMissionListVolume(db, actor, mapName, cls, id);
	}
	else if (cls == "TgBotStart") {
		int id = static_cast<ATgNavigationPoint*>(actor)->m_nMapObjectId;
		WriteTgNavigationPoint(db, actor, mapName, cls, id);
		WriteTgBotStart       (db, actor, mapName, cls, id);
	}
	else if (cls == "TgActionPoint") {
		int id = static_cast<ATgNavigationPoint*>(actor)->m_nMapObjectId;
		WriteTgNavigationPoint(db, actor, mapName, cls, id);
		WriteTgActionPoint    (db, actor, mapName, cls, id);
	}
	else if (cls == "TgAlarmPoint") {
		int id = static_cast<ATgNavigationPoint*>(actor)->m_nMapObjectId;
		WriteTgNavigationPoint(db, actor, mapName, cls, id);
		WriteTgAlarmPoint     (db, actor, mapName, cls, id);
	}
	else if (cls == "TgCoverPoint") {
		int id = static_cast<ATgNavigationPoint*>(actor)->m_nMapObjectId;
		WriteTgNavigationPoint(db, actor, mapName, cls, id);
		WriteTgCoverPoint     (db, actor, mapName, cls, id);
	}
	else if (cls == "TgDefensePoint") {
		int id = static_cast<ATgNavigationPoint*>(actor)->m_nMapObjectId;
		WriteTgNavigationPoint(db, actor, mapName, cls, id);
		WriteTgDefensePoint   (db, actor, mapName, cls, id);
	}
	else if (cls == "TgHoldSpot") {
		int id = static_cast<ATgNavigationPoint*>(actor)->m_nMapObjectId;
		WriteTgNavigationPoint(db, actor, mapName, cls, id);
		WriteTgDefensePoint   (db, actor, mapName, cls, id);
		WriteTgHoldSpot       (db, actor, mapName, cls, id);
	}
	else if (cls == "TgNavigationPointSpawnable") {
		int id = static_cast<ATgNavigationPoint*>(actor)->m_nMapObjectId;
		WriteTgNavigationPoint         (db, actor, mapName, cls, id);
		WriteTgNavigationPointSpawnable(db, actor, mapName, cls, id);
	}
	else if (cls == "TgPointOfInterest") {
		int id = static_cast<ATgNavigationPoint*>(actor)->m_nMapObjectId;
		WriteTgNavigationPoint(db, actor, mapName, cls, id);
		WriteTgPointOfInterest(db, actor, mapName, cls, id);
	}
	else if (cls == "TgNavigationPoint") {
		int id = static_cast<ATgNavigationPoint*>(actor)->m_nMapObjectId;
		WriteTgNavigationPoint(db, actor, mapName, cls, id);
	}
	else if (cls == "TgTeleporter") {
		int id = static_cast<ATgTeleporter*>(actor)->m_nMapObjectId;
		WriteTgTeleporter(db, actor, mapName, cls, id);
	}
	// === END DISPATCH BRANCHES ===
}

}  // namespace

void MapDataDumper::DumpAll() {
	sqlite3* db = Database::GetConnection();
	if (!db) {
		Logger::Log("mapdump", "DB connection unavailable — aborting dump\n");
		return;
	}

	std::string mapName = Config::GetMapNameChar();
	Logger::Log("mapdump", "=== map dump BEGIN map=%s ===\n", mapName.c_str());

	sqlite3_exec(db, "BEGIN TRANSACTION", nullptr, nullptr, nullptr);
	DeletePriorRows(db, mapName);

	TArray<UObject*>* objs = UObject::GObjObjects();
	if (!objs) {
		Logger::Log("mapdump", "GObjObjects() is null, aborting\n");
		sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
		return;
	}

	int scanned = 0;
	for (int i = 0; i < objs->Count; i++) {
		UObject* obj = objs->Data[i];
		if (!obj || !obj->Class) continue;

		// Skip Class Default Objects — synthetic "Default__X" names,
		// zero locations, not real map placements.
		const char* fullName = obj->GetFullName();
		if (fullName && std::strstr(fullName, "Default__")) continue;

		// Cheap reinterpret: WriteByClass's class-name match gates the writes,
		// so the cast itself never dereferences non-actor fields for objects
		// that don't actually match a dispatch branch.
		AActor* actor = static_cast<AActor*>(obj);
		WriteByClass(db, actor, mapName);
		scanned++;
	}

	sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr);
	Logger::Log("mapdump", "=== map dump END map=%s (scanned %d) ===\n",
		mapName.c_str(), scanned);
}
