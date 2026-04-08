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
		// Mission phase
		remaining = (Game->s_fMissionTimerStartedAt + Game->m_fGameMissionTime) - now;
	} else if (state == 3) {
		// Overtime
		remaining = (Game->s_fMissionTimerStartedAt + Game->m_fGameMissionTime + Game->m_fGameOvertimeTime) - now;
	}

	// Check timer-based Kismet alerts (fires at 60s, 30s, 10s etc. as placed in map)
	CheckTimerRemainingAlerts(Game, remaining);

	return remaining;
}

