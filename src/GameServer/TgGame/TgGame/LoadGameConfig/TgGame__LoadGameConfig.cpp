#include "src/GameServer/TgGame/TgGame/LoadGameConfig/TgGame__LoadGameConfig.hpp"
#include "src/Utils/Logger/Logger.hpp"

void __fastcall* TgGame__LoadGameConfig::Call(ATgGame* Game, void* edx) {
	Logger::Log("debug", "MINE TgGame__LoadGameConfig START\n");
	Game->m_nSecsToAutoRelease = 15;
	Game->m_nSecsToAutoReleaseAttackers = 15;
	Game->m_nSecsToAutoReleaseDefenders = 15;
	Game->m_bIsTutorialMap = 0;

	Logger::Log("debug", "MINE TgGame__LoadGameConfig END\n");
}

