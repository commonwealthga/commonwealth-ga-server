#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgGame__BeginEndMission : public HookBase<
	void(__fastcall*)(ATgGame*, void*, bool, ACameraActor*, float),
	0x10ad9b00,
	TgGame__BeginEndMission> {
public:
	static void __fastcall Call(ATgGame* Game, void* edx, bool bClearNextMapGame, ACameraActor* endMissionCamera, float fDelayOverride);
	static inline void __fastcall CallOriginal(ATgGame* Game, void* edx, bool bClearNextMapGame, ACameraActor* endMissionCamera, float fDelayOverride) {
		m_original(Game, edx, bClearNextMapGame, endMissionCamera, fDelayOverride);
	};
};


