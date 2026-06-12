#include "src/GameServer/TgGame/TgGame_Ticket/BeginEndMission/TgGame_Ticket__BeginEndMission.hpp"
#include "src/GameServer/TgGame/TgGame/BeginEndMission/TgGame__BeginEndMission.hpp"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Ticket's BeginEndMission native is a separate `_notimplemented` stub from
// the base TgGame one — UC dispatches by subclass-vtable so we must hook both
// addresses. UC ObjectiveTaken (which sets m_GameWinState for objective modes)
// never runs for Ticket, so we resolve win-state from Ticket-only fields
// (m_bTeamsTied / r_Winner set by our UpdateGameWinState) and stamp it onto
// m_GameWinState before delegating to the shared impl.
bool __fastcall TgGame_Ticket__BeginEndMission::Call(ATgGame_Ticket* Game, void* edx, unsigned long bClearNextMapGame, ACameraActor* endMissionCamera, float fDelayOverride) {
	LogCallBegin();

	if (Game != nullptr) {
		ATgRepInfo_Game* GRI = (ATgRepInfo_Game*)Game->GameReplicationInfo;
		unsigned char winState = 3;  // default = tie
		if (!Game->m_bTeamsTied && GRI && GRI->r_Winner) {
			if (GRI->r_Winner == GTeamsData.Attackers)      winState = 2;
			else if (GRI->r_Winner == GTeamsData.Defenders) winState = 1;
			else if (GRI->r_Winner->r_eCoalition == 1)      winState = 2;
			else if (GRI->r_Winner->r_eCoalition == 2)      winState = 1;
		}
		Game->m_GameWinState = winState;
	}

	BeginEndMissionImpl(Game, endMissionCamera, fDelayOverride);

	LogCallEnd();
	return true;
}
