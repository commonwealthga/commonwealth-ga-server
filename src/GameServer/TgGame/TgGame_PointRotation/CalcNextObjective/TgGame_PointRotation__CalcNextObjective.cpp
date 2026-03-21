#include "src/GameServer/TgGame/TgGame_PointRotation/CalcNextObjective/TgGame_PointRotation__CalcNextObjective.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Picks a random objective from GRI->m_MissionObjectives, excluding m_PreviousObjective.
// Sets m_NextObjective to the chosen objective and m_PreviousObjective for next round.
// All PointRotation objectives have nPriority=1, so no priority filtering is needed.
void __fastcall TgGame_PointRotation__CalcNextObjective::Call(ATgGame_PointRotation* Game, void* edx) {
	LogCallBegin();

	ATgRepInfo_Game* GRI = reinterpret_cast<ATgRepInfo_Game*>(Game->GameReplicationInfo);
	if (GRI == nullptr || GRI->m_MissionObjectives.Count == 0) {
		Logger::Log(GetLogChannel(), "No objectives to choose from\n");
		LogCallEnd();
		return;
	}

	// Build candidate list: all objectives except the previous one
	ATgMissionObjective* candidates[64];
	int numCandidates = 0;

	for (int i = 0; i < GRI->m_MissionObjectives.Count && numCandidates < 64; i++) {
		ATgMissionObjective* Obj = GRI->m_MissionObjectives.Data[i];
		if (Obj != nullptr && Obj != Game->m_PreviousObjective) {
			candidates[numCandidates++] = Obj;
		}
	}

	if (numCandidates == 0) {
		// Only one objective total — reuse it
		Game->m_NextObjective = Game->m_PreviousObjective;
		Logger::Log(GetLogChannel(), "Only one objective total\n");
		LogCallEnd();
		return;
	}

	// Pick a random candidate
	int index = rand() % numCandidates;
	Game->m_NextObjective = candidates[index];
	Game->m_PreviousObjective = Game->m_NextObjective;

	Logger::Log(GetLogChannel(), "Chosen objective: %s\n", Game->m_NextObjective->GetFullName());

	// Update r_Objectives so newly connecting clients only see the current objective
	GRI->r_Objectives[0] = Game->m_NextObjective;
	for (int i = 1; i < 0x4B; i++) {
		GRI->r_Objectives[i] = nullptr;
	}

	LogCallEnd();
}


