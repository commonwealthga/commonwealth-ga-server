#include "src/GameServer/TgGame/TgGame_Arena/LoadGameConfig/TgGame_Arena__LoadGameConfig.hpp"
#include "src/GameServer/TgGame/TgGame/LoadGameConfig/TgGame__LoadGameConfig.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall TgGame_Arena__LoadGameConfig::Call(ATgGame_Arena* Game, void* edx) {
	LogCallBegin();

	// Wave interval: UC default 20s, both sides. s_UseCustomReviveTimer stays
	// at the UC defaults (Arena true; Defense/PointRotation/CTF false).
	Game->m_nSecsToAutoRelease = 15;
	Game->m_nSecsToAutoReleaseAttackers = Game->m_nSecsToAutoRelease;
	Game->m_nSecsToAutoReleaseDefenders = Game->m_nSecsToAutoRelease;
	Game->m_bIsTutorialMap = 0;

	TgGame__LoadGameConfig::LoadCommonGameConfig(Game);


	Game->m_fGameMissionTime = 15 * 60.0f;   // 15 minute mission
	Game->m_fGameOvertimeTime = 4 * 60.0f;  // up to 4 minutes overtime
	Game->m_bAllowOvertime = 1;

	// Game->s_nRoundNumber = 1;
	// Game->s_nBetweenRoundDelay = 15;
	// Game->s_nRoundSetupTime = 15;
	// Game->s_nObjectiveUnlockDelay = 15;
	// Game->s_bResetPlayersBetweenRounds = 0;
	// Game->s_bResetPlayfieldBetweenRounds = 0;
	// Game->s_bDisplayEndRoundScreen = 0;
	LogCallEnd();
}

