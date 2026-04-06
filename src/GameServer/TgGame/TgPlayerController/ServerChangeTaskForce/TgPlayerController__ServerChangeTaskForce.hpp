#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPlayerController__ServerChangeTaskForce : public HookBase<
	void(__fastcall*)(ATgPlayerController*, void*, unsigned char),
	0x109639b0,
	TgPlayerController__ServerChangeTaskForce> {
public:
	static void __fastcall Call(ATgPlayerController* Controller, void* edx, unsigned char nTaskForce);
	static inline void __fastcall CallOriginal(ATgPlayerController* Controller, void* edx, unsigned char nTaskForce) {
		m_original(Controller, edx, nTaskForce);
	};
};
