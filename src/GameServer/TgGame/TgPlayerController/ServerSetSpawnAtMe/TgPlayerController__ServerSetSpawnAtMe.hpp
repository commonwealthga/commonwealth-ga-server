#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPlayerController__ServerSetSpawnAtMe : public HookBase<
	void(__fastcall*)(ATgPlayerController*, void*, int),
	0x10963ba0,
	TgPlayerController__ServerSetSpawnAtMe> {
public:
	static void __fastcall Call(ATgPlayerController* Controller, void* edx, int bSetToMe);
	static inline void __fastcall CallOriginal(ATgPlayerController* Controller, void* edx, int bSetToMe) {
		m_original(Controller, edx, bSetToMe);
	}
};
