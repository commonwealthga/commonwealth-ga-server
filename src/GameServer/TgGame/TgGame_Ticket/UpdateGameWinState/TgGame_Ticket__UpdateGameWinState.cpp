#include "src/GameServer/TgGame/TgGame_Ticket/UpdateGameWinState/TgGame_Ticket__UpdateGameWinState.hpp"
#include "src/GameServer/TgGame/TgGame_Ticket/BeginEndMission/TgGame_Ticket__BeginEndMission.hpp"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Stripped native. Called from UC TgGame_Ticket.MissionTimer and from our
// TickTicketsCalculation. Updates the standings booleans the UC reads from
// (m_bDefendersAhead / m_bTeamsTied), and once either side reaches the
// r_nPointsToWin threshold sets r_Winner / m_bTicketCountReached and fires
// BeginEndMission to drop into the end-mission flow.
//
// UC TgGame_Ticket has its own BeginEndMission native; stock UC only invokes
// it from MissionTimer expiry (TgGame.uc:1008/1016 — game time / overtime
// ending). Without an explicit fire here, ticket-target wins are silently
// recorded and the match keeps running until the timer runs out — which is
// the symptom: tickets cross 800 but UI never ends and counters keep climbing.
// The m_bTicketCountReached one-shot guard prevents double-fire even though
// BeginEndMissionImpl has its own s_bGameEndMissionProcessed guard downstream.
void __fastcall TgGame_Ticket__UpdateGameWinState::Call(ATgGame_Ticket* Game, void* edx) {
	if (Game == nullptr) return;

	ATgRepInfo_Game* GRI = reinterpret_cast<ATgRepInfo_Game*>(Game->GameReplicationInfo);
	if (GRI == nullptr) return;
	if (GTeamsData.Attackers == nullptr || GTeamsData.Defenders == nullptr) return;

	const int att    = GTeamsData.Attackers->r_nCurrentPointCount;
	const int def    = GTeamsData.Defenders->r_nCurrentPointCount;
	const int target = GRI->r_nPointsToWin;

	Game->m_bTeamsTied      = (att == def) ? 1u : 0u;
	Game->m_bDefendersAhead = (def >  att) ? 1u : 0u;

	if (!Game->m_bTicketCountReached && target > 0 && (att >= target || def >= target)) {
		Game->m_bTicketCountReached = 1;
		GRI->r_Winner = (att >= target) ? GTeamsData.Attackers : GTeamsData.Defenders;
		Logger::Log(GetLogChannel(),
			"UpdateGameWinState — TICKET TARGET REACHED: %s wins (att=%d target=%d, def=%d) — invoking BeginEndMission\n",
			(att >= target) ? "Attackers" : "Defenders", att, target, def);

		// Route through the Ticket subclass entry point — its hook resolves
		// the winner from Ticket-specific fields (m_bTeamsTied / r_Winner) and
		// stamps m_GameWinState before delegating to BeginEndMissionImpl, same
		// path UC's MissionTimer-driven BeginEndMission() would take.
		TgGame_Ticket__BeginEndMission::Call(
			Game, /*edx=*/nullptr,
			/*bClearNextMapGame=*/0,
			/*endMissionCamera=*/nullptr,
			/*fDelayOverride=*/0.0f);
	}
}
