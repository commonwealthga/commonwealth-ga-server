#include "src/GameServer/TgGame/TgGame/CheckRandomObjectives/TgGame__CheckRandomObjectives.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Called at PostBeginPlay to mark objectives for random selection.
// If GRI->m_MissionObjectives is empty (AddObjectivePointToList stub never ran),
// we scan GObjObjects and populate it manually using the native function.
// Then marks all objectives as s_bRandomPicked=1 for CalcNextObjective.
void __fastcall TgGame__CheckRandomObjectives::Call(ATgGame* Game, void* edx) {
	LogCallBegin();

	ATgRepInfo_Game* GRI = reinterpret_cast<ATgRepInfo_Game*>(Game->GameReplicationInfo);
	if (GRI == nullptr) {
		Logger::Log(GetLogChannel(), "GRI is nullptr\n");
		LogCallEnd();
		return;
	}

	// If objectives weren't self-registered via AddToList/AddObjectivePointToList,
	// scan GObjObjects and add them manually.
	if (GRI->m_MissionObjectives.Count == 0) {

		Logger::Log(GetLogChannel(), "GRI->m_MissionObjectives is empty, populating manually\n");

		using AddObjectivePointToList_t = void(__fastcall*)(ATgRepInfo_Game*, void*, ATgMissionObjective*);
		auto AddObjectivePointToList = reinterpret_cast<AddObjectivePointToList_t>(0x109f0580);

		for (int i = 0; i < UObject::GObjObjects()->Count; i++) {
			UObject* obj = UObject::GObjObjects()->Data[i];
			if (obj && strstr(obj->Class->GetFullName(), "Class TgGame.TgMissionObjective")) {
				if (strstr(obj->GetFullName(), "Default__")) continue;

				ATgMissionObjective* Objective = reinterpret_cast<ATgMissionObjective*>(obj);
				AddObjectivePointToList(GRI, nullptr, Objective);
				Logger::Log(GetLogChannel(), "Added objective %s to GRI->m_MissionObjectives\n", Objective->GetFullName());
			}
		}
	}

	// For PointRotation, CalcNextObjective manages r_Objectives (only the active objective).
	// For other game types, populate r_Objectives from m_MissionObjectives now.
	// bool bIsPointRotation = strstr(Game->Class->GetFullName(), "TgGame_PointRotation") != nullptr;
	// if (!bIsPointRotation) {
		int objIdx = 0;
		for (int i = 0; i < GRI->m_MissionObjectives.Count && objIdx < 0x4B; i++) {
			ATgMissionObjective* Obj = GRI->m_MissionObjectives.Data[i];
			if (Obj != nullptr && GRI->r_Objectives[objIdx] == nullptr) {
				GRI->r_Objectives[objIdx] = Obj;
				objIdx++;
				break;
			}
		}
		// }
		Logger::Log(GetLogChannel(), "r_Objectives populated: %d entries (m_MissionObjectives.Count=%d)\n", objIdx, GRI->m_MissionObjectives.Count);
	// } else {
	// 	Logger::Log(GetLogChannel(), "PointRotation: skipping r_Objectives population (CalcNextObjective will set it)\n");
	// }

	// Mark all objectives as candidates for random selection
	for (int i = 0; i < GRI->m_MissionObjectives.Count; i++) {
		ATgMissionObjective* Obj = GRI->m_MissionObjectives.Data[i];
		if (Obj != nullptr) {
			Obj->s_bRandomPicked = 1;
		}
	}

	LogCallEnd();
}


