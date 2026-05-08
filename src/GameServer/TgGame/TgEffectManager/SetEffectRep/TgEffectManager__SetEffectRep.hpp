#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

// Diagnostic-only wrap of TgEffectManager::SetEffectRep @ 0x10a6ffe0.
// Original is intact (NOT stripped) — we just log args + return for visibility.
// UC signature: native function int SetEffectRep(int nEffectGroupID, float fTime, bool bIsBuff, int nHealthChange);
class TgEffectManager__SetEffectRep : public HookBase<
	int(__fastcall*)(ATgEffectManager*, void*, int, float, unsigned long, int),
	0x10a6ffe0,
	TgEffectManager__SetEffectRep> {
public:
	static int __fastcall Call(ATgEffectManager* Manager, void* edx, int nEffectGroupID, float fTime, unsigned long bIsBuff, int nHealthChange);
	static inline int __fastcall CallOriginal(ATgEffectManager* Manager, void* edx, int nEffectGroupID, float fTime, unsigned long bIsBuff, int nHealthChange) {
		return m_original(Manager, edx, nEffectGroupID, fTime, bIsBuff, nHealthChange);
	};
};
