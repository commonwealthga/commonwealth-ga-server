#include "src/GameServer/TgGame/TgGame_Defense/CheckWinRound/TgGame_Defense__CheckWinRound.hpp"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Defense CheckWinRound: checks if all objectives have been captured by attackers
// (status 8 = attacker captured). In Defense/Raid mode, the round ends when all
// objectives are taken. Defenders "win" a round if the round timer expires with
// objectives still standing (handled by the Arena state machine timer, not here).
//
// r_eStatus values: 7 = defender captured/held, 8 = attacker captured/destroyed
bool __fastcall TgGame_Defense__CheckWinRound::Call(ATgGame_Defense* Game, void* edx, ATgRepInfo_TaskForce** Winner) {
	LogCallBegin();

	*Winner = nullptr;

	ATgRepInfo_Game* GRI = (ATgRepInfo_Game*)Game->GameReplicationInfo;
	if (GRI == nullptr || GRI->m_MissionObjectives.Count == 0) {
		LogCallEnd();
		return false;
	}

	// Check if all active objectives are captured by attackers
	bool bAllCapturedByAttacker = true;
	bool bHasActiveObjective = false;

	for (int i = 0; i < GRI->m_MissionObjectives.Count; i++) {
		ATgMissionObjective* Obj = GRI->m_MissionObjectives.Data[i];
		if (Obj == nullptr) continue;
		if (!Obj->r_bIsActive) continue;

		bHasActiveObjective = true;
		if (Obj->r_eStatus != 8) {
			bAllCapturedByAttacker = false;
			break;
		}
	}

	if (bHasActiveObjective && bAllCapturedByAttacker) {
		// Attackers captured everything — attackers win the round
		*Winner = GTeamsData.Attackers;
		Logger::Log("defense", "CheckWinRound: all objectives captured, attacker wins\n");
		LogCallEnd();
		return true;
	}

	LogCallEnd();
	return false;
}
