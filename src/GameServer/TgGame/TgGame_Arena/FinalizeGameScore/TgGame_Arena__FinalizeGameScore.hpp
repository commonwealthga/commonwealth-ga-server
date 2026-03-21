#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgGame_Arena__FinalizeGameScore : public HookBase<
	void(__fastcall*)(ATgGame_Arena*, void*, ATgRepInfo_TaskForce*),
	0x10ad9d80,
	TgGame_Arena__FinalizeGameScore> {
public:
	static void __fastcall Call(ATgGame_Arena* Game, void* edx, ATgRepInfo_TaskForce* Winner);
	static inline void __fastcall CallOriginal(ATgGame_Arena* Game, void* edx, ATgRepInfo_TaskForce* Winner) {
		m_original(Game, edx, Winner);
	};
};


