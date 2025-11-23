#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgGame__InitGameRepInfo : public HookBase<
	void(__fastcall*)(ATgGame*, void*),
	0x10AD9B80,
	TgGame__InitGameRepInfo> {
public:
	static void __fastcall* Call(ATgGame* Game, void* edx);
	static inline void __fastcall* CallOriginal(ATgGame* Game, void* edx) {
		LogCallOriginalBegin();
		m_original(Game, edx);
		LogCallOriginalEnd();
	}
};

