#include "src/GameServer/TgGame/TgGame_PointRotation/CalcNextObjective/TgGame_PointRotation__CalcNextObjective.hpp"
#include "src/GameServer/Combat/MissionAlerts/MissionAlerts.hpp"
#include "src/GameServer/Engine/World/GetWorldInfo/World__GetWorldInfo.hpp"
#include "src/GameServer/Globals.hpp"
#include "src/GameServer/TgGame/TgMissionObjective/SetObjectivePending/TgMissionObjective__SetObjectivePending.hpp"
#include "src/Utils/Logger/Logger.hpp"

// Picks a random objective from GRI->m_MissionObjectives, excluding m_PreviousObjective.
// Sets m_NextObjective to the chosen objective and m_PreviousObjective for next round.
// All PointRotation objectives have nPriority=1, so no priority filtering is needed.
void __fastcall TgGame_PointRotation__CalcNextObjective::Call(ATgGame_PointRotation* Game, void* edx) {
	LogCallBegin();

	ATgRepInfo_Game* GRI = reinterpret_cast<ATgRepInfo_Game*>(Game->GameReplicationInfo);
	if (GRI == nullptr || GRI->m_MissionObjectives.Count == 0) {
		Logger::Log(GetLogChannel(), "No objectives to choose from\n");
		LogCallEnd();
		return;
	}

	// Build candidate list: all objectives except the previous one
	ATgMissionObjective* candidates[64];
	int numCandidates = 0;

	for (int i = 0; i < GRI->m_MissionObjectives.Count && numCandidates < 64; i++) {
		ATgMissionObjective* Obj = GRI->m_MissionObjectives.Data[i];
		if (Obj != nullptr && Obj != Game->m_PreviousObjective) {
			candidates[numCandidates++] = Obj;
		}
	}

	if (numCandidates == 0) {
		// Only one objective total — reuse it
		Game->m_NextObjective = Game->m_PreviousObjective;
		Logger::Log(GetLogChannel(), "Only one objective total\n");
		LogCallEnd();
		return;
	}

	// Pick a random candidate
	int index = rand() % numCandidates;
	Game->m_NextObjective = candidates[index];
	Game->m_PreviousObjective = Game->m_NextObjective;

	// Clear r_bIsPending on every other objective first. Without this, on
	// round 2+ a previously-pending objective still has the bit set in the
	// `recent` rep snapshot — and if the new pick happens to match, no delta
	// fires for it either. Belt-and-suspenders: drive ALL of them through
	// the reimplemented SetObjectivePending native so dirty/force-update is
	// applied consistently.
	for (int i = 0; i < GRI->m_MissionObjectives.Count; i++) {
		ATgMissionObjective* Obj = GRI->m_MissionObjectives.Data[i];
		if (Obj != nullptr && Obj != Game->m_NextObjective && Obj->r_bIsPending) {
			TgMissionObjective__SetObjectivePending::Call(Obj, nullptr, 0);
		}
	}

	// Mark the new pick. This is what triggers the client-side smoke/audio
	// "pending" FX on the next-up objective via ReplicatedEvent('r_bIsPending')
	// → native UpdateFxVisibility → c_fxPending->Activate().
	TgMissionObjective__SetObjectivePending::Call(Game->m_NextObjective, nullptr, 1);

	if (Logger::IsChannelEnabled(GetLogChannel())) {
		Logger::Log(GetLogChannel(), "Chosen objective: %s\n", Game->m_NextObjective->GetFullName());
	}

	// Update r_Objectives so newly connecting clients only see the current objective
	// GRI->r_Objectives[0] = Game->m_NextObjective;
	// for (int i = 1; i < 0x4B; i++) {
	// 	GRI->r_Objectives[i] = nullptr;
	// }

	// Plant the next-activation timestamp for MissionAlerts' countdown poller.
	// PointRotation's RoundInProgress::BeginState (UC) calls CalcNextObjective
	// and immediately sets `SetTimer(s_nObjectiveUnlockDelay, false, 'ObjectiveUnlock')`,
	// so the unlock will fire that many seconds from now.
	AWorldInfo* WI = World__GetWorldInfo::CallOriginal((UWorld*)Globals::Get().GWorld, nullptr, 0);
	if (WI != nullptr && Game->s_nObjectiveUnlockDelay > 0) {
		float scheduledAt = WI->TimeSeconds + (float)Game->s_nObjectiveUnlockDelay;
		MissionAlerts::NotifyNextObjectiveScheduled((void*)Game, scheduledAt);
	}

	LogCallEnd();
}


