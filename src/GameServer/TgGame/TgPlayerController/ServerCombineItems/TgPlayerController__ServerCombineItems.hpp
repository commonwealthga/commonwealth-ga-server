#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPlayerController__ServerCombineItems : public HookBase<
	void(__fastcall*)(ATgPlayerController*, void*, int, int),
	0x10963a30,
	TgPlayerController__ServerCombineItems> {
public:
	static void __fastcall Call(ATgPlayerController* Controller, void* edx, int nInvId, int nModKitInvId);
	static inline void __fastcall CallOriginal(ATgPlayerController* Controller, void* edx, int nInvId, int nModKitInvId) {
		m_original(Controller, edx, nInvId, nModKitInvId);
	};
};
