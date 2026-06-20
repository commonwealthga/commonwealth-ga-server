#include "src/GameServer/TgGame/TgMissionObjective/UpdateMatineeNodeStatus/TgMissionObjective__UpdateMatineeNodeStatus.hpp"
#include "src/Utils/Logger/Logger.hpp"
#include <unordered_map>

// TgSeqEvent_ObjectiveMatineeCommander output-pin indices (from the UC
// defaultproperties): 0=Play, 1=Reverse, 2=Stop, 3=Pause, 4=ChangeDir.
//
// IMPORTANT: pin 3 (Pause) on UE3's SeqAct_Interp is a TOGGLE — each trigger
// flips bPaused. Firing it twice in a row pauses then un-pauses, which is the
// opposite of what we want when consecutive status transitions both map to
// Pause (e.g. attacker-INHOLD 4 → defender-INHOLD 3 during a touch-and-leave).
// We track the last pin fired per actor and suppress consecutive duplicates so
// Pause only toggles on actual state-group changes.
//
// MissionObjectiveStatus → pin mapping. Anything outside the table is a no-op
// (e.g. NONE / ATTACKER_WAITING / ATTACKER_START_NEXT — those don't drive
// the matinee directly).
static int MapStatusToPin(unsigned char eStatus) {
	switch (eStatus) {
		case 6: return 0;  // ATTACKER_INPROGRESS  → Play (payload advances)
		case 5: return 1;  // DEFENDER_INPROGRESS  → Reverse (payload retreats)
		case 1:            // PAUSED
		case 2:            // PAUSED_CONTESTED
		case 3:            // DEFENDER_INHOLD  (hold timer counting down, no movement yet)
		case 4: return 3;  // ATTACKER_INHOLD
		case 7:            // DEFENDER_CAPTURED  (final defender win, payload done)
		case 8: return 2;  // ATTACKER_CAPTURED  (final attacker win, payload reached end)
		default: return -1;
	}
}

// Last pin successfully fired per actor. Single-threaded by assumption
// (UC dispatch is game-thread only, same as the rest of our hooks).
static std::unordered_map<ATgMissionObjective*, int> s_LastFiredPin;

void __fastcall TgMissionObjective__UpdateMatineeNodeStatus::Call(ATgMissionObjective* Obj, void* edx, unsigned char eStatus) {
	if (Obj == nullptr) return;

	// Capture-moment trace for the CTR rotation points (objId 345). Logs EVERY
	// status transition with the live meter + nearby count, so we can see what
	// drives a point to status 7/8 (captured) — players present? meter at an
	// extreme? owner pre-assigned?
	if (Obj->nObjectiveId == 345 && Logger::IsChannelEnabled("ctrrot")) {
		ATgMissionObjective_Proximity* po = (ATgMissionObjective_Proximity*)Obj;
		int nNearby = po->s_CollisionProxy ? po->s_CollisionProxy->m_NearByPlayers.Count : -1;
		Logger::Log("ctrrot",
			"StatusChange %s: -> status=%d ownerTF=%d currOwnerTF=%d defOwnerTF=%d capOnce=%d time=%.2f curr=%.2f nearby=%d active=%d locked=%d\n",
			po->GetName(), (int)eStatus, po->r_nOwnerTaskForce, po->m_nCurrOwnerTaskforce,
			po->nDefaultOwnerTaskForce, (int)po->m_bCaptureOnlyOnce, po->m_fTimeToCapture,
			po->m_fCurrCaptureTime, nNearby, (int)po->r_bIsActive, (int)po->r_bIsLocked);
	}

	const int pin = MapStatusToPin(eStatus);
	if (pin < 0) return;

	// Suppress consecutive duplicate triggers (see the Pause-toggle note above).
	auto it = s_LastFiredPin.find(Obj);
	const bool sameAsLast = (it != s_LastFiredPin.end() && it->second == pin);
	if (sameAsLast) {
		if (Logger::IsChannelEnabled("matinee")) {
			Logger::Log("matinee",
				"TgMissionObjective::UpdateMatineeNodeStatus — %s status=%d → pin=%d  SUPPRESSED (same as last)\n",
				((UObject*)Obj)->GetFullName(), (int)eStatus, pin);
		}
		return;
	}

	if (Logger::IsChannelEnabled("matinee")) {
		// Walk GeneratedEvents and count which TgSeqEvent_ObjectiveMatineeCommander
		// events are bound to this actor — tells us whether the map's Kismet has
		// any commander events wired at all, and helps diagnose "trigger fires
		// but matinee doesn't react" (typically: map didn't wire OutputLinks[pin]
		// to the SeqAct_Interp's matching input).
		int totalEvents = Obj->GeneratedEvents.Count;
		int commanderCount = 0;
		for (int i = 0; i < totalEvents; i++) {
			USequenceEvent* Evt = Obj->GeneratedEvents.Data[i];
			if (Evt == nullptr) continue;
			const char* clsName = Evt->Class ? Evt->Class->GetName() : "?";
			if (strstr(clsName, "TgSeqEvent_ObjectiveMatineeCommander") != nullptr) {
				commanderCount++;
			}
		}
		Logger::Log("matinee",
			"TgMissionObjective::UpdateMatineeNodeStatus — %s status=%d → pin=%d  (GeneratedEvents: %d total, %d MatineeCommander)\n",
			((UObject*)Obj)->GetFullName(), (int)eStatus, pin, totalEvents, commanderCount);
	}

	// eventTriggerNodeCommander wraps TriggerEventClass(TgSeqEvent_ObjectiveMatineeCommander,
	// self, pin) — fires the appropriate output pin on every linked Kismet
	// MatineeCommander event for this actor (Engine populates GeneratedEvents
	// from the map's Kismet at load time).
	Obj->eventTriggerNodeCommander(pin);
	s_LastFiredPin[Obj] = pin;
}
