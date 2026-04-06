#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPlayerController__ServerSalvageInvItem : public HookBase<
	void(__fastcall*)(ATgPlayerController*, void*, int, int),
	0x10963a80,
	TgPlayerController__ServerSalvageInvItem> {
public:
	static void __fastcall Call(ATgPlayerController* Controller, void* edx, int nInvId, int nCount);
	static inline void __fastcall CallOriginal(ATgPlayerController* Controller, void* edx, int nInvId, int nCount) {
		m_original(Controller, edx, nInvId, nCount);
	}
};
