#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgGame__GetReviveTimeRemaining : public HookBase<
	int(__fastcall*)(ATgGame*, void*, AController*),
	0x10ad9ce0,
	TgGame__GetReviveTimeRemaining> {
public:
	static int __fastcall Call(ATgGame* Game, void* edx, AController* Controller);
	static inline int __fastcall CallOriginal(ATgGame* Game, void* edx, AController* Controller) {
		return m_original(Game, edx, Controller);
	};
};


