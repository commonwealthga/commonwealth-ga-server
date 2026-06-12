#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgGame__BeginEndMission : public HookBase<
	bool(__fastcall*)(ATgGame*, void*, unsigned long, ACameraActor*, float),
	0x10ad9b00,
	TgGame__BeginEndMission> {
public:
	static bool __fastcall Call(ATgGame* Game, void* edx, unsigned long bClearNextMapGame, ACameraActor* endMissionCamera, float fDelayOverride);
	static inline bool __fastcall CallOriginal(ATgGame* Game, void* edx, unsigned long bClearNextMapGame, ACameraActor* endMissionCamera, float fDelayOverride) {
		return m_original(Game, edx, bClearNextMapGame, endMissionCamera, fDelayOverride);
	};
};

// Shared implementation — the TgGame_Ticket subclass native is also a stub
// and reuses the same flow (set winner, broadcast win-state, fire UC
// AllPlayersEndGame, arm FinishEndMission timer).
// Two-phase end flow. fDelayOverride ("Delay to End" — kismet
// TgSeqAct_EndMission.m_nDelay, or UC callers' explicit override) delays the
// END SCREEN, not the post-screen finish: win state/flags/IPC stamp
// immediately, the Scaleform scene opens after the delay. fDelayOverride==0:
// instant screen — except a boss kill (captured TgMissionObjective_Bot),
// which gets a default window so the death animation plays out first.
void BeginEndMissionImpl(ATgGame* Game, ACameraActor* endMissionCamera,
                         float fDelayOverride = 0.0f);

// Called by the TgGame__FinishEndMission hook on every timer fire: when a
// deferred end screen is pending, shows it (and re-arms the real finish
// timer), returning true so the caller skips the finish flow this round.
bool ConsumePendingEndScreen(ATgGame* Game);
