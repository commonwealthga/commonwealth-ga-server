#include "src/GameServer/TgGame/TgGame_Defense/LoadGameConfig/TgGame_Defense__LoadGameConfig.hpp"
#include "src/GameServer/TgGame/TgGame/LoadGameConfig/TgGame__LoadGameConfig.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Defense mode game config. Sets Defense-specific values on top of Arena base.
// UC defaultproperties: s_nMaxRoundNumber=1, s_fRoundDuration=120.0, s_nBetweenRoundDelay=30
void __fastcall TgGame_Defense__LoadGameConfig::Call(ATgGame_Defense* Game, void* edx) {
	LogCallBegin();

	// Base config (timers, auto-release, etc.)
	Game->m_nSecsToAutoRelease = 10;
	Game->m_nSecsToAutoReleaseAttackers = 10;
	Game->m_nSecsToAutoReleaseDefenders = 10;
	Game->m_bIsTutorialMap = 0;

	TgGame__LoadGameConfig::LoadCommonGameConfig(Game);

	Game->m_fGameMissionTime = 15 * 60.0f;
	Game->m_fGameOvertimeTime = 0.0f;  // Defense typically has no overtime
	Game->m_bAllowOvertime = 0;

	// Defense-specific defaults from UC
	Game->s_nMaxRoundNumber = 1;
	Game->s_fRoundDuration = 120.0f;
	Game->s_nBetweenRoundDelay = 30;
	Game->s_nRoundSetupTime = 5;
	Game->s_nObjectiveUnlockDelay = 0;

	ATgRepInfo_Game* GRI = (ATgRepInfo_Game*)Game->GameReplicationInfo;
	if (GRI != nullptr) {
		GRI->r_nMaxRoundNumber = Game->s_nMaxRoundNumber;
	}

	Logger::Log("gameconfig", "Defense LoadGameConfig: maxRounds=%d, roundDuration=%.0f, betweenRoundDelay=%d\n",
		Game->s_nMaxRoundNumber, Game->s_fRoundDuration, Game->s_nBetweenRoundDelay);

	LogCallEnd();
}
