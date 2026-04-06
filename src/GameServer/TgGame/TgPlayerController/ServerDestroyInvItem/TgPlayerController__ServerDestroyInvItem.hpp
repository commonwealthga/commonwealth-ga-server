#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPlayerController__ServerDestroyInvItem : public HookBase<
	void(__fastcall*)(ATgPlayerController*, void*, int, int, unsigned long),
	0x10963a50,
	TgPlayerController__ServerDestroyInvItem> {
public:
	static void __fastcall Call(ATgPlayerController* Controller, void* edx, int nInvId, int nCount, unsigned long bSellIt);
	static inline void __fastcall CallOriginal(ATgPlayerController* Controller, void* edx, int nInvId, int nCount, unsigned long bSellIt) {
		m_original(Controller, edx, nInvId, nCount, bSellIt);
	};
};
