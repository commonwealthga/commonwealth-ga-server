#include "src/GameServer/TgGame/TgGame_Defense/LoadGameConfig/TgGame_Defense__LoadGameConfig.hpp"
#include "src/GameServer/TgGame/TgGame/LoadGameConfig/TgGame__LoadGameConfig.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Defense mode game config. Sets Defense-specific values on top of Arena base.
// UC defaultproperties: s_nMaxRoundNumber=1, s_fRoundDuration=120.0, s_nBetweenRoundDelay=30
void __fastcall TgGame_Defense__LoadGameConfig::Call(ATgGame_Defense* Game, void* edx) {
	LogCallBegin();

	// TgGame__LoadGameConfig::Call(Game, edx);
	Game->m_GameType = 11; // GT_DEFENSE
	// Game->s_bSecuringMode = 1;

	LogCallEnd();
}
