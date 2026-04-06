#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPlayerController__ServerChangeCoalition : public HookBase<
	void(__fastcall*)(ATgPlayerController*, void*, unsigned char),
	0x109639c0,
	TgPlayerController__ServerChangeCoalition> {
public:
	static void __fastcall Call(ATgPlayerController* Controller, void* edx, unsigned char nCoalition);
	static inline void __fastcall CallOriginal(ATgPlayerController* Controller, void* edx, unsigned char nCoalition) {
		m_original(Controller, edx, nCoalition);
	};
};
