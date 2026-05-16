#include "src/GameServer/TgGame/TgGame/CheckRandomObjectives/TgGame__CheckRandomObjectives.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Called at PostBeginPlay. Copies the server-side m_MissionObjectives list
// into the replicated r_Objectives[75] so clients can render every objective
// on the HUD/map. PointRotation overrides this later in CalcNextObjective
// (where it intentionally exposes only the currently-active objective).
//
// Also marks all objectives as s_bRandomPicked=1 (used by CalcNextObjective
// to pick from the random-eligible pool).
void __fastcall TgGame__CheckRandomObjectives::Call(ATgGame* Game, void* edx) {
	LogCallBegin();

	ATgRepInfo_Game* GRI = reinterpret_cast<ATgRepInfo_Game*>(Game->GameReplicationInfo);
	if (GRI == nullptr) {
		Logger::Log(GetLogChannel(), "GRI is nullptr\n");
		LogCallEnd();
		return;
	}

	int objIdx = 0;
	for (int i = 0; i < GRI->m_MissionObjectives.Count && objIdx < 0x4B; i++) {
		ATgMissionObjective* Obj = GRI->m_MissionObjectives.Data[i];
		if (Obj != nullptr) {
			GRI->r_Objectives[objIdx] = Obj;
			objIdx++;
		}
	}
	Logger::Log(GetLogChannel(), "r_Objectives populated: %d entries (m_MissionObjectives.Count=%d)\n", objIdx, GRI->m_MissionObjectives.Count);

	for (int i = 0; i < GRI->m_MissionObjectives.Count; i++) {
		ATgMissionObjective* Obj = GRI->m_MissionObjectives.Data[i];
		if (Obj != nullptr) {
			Obj->s_bRandomPicked = 1;
		}
	}

	LogCallEnd();
}


