#include "src/GameServer/TgGame/TgActorFactory/LoadObjectConfig/TgActorFactory__LoadObjectConfig.hpp"
#include "src/GameServer/Engine/MapObjectConfig/MapObjectConfig.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgActorFactory__LoadObjectConfig::Call(ATgActorFactory* Factory, void* edx) {
	CallOriginal(Factory, edx);
	if (Factory == nullptr || Factory->Class == nullptr) return;

	const char* rawClassName = Factory->Class->GetFullName();
	if (rawClassName == nullptr) return;
	const std::string className(rawClassName);

	if (className.find("TgBeaconFactory") != std::string::npos) {
		ATgBeaconFactory* factory = (ATgBeaconFactory*)Factory;
		const int mid = factory->m_nMapObjectId;

		factory->m_nPriority = MapObjectConfig::GetInt(mid, "m_n_priority", factory->m_nPriority);
		factory->s_nTaskForce  = (unsigned char)MapObjectConfig::GetInt(
			mid, "s_n_task_force", factory->s_nTaskForce);
		factory->s_nTeamNumber = MapObjectConfig::GetInt(
			mid, "s_n_team_number", factory->s_nTeamNumber);
		factory->s_bAutoSpawn = (bool)MapObjectConfig::GetInt(
			mid, "s_b_auto_spawn", factory->s_bAutoSpawn);
	}

	// Deployable factories (e.g. Raid_DomeCityDefense_P's EMP / dismantler-landing
	// FX) carry no per-factory deployable id in the baked map — s_nSelectedObjectId
	// is 0 and the selection-list resolution was in the stripped native. Drive it
	// from map_object_config instead: which deployable to spawn, the owning task
	// force / team, and the auto-spawn gate (these FX are kismet-toggle driven, so
	// they ship with s_b_auto_spawn=0 to suppress the PostBeginPlay auto-spawn).
	// Read here in PreBeginPlay so PostBeginPlay + SpawnObject see the values.
	if (className.find("TgDeployableFactory") != std::string::npos) {
		const int mid = Factory->m_nMapObjectId;

		Factory->s_nSelectedObjectId = MapObjectConfig::GetInt(
			mid, "s_n_selected_object_id", Factory->s_nSelectedObjectId);
		Factory->s_nTaskForce = (unsigned char)MapObjectConfig::GetInt(
			mid, "s_n_task_force", Factory->s_nTaskForce);
		Factory->s_nTeamNumber = MapObjectConfig::GetInt(
			mid, "s_n_team_number", Factory->s_nTeamNumber);
		Factory->s_bAutoSpawn = (bool)MapObjectConfig::GetInt(
			mid, "s_b_auto_spawn", Factory->s_bAutoSpawn);

		Logger::Log("deployablefactory",
			"LoadObjectConfig mapObjectId=%d selectedObj=%d taskForce=%d team=%d autoSpawn=%d\n",
			mid, Factory->s_nSelectedObjectId, (int)Factory->s_nTaskForce,
			Factory->s_nTeamNumber, (int)Factory->s_bAutoSpawn);
	}
}
