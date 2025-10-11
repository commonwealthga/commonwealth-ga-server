#include "src/GameServer/TgGame/TgGame_Arena/LoadGameConfig/TgGame_Arena__LoadGameConfig.hpp"

void __fastcall* TgGame_Arena__LoadGameConfig::Call(ATgGame_Arena* Game, void* edx) {
	Game->m_nSecsToAutoRelease = 15;
	Game->m_nSecsToAutoReleaseAttackers = 15;
	Game->m_nSecsToAutoReleaseDefenders = 15;

	Game->m_fMissionTime = 15 * 60;
	Game->m_fGameMissionTime = 15 * 60;

	Game->s_nRoundNumber = 1;
	Game->s_nBetweenRoundDelay = 15;
	Game->s_nRoundSetupTime = 15;
	Game->s_nObjectiveUnlockDelay = 15;
	Game->s_bResetPlayersBetweenRounds = 0;
	Game->s_bResetPlayfieldBetweenRounds = 0;
	Game->s_bDisplayEndRoundScreen = 0;
}

