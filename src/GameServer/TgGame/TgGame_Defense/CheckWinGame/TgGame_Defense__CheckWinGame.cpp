#include "src/GameServer/TgGame/TgGame_Defense/CheckWinGame/TgGame_Defense__CheckWinGame.hpp"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Defense CheckWinGame: determines if the game is over.
// In Defense/Raid mode:
// - Attackers win the game if they capture all objectives (any round)
// - Defenders win if they survive all rounds (s_nMaxRoundNumber reached)
// - If bForceWin (timer expired), defenders win by default (they held)
//
// Uses the Arena point system: r_nCurrentPointCount vs r_nPointsToWin
bool __fastcall TgGame_Defense__CheckWinGame::Call(ATgGame_Defense* Game, void* edx, unsigned long bForceWin, ATgRepInfo_TaskForce** Winner) {
	LogCallBegin();

	*Winner = nullptr;

	ATgRepInfo_Game* GRI = (ATgRepInfo_Game*)Game->GameReplicationInfo;
	if (GRI == nullptr) {
		LogCallEnd();
		return false;
	}

	// Check if either team has reached the points-to-win threshold
	if (GTeamsData.Attackers != nullptr && GTeamsData.Attackers->r_nCurrentPointCount >= GRI->r_nPointsToWin && GRI->r_nPointsToWin > 0) {
		*Winner = GTeamsData.Attackers;
		Logger::Log("defense", "CheckWinGame: attacker reached %d points (need %d)\n",
			GTeamsData.Attackers->r_nCurrentPointCount, GRI->r_nPointsToWin);
		LogCallEnd();
		return true;
	}

	if (GTeamsData.Defenders != nullptr && GTeamsData.Defenders->r_nCurrentPointCount >= GRI->r_nPointsToWin && GRI->r_nPointsToWin > 0) {
		*Winner = GTeamsData.Defenders;
		Logger::Log("defense", "CheckWinGame: defender reached %d points (need %d)\n",
			GTeamsData.Defenders->r_nCurrentPointCount, GRI->r_nPointsToWin);
		LogCallEnd();
		return true;
	}

	// If forcing (timer expired / all rounds played), defenders win by default
	if (bForceWin) {
		*Winner = GTeamsData.Defenders;
		Logger::Log("defense", "CheckWinGame: forced win, defenders hold\n");
		LogCallEnd();
		return true;
	}

	LogCallEnd();
	return false;
}
