#include "src/GameServer/TgGame/TgGame_Control/TickCountdownCalculation/TgGame_Control__TickCountdownCalculation.hpp"
#include "src/GameServer/TgGame/TgGame/UpdateMissionTimerEventWinVar/TgGame__UpdateMissionTimerEventWinVar.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgGame_Control__TickCountdownCalculation::Call(ATgGame_Control* Game, void* edx, float DeltaTime) {
	LogCallBegin();

	// bGameEnded: mission already over (BeginEndMission ran) — don't keep
	// counting down / spamming countdown messages during the end window.
	if (Game == nullptr || !Game->m_bCountdownEnabled || Game->bGameEnded) {
		LogCallEnd();
		return;
	}

	float oldTimeRemaining = Game->m_fCurrentCountdownTime;
	if (oldTimeRemaining <= 0.0f) {
		oldTimeRemaining = Game->m_fControlTimeToWin;
		if (oldTimeRemaining <= 0.0f) oldTimeRemaining = 30.0f;
	}

	float newTimeRemaining = oldTimeRemaining - DeltaTime;
	if (newTimeRemaining < 0.0f) newTimeRemaining = 0.0f;

	Game->m_fCurrentCountdownTime = newTimeRemaining;
	Game->SendCountdownRemainingMessages(oldTimeRemaining, newTimeRemaining);

	if (newTimeRemaining <= 0.0f) {
		Game->m_bCountdownEnabled = 0;
		Game->m_bShouldWait = 0;
		Game->m_GameWinState = 2; // GWS_ATTACKER_WON
		TgGame__UpdateMissionTimerEventWinVar::Call(Game, nullptr);
		Game->BeginEndMission(0, nullptr, 0.0f);
		Logger::Log("control", "TickCountdownCalculation: attackers held final objective, ending mission\n");
	}

	LogCallEnd();
}
