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
void BeginEndMissionImpl(ATgGame* Game, ACameraActor* endMissionCamera);
