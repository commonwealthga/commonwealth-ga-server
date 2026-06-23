#include "src/GameServer/TgGame/TgGame_Defense/CheckWinRound/TgGame_Defense__CheckWinRound.hpp"
#include "src/GameServer/TgGame/TgGame_Defense/ObjectiveBotOutcome.hpp"
#include "src/GameServer/TgGame/TgGame/BeginEndMission/TgGame__BeginEndMission.hpp"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Defense CheckWinRound:
// - attackers win a round by capturing all active objectives
// - defenders win a timed round when its RoundTimer has expired
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

	// Boss/escort objective-bot outcome = GAME over, not just round over, so we
	// end the mission directly here. CheckWinRound is the only point that
	// reliably runs every frame and sees the deciding state; the round →
	// PostRound → CheckWinGame → BeginEndMission path does NOT fire in this
	// build (the redeclared Defense PostRound state never executes the inherited
	// BeginState, so CheckWinGame is never reached — verified by logging). This
	// mirrors the canonical base TgGame.ObjectiveTaken, which calls
	// BeginEndMission() the instant the deciding objective is taken.
	// Boss (attacker objective) destroyed → Defenders win; defended NPC
	// destroyed → Attackers win. BeginEndMissionImpl is idempotent
	// (s_bGameEndMissionProcessed) so the per-frame call is safe, and it applies
	// the boss-death end-screen delay via HasCapturedBossObjective.
	const int botOutcome = ObjectiveBotOutcome(GRI);
	if (botOutcome != 0) {
		const bool bDefendersWin = (botOutcome == 1);
		*Winner = bDefendersWin ? GTeamsData.Defenders : GTeamsData.Attackers;
		Game->m_GameWinState = bDefendersWin ? 1 : 2;
		GRI->r_Winner = *Winner;
		Logger::Log("gametimer",
			"CheckWinRound: objective-bot outcome -> %s win — ending mission directly\n",
			bDefendersWin ? "Defenders" : "Attackers");
		BeginEndMissionImpl((ATgGame*)Game, nullptr);
		LogCallEnd();
		return true;
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
		Logger::Log("gametimer", "CheckWinRound: all objectives captured, attacker wins\n");
		LogCallEnd();
		return true;
	}

	const float roundDuration = (Game->s_nRoundNumber >= Game->s_nMaxRoundNumber)
		? 0.0f
		: Game->s_fRoundDuration;
	if (roundDuration > 0.0f && !Game->IsTimerActive(FName("RoundTimer"), nullptr)) {
		*Winner = GTeamsData.Defenders;
		Logger::Log("gametimer",
			"CheckWinRound: timed round expired, defenders win round=%d duration=%.2f\n",
			Game->s_nRoundNumber, roundDuration);
		LogCallEnd();
		return true;
	}

	LogCallEnd();
	return false;
}
