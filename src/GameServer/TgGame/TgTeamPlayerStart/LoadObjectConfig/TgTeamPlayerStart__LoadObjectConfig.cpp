#include "src/GameServer/TgGame/TgTeamPlayerStart/LoadObjectConfig/TgTeamPlayerStart__LoadObjectConfig.hpp"
#include "src/GameServer/Engine/MapObjectConfig/MapObjectConfig.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgTeamPlayerStart__LoadObjectConfig::Call(ATgTeamPlayerStart* PlayerStart, void* edx) {
	if (PlayerStart == nullptr) return;
	const int mid = PlayerStart->m_nMapObjectId;

	// Each field defaults to whatever the editor set (i.e. whatever was
	// already in memory before this hook ran). The override is applied
	// only when MapObjectConfig has a row for this column.
	PlayerStart->bEnabled      =          (bool)MapObjectConfig::GetInt(mid, "b_enabled",       PlayerStart->bEnabled);
	PlayerStart->m_nTaskForce  = (unsigned char)MapObjectConfig::GetInt(mid, "m_n_task_force",  PlayerStart->m_nTaskForce);
	PlayerStart->m_eCoalition  = (unsigned char)MapObjectConfig::GetInt(mid, "m_e_coalition",   PlayerStart->m_eCoalition);
	PlayerStart->m_nPriority   =                MapObjectConfig::GetInt(mid, "m_n_priority",    PlayerStart->m_nPriority);
	PlayerStart->nPrevPriority =                MapObjectConfig::GetInt(mid, "n_prev_priority", PlayerStart->nPrevPriority);
	PlayerStart->m_nMinLevel   =                MapObjectConfig::GetInt(mid, "m_n_min_level",   PlayerStart->m_nMinLevel);

	Logger::Log("config",
		"TgTeamPlayerStart::LoadObjectConfig — map_object_id=%d tf=%d coalition=%d priority=%d prevPriority=%d minLevel=%d\n",
		mid, (int)PlayerStart->m_nTaskForce, (int)PlayerStart->m_eCoalition,
		PlayerStart->m_nPriority, PlayerStart->nPrevPriority, PlayerStart->m_nMinLevel);
}
