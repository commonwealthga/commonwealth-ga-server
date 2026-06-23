#include "src/GameServer/TgGame/TgGame_Defense/CheckWinGame/TgGame_Defense__CheckWinGame.hpp"
#include "src/GameServer/TgGame/TgGame_Defense/ObjectiveBotOutcome.hpp"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/Utils/Logger/Logger.hpp"

namespace {

bool AllActiveObjectivesCapturedByAttackers(ATgRepInfo_Game* GRI) {
	if (GRI == nullptr || GRI->m_MissionObjectives.Num() == 0) return false;

	bool hasActiveObjective = false;
	for (int i = 0; i < GRI->m_MissionObjectives.Num(); i++) {
		ATgMissionObjective* Objective = GRI->m_MissionObjectives.Data[i];
		if (Objective == nullptr || !Objective->r_bIsActive) continue;

		hasActiveObjective = true;
		if (Objective->r_eStatus != 8) return false;
	}

	return hasActiveObjective;
}

}

// Defense CheckWinGame: determines if the game is over.
// In Defense/Raid mode:
// - Attackers win the game if they capture all objectives (any round)
// - Defenders win when the mode explicitly forces a win after attackers fail
// - If bForceWin (timer expired), defenders win by default (they held)
//
// Defense's final round can be untimed, so reaching s_nMaxRoundNumber is not
// itself a win condition.
bool __fastcall TgGame_Defense__CheckWinGame::Call(ATgGame_Defense* Game, void* edx, unsigned long bForceWin, ATgRepInfo_TaskForce** Winner) {
	LogCallBegin();

	if (Winner == nullptr) {
		LogCallEnd();
		return false;
	}

	*Winner = nullptr;

	ATgRepInfo_Game* GRI = (ATgRepInfo_Game*)Game->GameReplicationInfo;
	if (GRI == nullptr) {
		LogCallEnd();
		return false;
	}

	// Boss/escort objective-bot outcome first: killing the attacker boss = a
	// Defenders win; losing the defended NPC = an Attackers win. The canonical
	// flow (ObjectiveTaken → GotoState('PostRound') → TgGame_Arena::PostRound.
	// BeginState → CheckWinGame → FinalizeGameScore + BeginEndMission) ends the
	// mission from here, and BeginEndMissionImpl applies the boss-death screen
	// delay. m_GameWinState (1=Def,2=Att) is seeded so the end screen reads right.
	const int botOutcome = ObjectiveBotOutcome(GRI);
	if (botOutcome != 0) {
		const bool bDefendersWin = (botOutcome == 1);
		*Winner = bDefendersWin ? GTeamsData.Defenders : GTeamsData.Attackers;
		Game->m_GameWinState = bDefendersWin ? 1 : 2;
		GRI->r_Winner = *Winner;
		Logger::Log("gametimer",
			"CheckWinGame: objective-bot outcome -> %s win\n",
			bDefendersWin ? "Defenders" : "Attackers");
		LogCallEnd();
		return true;
	}

	if (AllActiveObjectivesCapturedByAttackers(GRI)) {
		*Winner = GTeamsData.Attackers;
		Logger::Log("gametimer", "CheckWinGame: all active objectives captured, attackers win\n");
		LogCallEnd();
		return true;
	}

	// Check attacker threshold for maps/configs that still use points.
	if (GTeamsData.Attackers != nullptr && GTeamsData.Attackers->r_nCurrentPointCount >= GRI->r_nPointsToWin && GRI->r_nPointsToWin > 0) {
		*Winner = GTeamsData.Attackers;
		Logger::Log("gametimer", "CheckWinGame: attacker reached %d points (need %d)\n",
			GTeamsData.Attackers->r_nCurrentPointCount, GRI->r_nPointsToWin);
		LogCallEnd();
		return true;
	}

	// If forcing (timer expired / all rounds played), defenders win by default
	if (bForceWin) {
		*Winner = GTeamsData.Defenders;
		Logger::Log("gametimer", "CheckWinGame: forced win, defenders hold\n");
		LogCallEnd();
		return true;
	}

	LogCallEnd();
	return false;
}
