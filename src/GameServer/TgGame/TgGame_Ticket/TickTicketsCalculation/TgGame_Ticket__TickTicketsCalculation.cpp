#include "src/GameServer/TgGame/TgGame_Ticket/TickTicketsCalculation/TgGame_Ticket__TickTicketsCalculation.hpp"
#include "src/GameServer/TgGame/TgGame_Ticket/UpdateGameWinState/TgGame_Ticket__UpdateGameWinState.hpp"
#include "src/GameServer/Storage/TeamsData/TeamsData.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Stripped native. Called once per second from a timer the UC PostBeginPlay
// installs (SetTimer(m_fTicketTickDelay, true, 'TickTicketsCalculation')).
//
// For each KOTH/Ticket objective with a non-zero r_nOwnerTaskForce, credit
// that taskforce with m_nPointsPerSecond (scaled by the tick delay) into
// r_nCurrentPointCount. r_nCurrentPointCount is in the always-replicated
// rep block, so the HUD/scoreboard updates automatically.
//
// IMPORTANT: we gate on owner, not on r_eStatus being captured (7/8). UC
// only updates r_nOwnerTaskForce on a successful 7/8 transition, so it
// retains the *previous* owner while the enemy is contesting/capturing
// (states 5/6 in-progress, 3/4 in-hold, 2 contested). That means the prior
// owner keeps earning tickets right up until the enemy fully captures —
// which is the intended Ticket-mode behaviour: a half-captured point still
// belongs to whoever held it last.
//
// We also gate on r_bHasBeenCapturedOnce. r_nOwnerTaskForce is seeded at
// map load from nDefaultOwnerTaskForce (usually 2 for these maps), which
// would otherwise mean defenders silently pocket points from t=0 without
// having fought for any. KOTH UC OnObjectiveStatusChange sets
// r_bHasBeenCapturedOnce=true only on the 7/8 capture transition, so this
// flag is the authoritative "someone actually captured this point at
// least once" signal.
//
// Then runs UpdateGameWinState inline so the winner/standings reflect this
// tick's award immediately. (UC also runs UpdateGameWinState from
// MissionTimer, but that's a coarser timer.)
// Minimum captured points a team must hold to start earning. Below this,
// the team scores nothing — even if they own one and the enemy owns zero.
static constexpr int kMinOwnedToEarn = 2;

void __fastcall TgGame_Ticket__TickTicketsCalculation::Call(ATgGame_Ticket* Game, void* edx) {
	if (Game == nullptr) return;

	ATgRepInfo_Game* GRI = reinterpret_cast<ATgRepInfo_Game*>(Game->GameReplicationInfo);
	if (GRI == nullptr) return;
	if (GTeamsData.Attackers == nullptr || GTeamsData.Defenders == nullptr) return;

	// Don't award during setup / pre-match. r_nMissionTimerState flips to
	// MTS_RUNNING (1) when UC SendMissionTimerNotify is called with the
	// running state, which happens once the mission timer actually starts
	// counting down. Setup-phase ticks will still fire from the PostBeginPlay
	// timer but we skip them.
	if (GRI->r_nMissionTimerState != 1) {
		return;
	}

	// Don't award during end-mission either. BeginEndMissionImpl sets
	// bGameEnded; UC ClearTimer would be cleaner but the SetTimer was armed
	// in UC PostBeginPlay with no UC handle we can reach from here. This gate
	// also covers the case where the threshold was crossed by another path
	// (e.g. our UpdateGameWinState fire) — tickets shouldn't keep climbing
	// while the end-mission screen is showing.
	if (Game->bGameEnded) {
		return;
	}

	const float tickDelay = (Game->m_fTicketTickDelay > 0.0f) ? Game->m_fTicketTickDelay : 1.0f;

	int sumAtt = 0, sumDef = 0;
	int ownedAtt = 0, ownedDef = 0;
	for (int i = 0; i < GRI->m_MissionObjectives.Count; i++) {
		ATgMissionObjective* Obj = GRI->m_MissionObjectives.Data[i];
		if (Obj == nullptr) continue;
		if (!Obj->r_bHasBeenCapturedOnce) continue;  // map-default owner doesn't pay until first real capture
		const int tf = Obj->r_nOwnerTaskForce;
		if (tf == 1) { sumAtt += Obj->m_nPointsPerSecond; ownedAtt++; }
		else if (tf == 2) { sumDef += Obj->m_nPointsPerSecond; ownedDef++; }
	}

	// Two-point minimum: a team only earns when they hold at least
	// kMinOwnedToEarn captured points. Holding just one yields nothing.
	if (ownedAtt < kMinOwnedToEarn) sumAtt = 0;
	if (ownedDef < kMinOwnedToEarn) sumDef = 0;

	const int addAtt = (int)(sumAtt * tickDelay + 0.5f);
	const int addDef = (int)(sumDef * tickDelay + 0.5f);

	if (addAtt > 0) GTeamsData.Attackers->r_nCurrentPointCount += addAtt;
	if (addDef > 0) GTeamsData.Defenders->r_nCurrentPointCount += addDef;

	if (ownedAtt > ownedDef)      Game->m_nTfControlledLast = 1;
	else if (ownedDef > ownedAtt) Game->m_nTfControlledLast = 2;
	else                          Game->m_nTfControlledLast = 0;

	if (Logger::IsChannelEnabled(GetLogChannel()) && (addAtt > 0 || addDef > 0)) {
		Logger::Log(GetLogChannel(),
			"TickTicketsCalculation — owned att=%d/def=%d, +%d/+%d → att=%d def=%d (target=%d)\n",
			ownedAtt, ownedDef, addAtt, addDef,
			GTeamsData.Attackers->r_nCurrentPointCount,
			GTeamsData.Defenders->r_nCurrentPointCount,
			GRI->r_nPointsToWin);
	}

	TgGame_Ticket__UpdateGameWinState::Call(Game, nullptr);
}
