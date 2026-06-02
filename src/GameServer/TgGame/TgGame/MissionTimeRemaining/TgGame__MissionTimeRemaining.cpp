#include "src/GameServer/TgGame/TgGame/MissionTimeRemaining/TgGame__MissionTimeRemaining.hpp"
#include "src/GameServer/Engine/World/GetWorldInfo/World__GetWorldInfo.hpp"
#include "src/GameServer/Utils/ClassPreloader/ClassPreloader.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Tracks whether we've already fired alerts for each threshold this mission.
// Reset by ResetAlertsForExtendedTime when mission time is extended.
static bool s_bTimerAlertsInitialized = false;

// Fire TgSeqEvent_MissionTimeRemaining Kismet events when remaining time
// crosses their SecsRemaining thresholds. Called every 5s from KeepClientsInSync.
static void CheckTimerRemainingAlerts(ATgGame* Game, float fRemaining) {
	// Only during active mission (state 2) or overtime (state 3)
	int state = (int)Game->m_eTimerState;
	if (state != 2 && state != 3) return;
	if (fRemaining <= 0.0f) return;

	// Scan s_TimerRemainingAlerts for Kismet events whose threshold we've crossed
	TArray<UTgSeqEvent_MissionTimeRemaining*>& alerts = Game->s_TimerRemainingAlerts;
	if (alerts.Data == nullptr || alerts.Count == 0) {
		// Populate from Kismet if not yet done
		if (!s_bTimerAlertsInitialized) {
			s_bTimerAlertsInitialized = true;
			AWorldInfo* WorldInfo = World__GetWorldInfo::CallOriginal((UWorld*)Globals::Get().GWorld, nullptr, 0);
			if (WorldInfo == nullptr) return;

			TArray<USequenceObject*> Events;
			WorldInfo->GetGameSequence()->FindSeqObjectsByClass(
				ClassPreloader::GetTgSeqEventMissionTimeRemainingClass(),
				1,
				&Events
			);

			for (int i = 0; i < Events.Num(); i++) {
				alerts.Add((UTgSeqEvent_MissionTimeRemaining*)Events.Data[i]);
			}
			Logger::Log("timer", "Cached %d TgSeqEvent_MissionTimeRemaining events\n", alerts.Count);
		}
	}

	for (int i = 0; i < alerts.Count; i++) {
		UTgSeqEvent_MissionTimeRemaining* alert = alerts.Data[i];
		if (alert == nullptr) continue;

		// Check if threshold crossed: remaining time is at or below SecsRemaining
		// and we haven't fired this alert yet (offset 0xE8 = "already fired" flag)
		unsigned char* pFired = (unsigned char*)alert + 0xE8;
		if (*pFired) continue;  // already fired

		if (fRemaining <= alert->SecsRemaining) {
			*pFired = 1;

			// Fire the Kismet event
			TArray<int> Indices;
			Indices.Add(0);
			alert->CheckActivate(Game, Game, 0, 0, Indices);

			Logger::Log("timer", "Fired MissionTimeRemaining alert: %.0f seconds (remaining=%.1f)\n",
				alert->SecsRemaining, fRemaining);
		}
	}
}

float __fastcall TgGame__MissionTimeRemaining::Call(ATgGame *Game, void *edx) {
	AWorldInfo* WorldInfo = World__GetWorldInfo::CallOriginal((UWorld*)Globals::Get().GWorld, nullptr, 0);
	float now = WorldInfo->TimeSeconds;
	int state = (int)Game->m_eTimerState;

	float remaining = 0.0f;

	if (state == 1) {
		// Setup phase
		remaining = Game->s_fMissionTimerStartedAt - now;
	} else if (state == 2) {
		// Mission phase — prefer the engine's `MissionTimer` SetTimer state as
		// the source of truth. UC's MissionTimerStart / MissionTimeIncrement /
		// MissionTimerPause / MissionTimerStop / TgSeqAct_SetMissionTime all
		// mutate the timer via SetTimer('MissionTimer'); the
		// `s_fMissionTimerStartedAt + m_fGameMissionTime` formula only sees the
		// initial configuration, so any extension (e.g. boss-room
		// SoundInsulationVolume calling MissionTimerBossIncrement → +210s up to
		// 240s) flashed correctly for one tick — UC writes
		// r_fMissionRemainingTime directly in SendMissionTimerNotify — then got
		// overwritten by the next KeepClientsInSync 5s later when this function
		// returned the stale formula value. Same pattern state 6 already uses.
		remaining = Game->GetRemainingTimeForTimer(FName("MissionTimer"), nullptr);
		if (remaining < 0.0f) {
			// Engine timer not armed (e.g. between MissionTimerStop and a re-
			// start, or paused → state would be 5 normally). Fall back to the
			// static formula so we still report something sensible.
			remaining = (Game->s_fMissionTimerStartedAt + Game->m_fGameMissionTime) - now;
		}
	} else if (state == 3) {
		// Overtime. Same engine-timer-first logic as state 2 — `MissionTimer`
		// is re-armed when entering overtime (TgGame.uc:998 SetMissionTime +
		// MissionTimerStart) and on any MissionTimeIncrement during overtime,
		// so GetRemainingTimeForTimer is the single source of truth. The
		// stale s_fMissionTimerStartedAt + m_fGameMissionTime + m_fGameOvertimeTime
		// formula goes NEGATIVE as soon as regulation included any extension
		// (capture, boss-room) because those bump the engine timer past the
		// start+mission baseline — produces the "OT: -2:-43" HUD readout.
		remaining = Game->GetRemainingTimeForTimer(FName("MissionTimer"), nullptr);
		if (remaining < 0.0f) {
			remaining = (Game->s_fMissionTimerStartedAt + Game->m_fGameMissionTime + Game->m_fGameOvertimeTime) - now;
		}
	} else if (state == 5) {
		// Paused timer: the UC pause path stores the remaining time here.
		remaining = Game->m_fPausedAtTime;
	} else if (state == 6) {
		// Custom timers, used by Arena/Defense round setup and round timers,
		// do not derive from s_fMissionTimerStartedAt + m_fGameMissionTime.
		// MissionTimerStart() arms the actual one-shot timer using m_fMissionTime,
		// so ask the actor timer manager for the authoritative remaining value.
		remaining = Game->GetRemainingTimeForTimer(FName("MissionTimer"), nullptr);
		if (remaining < 0.0f) {
			ATgRepInfo_Game* GRI = (ATgRepInfo_Game*)Game->GameReplicationInfo;
			remaining = (GRI != nullptr && GRI->r_nMissionTimerState == 1)
				? Game->m_fMissionTime
				: 0.0f;
		}
	}

	// Check timer-based Kismet alerts (fires at 60s, 30s, 10s etc. as placed in map)
	CheckTimerRemainingAlerts(Game, remaining);

	return remaining;
}
