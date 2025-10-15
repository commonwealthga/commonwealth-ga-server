#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgGame_Arena__FinalizeRoundScore : public HookBase<
	void(__fastcall*)(ATgGame*, void*, int),
	0x10ad9bf0,
	TgGame_Arena__FinalizeRoundScore> {
public:
	static void __fastcall Call(ATgGame_Arena* Game, void* edx, int nPriority);
	static inline void __fastcall CallOriginal(ATgGame_Arena* Game, void* edx, int nPriority) {
		m_original(Game, edx, nPriority);
	};
};


