#pragma once

#include "src/pch.hpp"
#include "src/Utils/HookBase.hpp"

class TgEffectManager__RemoveAllEffects : public HookBase<
	void(__fastcall*)(ATgEffectManager*, void*, void*),
	0x10a6ef00,
	TgEffectManager__RemoveAllEffects> {
public:
	static void __fastcall Call(ATgEffectManager* pThis, void* edx, void*);
	static inline void __fastcall CallOriginal(ATgEffectManager* pThis, void* edx, void* ExludeCategoryCodes) {
		m_original(pThis, edx, ExludeCategoryCodes);
	};
};

