#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPlayerController__ServerSetLevel : public HookBase<
	void(__fastcall*)(ATgPlayerController*, void*, int),
	0x10963aa0,
	TgPlayerController__ServerSetLevel> {
public:
	static void __fastcall Call(ATgPlayerController* Controller, void* edx, int nLevel);
	static inline void __fastcall CallOriginal(ATgPlayerController* Controller, void* edx, int nLevel) {
		m_original(Controller, edx, nLevel);
	}
};
