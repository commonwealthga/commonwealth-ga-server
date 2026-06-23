#pragma once

// Server-side driver for kismet SeqEvent_Console events — the mechanism the
// shipped game used to trigger mission VO ("Ava", the Dome Defense handler) and
// other scripted console events.
//
// Background: the announcer-VO kismet (streamed *_Sound sublevel) listens for
// SeqEvent_Console nodes (Bancroft_SetupBanter / Bancroft_Start / _HalfwayPoint
// / _30sRemaining / _10sRemaining / _BossIncoming / _BossDefeated / ...). Those
// events fire via CauseEvent ('ce'): PlayerController.ServerCauseEvent walks the
// game sequence's SeqEvent_Console list and CheckActivate's the matching one. On
// our server nothing called it (the original native mission director is gone), so
// the VO never triggered. CauseEvent::Fire reproduces ServerCauseEvent exactly.
//
// Pipeline is engine-confirmed end-to-end on a dedicated server: the activated
// SeqEvent_Console drives SeqAct_PlaySound, whose ActivateSound() (no target)
// loops WorldInfo.ControllerList and calls PC.eventKismet_ClientPlaySound — a
// client RPC — so every connected client hears it.
class MissionVO {
public:
	// Fire the kismet console event whose ConsoleEventName matches `name`,
	// exactly as 'ce <name>' would. No-op (logs) when there's no game sequence
	// or no player to act as instigator (the events are playerOnly=True, so
	// CheckActivate rejects a non-player-owned instigator). Returns the number
	// of matching events activated. Channel: "missionvo".
	static int CauseEvent(const char* name);
};
