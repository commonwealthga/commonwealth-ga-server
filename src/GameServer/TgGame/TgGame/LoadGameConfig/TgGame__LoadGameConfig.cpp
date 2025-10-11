#include "src/GameServer/TgGame/TgGame/LoadGameConfig/TgGame__LoadGameConfig.hpp"

void __fastcall* TgGame__LoadGameConfig::Call(ATgGame* Game, void* edx) {
	Game->m_nSecsToAutoRelease = 15;
	Game->m_nSecsToAutoReleaseAttackers = 15;
	Game->m_nSecsToAutoReleaseDefenders = 15;

	Game->m_fMissionTime = 15 * 60;
	Game->m_fGameMissionTime = 15 * 60;
}

