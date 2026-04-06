#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPlayerController__ServerActivateInvItem : public HookBase<
	void(__fastcall*)(ATgPlayerController*, void*, int),
	0x10963a90,
	TgPlayerController__ServerActivateInvItem> {
public:
	static void __fastcall Call(ATgPlayerController* Controller, void* edx, int nInvId);
	static inline void __fastcall CallOriginal(ATgPlayerController* Controller, void* edx, int nInvId) {
		m_original(Controller, edx, nInvId);
	};
};
