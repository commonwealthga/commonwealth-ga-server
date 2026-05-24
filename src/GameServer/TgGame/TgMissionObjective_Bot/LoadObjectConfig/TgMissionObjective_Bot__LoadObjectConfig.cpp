#include "src/GameServer/TgGame/TgMissionObjective_Bot/LoadObjectConfig/TgMissionObjective_Bot__LoadObjectConfig.hpp"
#include "src/GameServer/Engine/MapObjectConfig/MapObjectConfig.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgMissionObjective_Bot__LoadObjectConfig::Call(ATgMissionObjective_Bot* Obj, void* edx) {
	if (Obj == nullptr) return;

	// The binary's body at 0x10a7c440 is intact — call it first so whatever
	// it populates is in place before we override.
	CallOriginal(Obj, edx);

	const int mid = Obj->m_nMapObjectId;

	Obj->s_nBotId        = MapObjectConfig::GetInt  (mid, "s_n_bot_id",          Obj->s_nBotId);
	Obj->s_nSpawnTableId = MapObjectConfig::GetInt  (mid, "s_n_spawn_table_id",  Obj->s_nSpawnTableId);
	Obj->s_bAutoSpawn    = (bool)MapObjectConfig::GetInt(mid, "s_b_auto_spawn",  Obj->s_bAutoSpawn);
	Obj->bPatrolLoop     = (bool)MapObjectConfig::GetInt(mid, "b_patrol_loop",   Obj->bPatrolLoop);
	Obj->nGlobalAlarmId  = MapObjectConfig::GetInt  (mid, "n_global_alarm_id",   Obj->nGlobalAlarmId);
	Obj->fBalance        = MapObjectConfig::GetFloat(mid, "f_balance",           Obj->fBalance);

	Logger::Log("tgmissionobjective_bot",
		"[%s] %s LoadObjectConfig mapObjectId=%d botId=%d spawnTable=%d autoSpawn=%d "
		"defaultOwnerTaskForce=%d (parent UC field)\n",
		Logger::GetTime(), Obj->GetName(), mid,
		Obj->s_nBotId, Obj->s_nSpawnTableId, (int)Obj->s_bAutoSpawn,
		Obj->nDefaultOwnerTaskForce);
}
