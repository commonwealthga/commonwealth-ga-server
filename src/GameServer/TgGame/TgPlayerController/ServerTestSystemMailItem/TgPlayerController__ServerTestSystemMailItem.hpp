#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgPlayerController__ServerTestSystemMailItem : public HookBase<
	void(__fastcall*)(ATgPlayerController*, void*, int, int, int, unsigned long, int),
	0x10962ff0,
	TgPlayerController__ServerTestSystemMailItem> {
public:
	static void __fastcall Call(ATgPlayerController* Controller, void* edx, int nPlayerId, int nItemId, int nQuantity, unsigned long bSystemCraft, int nMaxQuanlityValueId);
	static inline void __fastcall CallOriginal(ATgPlayerController* Controller, void* edx, int nPlayerId, int nItemId, int nQuantity, unsigned long bSystemCraft, int nMaxQuanlityValueId) {
		m_original(Controller, edx, nPlayerId, nItemId, nQuantity, bSystemCraft, nMaxQuanlityValueId);
	};
};
