#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPlayerController__ServerMarkSpawnReturn : public HookBase<
	void(__fastcall*)(ATgPlayerController*, void*, ATeleporter*),
	0x10963370,
	TgPlayerController__ServerMarkSpawnReturn> {
public:
	static void __fastcall Call(ATgPlayerController* Controller, void* edx, ATeleporter* pTeleporter);
	static inline void __fastcall CallOriginal(ATgPlayerController* Controller, void* edx, ATeleporter* pTeleporter) {
		m_original(Controller, edx, pTeleporter);
	}
};
