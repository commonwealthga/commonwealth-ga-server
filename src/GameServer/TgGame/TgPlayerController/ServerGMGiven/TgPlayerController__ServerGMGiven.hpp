#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPlayerController__ServerGMGiven : public HookBase<
	void(__fastcall*)(ATgPlayerController*, void*, int, int),
	0x10963a20,
	TgPlayerController__ServerGMGiven> {
public:
	static void __fastcall Call(ATgPlayerController* Controller, void* edx, int nCharacterId, int nCurrency);
	static inline void __fastcall CallOriginal(ATgPlayerController* Controller, void* edx, int nCharacterId, int nCurrency) {
		m_original(Controller, edx, nCharacterId, nCurrency);
	};
};
