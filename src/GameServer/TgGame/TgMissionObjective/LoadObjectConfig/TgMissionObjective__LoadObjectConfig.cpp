#include "src/GameServer/TgGame/TgMissionObjective/LoadObjectConfig/TgMissionObjective__LoadObjectConfig.hpp"
#include "src/GameServer/Engine/MapObjectConfig/MapObjectConfig.hpp"
#include "src/Config/Config.hpp"
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

	// n_priority override. The baked map sometimes leaves scripted objectives
	// (e.g. a kismet-spawned boss) at priority 1, which makes PostBeginPlay's
	// UnlockObjective(1) + TgGame_Defense's per-objective UnlockObjective() loop
	// activate them at map load. Setting n_priority<=0 takes them out of the
	// unlock sequence (both paths skip nPriority<=0) so only kismet flips them.
	// Runs in LoadObjectConfig (objective PostBeginPlay), before the game's
	// PostBeginPlay unlock pass.
	const int prevPriority = Obj->nPriority;
	Obj->nPriority = MapObjectConfig::GetInt(mid, "n_priority", Obj->nPriority);

	if (Obj->nPriority != prevPriority) {
		Logger::Log("config",
			"TgMissionObjective::LoadObjectConfig — map_object_id=%d n_priority %d -> %d\n",
			mid, prevPriority, Obj->nPriority);
	}

	std::string map = Config::GetMapNameChar();
	if (map == "Rot_Redistribution03"
		|| map == "Rot_Redistribution05"
		|| map == "Rot_Redistribution04") {
		Obj->r_bUsePendingState = 1;
		Obj->r_bIsPending = 0;
	}
}
