#include "src/GameServer/TgGame/TgDeviceVolume/setupDevice/TgDeviceVolume__setupDevice.hpp"
#include "src/GameServer/Engine/MapObjectConfig/MapObjectConfig.hpp"
#include "src/Utils/Logger/Logger.hpp"

bool __fastcall TgDeviceVolume_setupDevice::Call(ATgDeviceVolume* Volume, void* edx) {
	Volume->s_nDeviceId = MapObjectConfig::GetInt(Volume->m_nMapObjectId, "s_n_device_id", Volume->s_nDeviceId);
	Volume->s_nTeamNumber = MapObjectConfig::GetInt(Volume->m_nMapObjectId, "s_n_team_number", Volume->s_nTeamNumber);
	Volume->s_nTaskForce = MapObjectConfig::GetInt(Volume->m_nMapObjectId, "s_n_task_force", Volume->s_nTaskForce);


}

