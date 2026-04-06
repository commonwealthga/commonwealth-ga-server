#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPlayerController__ServerDevGiveXP : public HookBase<
	void(__fastcall*)(ATgPlayerController*, void*, int),
	0x109639d0,
	TgPlayerController__ServerDevGiveXP> {
public:
	static void __fastcall Call(ATgPlayerController* Controller, void* edx, int Amt);
	static inline void __fastcall CallOriginal(ATgPlayerController* Controller, void* edx, int Amt) {
		m_original(Controller, edx, Amt);
	};
};
