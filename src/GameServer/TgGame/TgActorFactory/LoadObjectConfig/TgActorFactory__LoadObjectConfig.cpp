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
}
