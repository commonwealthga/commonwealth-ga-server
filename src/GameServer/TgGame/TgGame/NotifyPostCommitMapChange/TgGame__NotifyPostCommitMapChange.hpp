#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgGame__NotifyPostCommitMapChange : public HookBase<
	void(__fastcall*)(ATgGame*, void*),
	0x10a744e0,
	TgGame__NotifyPostCommitMapChange> {
public:
	static void __fastcall Call(ATgGame* Game, void* edx);
	static inline void __fastcall CallOriginal(ATgGame* Game, void* edx) {
		m_original(Game, edx);
	};
};
