#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPlayerController__ServerSetTaskForceLead : public HookBase<
	void(__fastcall*)(ATgPlayerController*, void*, int),
	0x10963350,
	TgPlayerController__ServerSetTaskForceLead> {
public:
	// One stack arg (original stub does ret 0x4).
	static void __fastcall Call(ATgPlayerController* Controller, void* edx, int a2);
	static inline void __fastcall CallOriginal(ATgPlayerController* Controller, void* edx, int a2) {
		m_original(Controller, edx, a2);
	}
};
