#include "src/GameServer/TgGame/TgGame_Arena/FinalizeRoundScore/TgGame_Arena__FinalizeRoundScore.hpp"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/GameServer/Storage/ClientConnectionsData/ClientConnectionsData.hpp"
#include "src/Utils/Logger/Logger.hpp"

static const int MSGID_YOUR_TEAM_TAKEN_LEAD  = 39056;
static const int MSGID_YOUR_TEAM_LOST_LEAD   = 39057;
static const int MSGID_TEAMS_TIED            = 39058;

// AlertPriority::APT_High = 2, AlertType::ATT_Important = 3
static const unsigned char ALERT_PRIORITY_HIGH = 2;
static const unsigned char ALERT_TYPE_IMPORTANT = 3;
static const float ALERT_DURATION = 5.0f;

// Send a lead-change alert to each connected player controller.
// The alert text depends on whether the player is on the leading team, losing team, or tied.
static void SendLeadChangeAlerts(ATgGame_Arena* Game, ATgRepInfo_TaskForce* NewLeader, ATgRepInfo_TaskForce* OtherTeam) {
	bool bTied = (NewLeader == nullptr) ||
		(OtherTeam != nullptr && NewLeader->r_nCurrentPointCount == OtherTeam->r_nCurrentPointCount);

	for (auto& pair : GClientConnectionsData) {
		ClientConnectionData& conn = pair.second;
		if (conn.bClosed || conn.Pawn == nullptr) continue;

		ATgPlayerController* PC = (ATgPlayerController*)conn.Pawn->Controller;
		if (PC == nullptr) continue;

		ATgRepInfo_Player* PRI = (ATgRepInfo_Player*)PC->PlayerReplicationInfo;
		if (PRI == nullptr || PRI->r_TaskForce == nullptr) continue;

		int nMsgId;
		if (bTied) {
			nMsgId = MSGID_TEAMS_TIED;
		} else if (PRI->r_TaskForce == NewLeader) {
			nMsgId = MSGID_YOUR_TEAM_TAKEN_LEAD;
		} else {
			nMsgId = MSGID_YOUR_TEAM_LOST_LEAD;
		}

		PC->AddAlertScript(ALERT_PRIORITY_HIGH, ALERT_TYPE_IMPORTANT, ALERT_DURATION, nMsgId, 1, 0);
	}
}

// Called when a round ends with a winner. Increments that team's round win count.
// Also sends lead-change announcements to all connected players.
void __fastcall TgGame_Arena__FinalizeRoundScore::Call(ATgGame_Arena* Game, void* edx, ATgRepInfo_TaskForce* Winner) {
	LogCallBegin();

	if (Winner == nullptr) {
		LogCallEnd();
		return;
	}

	// Capture scores before increment for lead-change detection
	ATgRepInfo_TaskForce* otherTeam = (Winner == GTeamsData.Attackers) ? GTeamsData.Defenders : GTeamsData.Attackers;
	int winnerScoreBefore = Winner->r_nCurrentPointCount;
	int otherScoreBefore = otherTeam != nullptr ? otherTeam->r_nCurrentPointCount : 0;

	// Determine previous lead state
	bool bWasAhead = (winnerScoreBefore > otherScoreBefore);
	bool bWasTied  = (winnerScoreBefore == otherScoreBefore);

	// Increment score
	Winner->r_nCurrentPointCount++;
	Winner->bNetDirty = 1;
	Winner->bForceNetUpdate = 1;

	// Detect lead changes and send announcements
	int winnerScoreAfter = Winner->r_nCurrentPointCount;
	bool bNowAhead = (winnerScoreAfter > otherScoreBefore);

	if (bWasTied && bNowAhead) {
		// Was tied, now this team leads → "taken the lead" / "lost the lead"
		SendLeadChangeAlerts(Game, Winner, otherTeam);
		Logger::Log("announcer", "Lead change: team %d takes the lead (%d-%d)\n",
			Winner->r_nTaskForce, winnerScoreAfter, otherScoreBefore);
	} else if (!bWasAhead && bNowAhead) {
		// Was behind, now ahead → "taken the lead"
		SendLeadChangeAlerts(Game, Winner, otherTeam);
		Logger::Log("announcer", "Lead change: team %d takes the lead (%d-%d)\n",
			Winner->r_nTaskForce, winnerScoreAfter, otherScoreBefore);
	}

	LogCallEnd();
}


