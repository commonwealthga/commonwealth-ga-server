#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgGame__SendMissionTimerEvent : public HookBase<
	void(__fastcall*)(ATgGame*, void*, int),
	0x10ad9c30,
	TgGame__SendMissionTimerEvent> {
public:
	static TArray<int>* ActivateIndices;
	static void __fastcall Call(ATgGame* Game, void* edx, int nEventId);
	static inline void __fastcall CallOriginal(ATgGame* Game, void* edx, int nEventId) {
		m_original(Game, edx, nEventId);
	};
};


