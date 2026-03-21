#include "src/GameServer/TgGame/TgGame_Arena/FinalizeGameScore/TgGame_Arena__FinalizeGameScore.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Called when the game ends. Sets m_GameWinState and GRI->r_Winner.
// GWS_DEFENDER_WON=1, GWS_ATTACKER_WON=2, GWS_TIE=3
// Attacker task force has r_nTaskForce=1, defender has r_nTaskForce=2.
void __fastcall TgGame_Arena__FinalizeGameScore::Call(ATgGame_Arena* Game, void* edx, ATgRepInfo_TaskForce* Winner) {
	LogCallBegin();

	ATgRepInfo_Game* GRI = reinterpret_cast<ATgRepInfo_Game*>(Game->GameReplicationInfo);

	if (Winner == nullptr) {
		Game->m_GameWinState = 3; // GWS_TIE
	} else if (Winner->r_nTaskForce == 1) {
		Game->m_GameWinState = 2; // GWS_ATTACKER_WON
	} else {
		Game->m_GameWinState = 1; // GWS_DEFENDER_WON
	}

	if (GRI != nullptr) {
		GRI->r_Winner = Winner;
		GRI->bNetDirty = 1;
		GRI->bForceNetUpdate = 1;
	}

	LogCallEnd();
}


