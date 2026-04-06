#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgGame_Ticket__BeginEndMission : public HookBase<
	bool(__fastcall*)(ATgGame_Ticket*, void*, unsigned long, ACameraActor*, float),
	0x10ad9d60,
	TgGame_Ticket__BeginEndMission> {
public:
	static bool __fastcall Call(ATgGame_Ticket* Game, void* edx, unsigned long bClearNextMapGame, ACameraActor* endMissionCamera, float fDelayOverride);
	static inline bool __fastcall CallOriginal(ATgGame_Ticket* Game, void* edx, unsigned long bClearNextMapGame, ACameraActor* endMissionCamera, float fDelayOverride) {
		return m_original(Game, edx, bClearNextMapGame, endMissionCamera, fDelayOverride);
	};
};
