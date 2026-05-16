#include "src/GameServer/TgGame/TgMissionObjective/LoadObjectConfig/TgMissionObjective__LoadObjectConfig.hpp"
#include "src/GameServer/Engine/MapObjectConfig/MapObjectConfig.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgMissionObjective__LoadObjectConfig::Call(ATgMissionObjective* Obj, void* edx) {
	CallOriginal(Obj, edx);
	if (Obj == nullptr) return;

	const int mid = Obj->m_nMapObjectId;

	const unsigned char prevStatus = Obj->r_eStatus;
	Obj->r_eStatus = (unsigned char)MapObjectConfig::GetInt(mid, "r_e_status", Obj->r_eStatus);

	if (Obj->r_eStatus != prevStatus) {
		Logger::Log("config",
			"TgMissionObjective::LoadObjectConfig — map_object_id=%d r_e_status %d -> %d\n",
			mid, (int)prevStatus, (int)Obj->r_eStatus);
	}
}
