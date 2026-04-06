#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPlayerController__ServerSetPawnAlwaysRelevant : public HookBase<
	void(__fastcall*)(ATgPlayerController*, void*, int),
	0x10963000,
	TgPlayerController__ServerSetPawnAlwaysRelevant> {
public:
	static void __fastcall Call(ATgPlayerController* Controller, void* edx, int nDistSquared);
	static inline void __fastcall CallOriginal(ATgPlayerController* Controller, void* edx, int nDistSquared) {
		m_original(Controller, edx, nDistSquared);
	}
};
