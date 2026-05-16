#include "src/GameServer/TgGame/TgGame_Ticket/UpdateGameWinState/TgGame_Ticket__UpdateGameWinState.hpp"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Stripped native. Called from UC TgGame_Ticket.MissionTimer and from our
// TickTicketsCalculation. Updates the standings booleans the UC reads from
// (m_bDefendersAhead / m_bTeamsTied), and once either side reaches the
// r_nPointsToWin threshold sets r_Winner and m_bTicketCountReached.
//
// Does NOT call BeginEndMission yet — that path is intentionally deferred
// (per user note: "massive task for another time"). When the threshold is
// crossed the winner is recorded; ending the round is a follow-up.
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
			"UpdateGameWinState — TICKET TARGET REACHED: %s wins (att=%d def=%d target=%d) — BeginEndMission not yet wired\n",
			(att >= target) ? "Attackers" : "Defenders", att, def, target);
	}
}
