#include "src/GameServer/TgGame/TgGame_Ticket/LoadGameConfig/TgGame_Ticket__LoadGameConfig.hpp"
#include "src/GameServer/TgGame/TgGame/LoadGameConfig/TgGame__LoadGameConfig.hpp"
#include "src/Utils/Logger/Logger.hpp"

// TgGame_Ticket LoadGameConfig is stripped on this binary.
// Sets the ticket-mode win threshold and per-tick cadence.
//
// r_nPointsToWin is replicated in the bNetInitial block, so it must be
// finalised before the first client connects — LoadGameConfig runs during
// PostBeginPlay, well ahead of any PlayerController, so this is safe.
void __fastcall TgGame_Ticket__LoadGameConfig::Call(ATgGame_Ticket* Game, void* edx) {
	LogCallBegin();
	if (Game == nullptr) { LogCallEnd(); return; }

	Game->m_fTicketTickDelay   = 1.0f;
	Game->m_bDefendersAhead    = 1;
	Game->m_bTeamsTied         = 1;
	Game->m_bTicketCountReached = 0;
	Game->m_nTfControlledLast  = 0;

	TgGame__LoadGameConfig::LoadCommonGameConfig(Game);

	Game->m_fGameMissionTime = 15 * 60.0f;   // 15 minute mission
	Game->m_fGameOvertimeTime = 4 * 60.0f;  // up to 4 minutes overtime
	Game->m_bAllowOvertime = 1;

	ATgRepInfo_Game* GRI = reinterpret_cast<ATgRepInfo_Game*>(Game->GameReplicationInfo);
	if (GRI != nullptr) {
		GRI->r_nPointsToWin = 800;
		Logger::Log(GetLogChannel(), "TgGame_Ticket::LoadGameConfig — r_nPointsToWin=%d tickDelay=%.2f\n",
			GRI->r_nPointsToWin, Game->m_fTicketTickDelay);
	}

	LogCallEnd();
}
